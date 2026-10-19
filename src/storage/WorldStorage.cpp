#include "storage/WorldStorage.hpp"
#include "core/Log.hpp"
#include <algorithm>
#include <fstream>
#include <cerrno>
#include <set>
#include <system_error>
#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace OpenCraft {

const char* storageErrorToString(StorageError err) noexcept {
    switch (err) {
        case StorageError::None: return "Sin error";
        case StorageError::FileNotFound: return "Archivo no encontrado";
        case StorageError::IoError: return "Error de Entrada/Salida (I/O)";
        case StorageError::CorruptedData: return "Datos corruptos o formato alterado";
        case StorageError::ChecksumMismatch: return "Fallo de suma de verificación (Checksum Mismatch)";
        case StorageError::DiskFull: return "Disco sin espacio suficiente";
        case StorageError::InvalidMagic: return "Identificador mágico inválido";
        case StorageError::UnsupportedVersion: return "Versión de esquema no soportada";
        case StorageError::TransactionAborted: return "Transacción abortada";
        case StorageError::Locked: return "Mundo bloqueado por otra instancia";
    }
    return "Error desconocido";
}

WorldStorage::WorldStorage(std::filesystem::path worldDir) : m_worldDir(std::move(worldDir)) {}

WorldStorage::~WorldStorage() {
    if (m_inTransaction) {
        rollbackTransaction();
    }
}

bool WorldStorage::exists() const { return std::filesystem::exists(getMetadataPath()); }

bool WorldStorage::hasChunk(int cx, int cz, DimensionType dim) const {
    return std::filesystem::exists(getChunkPath(cx, cz, dim));
}

StorageError WorldStorage::init() {
    std::error_code ec;
    std::filesystem::create_directories(m_worldDir, ec);
    if (ec) return StorageError::IoError;

    std::filesystem::create_directories(m_worldDir / "players", ec);
    if (ec) return StorageError::IoError;

    std::filesystem::create_directories(m_worldDir / "chunks", ec);
    if (ec) return StorageError::IoError;

    std::filesystem::create_directories(m_worldDir / "DIM-1", ec);
    if (ec) return StorageError::IoError;

    std::filesystem::create_directories(m_worldDir / "DIM1", ec);
    if (ec) return StorageError::IoError;

    std::filesystem::create_directories(m_worldDir / "entities", ec);
    if (ec) return StorageError::IoError;

    return recoverJournalIfNeeded();
}

StorageError WorldStorage::initializeWorld(const std::string& name, uint64_t seed, uint32_t generatorVersion) {
    StorageError err = init();
    if (err != StorageError::None) return err;

    WorldMetadata meta;
    meta.worldName = name;
    meta.worldId = name;
    meta.seed = seed;
    meta.generatorVersion = generatorVersion;
    meta.creationTime = 0;
    meta.lastPlayedTime = 0;

    err = beginTransaction();
    if (err != StorageError::None) return err;
    err = stageMetadata(meta);
    if (err != StorageError::None) {
        rollbackTransaction();
        return err;
    }
    return commitTransaction();
}

namespace {

// Escritura duradera: los datos llegan al medio físico antes de continuar (fsync), de modo que un
// corte de energía no deja archivos vacíos ni un journal que apunte a datos inexistentes (A13)
StorageError writeFileDurable(const std::filesystem::path& path, const std::vector<uint8_t>& data) {
#if defined(__unix__) || defined(__APPLE__)
    int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return StorageError::IoError;
    size_t written = 0;
    while (written < data.size()) {
        ssize_t n = ::write(fd, data.data() + written, data.size() - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            int err = errno;
            ::close(fd);
            return (err == ENOSPC) ? StorageError::DiskFull : StorageError::IoError;
        }
        written += static_cast<size_t>(n);
    }
    bool synced = ::fsync(fd) == 0;
    bool closed = ::close(fd) == 0;
    return (synced && closed) ? StorageError::None : StorageError::IoError;
#elif defined(_WIN32)
    HANDLE h =
        CreateFileW(path.wstring().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return StorageError::IoError;
    size_t written = 0;
    while (written < data.size()) {
        DWORD chunk = static_cast<DWORD>(std::min<size_t>(data.size() - written, 1u << 30));
        DWORD n = 0;
        if (!WriteFile(h, data.data() + written, chunk, &n, nullptr)) {
            DWORD err = GetLastError();
            CloseHandle(h);
            return (err == ERROR_DISK_FULL || err == ERROR_HANDLE_DISK_FULL) ? StorageError::DiskFull
                                                                             : StorageError::IoError;
        }
        written += n;
    }
    bool synced = FlushFileBuffers(h) != 0;
    bool closed = CloseHandle(h) != 0;
    return (synced && closed) ? StorageError::None : StorageError::IoError;
#else
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return StorageError::IoError;
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    out.flush();
    return out ? StorageError::None : StorageError::DiskFull;
#endif
}

// Persistir las entradas de un directorio (renombres y borrados) en el medio físico
void syncDirectory(const std::filesystem::path& dir) {
#if defined(__unix__) || defined(__APPLE__)
    int fd = ::open(dir.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd >= 0) {
        ::fsync(fd);
        ::close(fd);
    }
#else
    (void)dir;
#endif
}

} // namespace

StorageError WorldStorage::recoverJournalIfNeeded() {
    std::filesystem::path jPath = getJournalPath();
    if (!std::filesystem::exists(jPath)) {
        return StorageError::None;
    }

    std::vector<uint8_t> jData;
    StorageError err = readBinaryFile(jPath, jData);
    if (err != StorageError::None || jData.size() < 12) {
        // Journal inválido o inaccesible; eliminarlo de forma segura
        std::error_code ec;
        std::filesystem::remove(jPath, ec);
        return StorageError::None;
    }

    uint32_t magic = static_cast<uint32_t>(jData[0]) | (static_cast<uint32_t>(jData[1]) << 8) |
                     (static_cast<uint32_t>(jData[2]) << 16) | (static_cast<uint32_t>(jData[3]) << 24);

    if (magic != MAGIC_JOURNAL) {
        std::error_code ec;
        std::filesystem::remove(jPath, ec);
        return StorageError::None;
    }

    uint8_t status = jData[4]; // 0: PENDING, 1: COMMITTED
    uint16_t entryCount = static_cast<uint16_t>(jData[5]) | (static_cast<uint16_t>(jData[6]) << 8);

    size_t offset = 7;
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> entries;

    for (uint16_t i = 0; i < entryCount; ++i) {
        if (offset + 2 > jData.size()) break;
        uint16_t tmpLen = static_cast<uint16_t>(jData[offset]) | (static_cast<uint16_t>(jData[offset + 1]) << 8);
        offset += 2;
        if (offset + tmpLen > jData.size()) break;
        std::string tmpStr(reinterpret_cast<const char*>(&jData[offset]), tmpLen);
        offset += tmpLen;

        if (offset + 2 > jData.size()) break;
        uint16_t finalLen = static_cast<uint16_t>(jData[offset]) | (static_cast<uint16_t>(jData[offset + 1]) << 8);
        offset += 2;
        if (offset + finalLen > jData.size()) break;
        std::string finalStr(reinterpret_cast<const char*>(&jData[offset]), finalLen);
        offset += finalLen;

        entries.emplace_back(m_worldDir / tmpStr, m_worldDir / finalStr);
    }

    std::error_code ec;
    if (status == 1) { // COMMITTED: Completar renombres atómicos pendientes
        std::set<std::filesystem::path> dirs;
        for (const auto& [tmpP, finalP] : entries) {
            if (std::filesystem::exists(tmpP)) {
                std::filesystem::create_directories(finalP.parent_path(), ec);
                std::filesystem::rename(tmpP, finalP, ec);
                if (ec) {
                    // Se conserva el journal para reintentar: borrarlo perdería la transacción
                    Log::error("Recuperación incompleta: no se pudo renombrar '{}' ({})", tmpP.string(), ec.message());
                    return StorageError::IoError;
                }
                dirs.insert(finalP.parent_path());
            }
        }
        for (const auto& d : dirs) syncDirectory(d);
    } else { // PENDING: Transacción incompleta, descartar archivos temporales
        for (const auto& [tmpP, finalP] : entries) {
            (void)finalP;
            if (std::filesystem::exists(tmpP)) {
                std::filesystem::remove(tmpP, ec);
            }
        }
    }

    std::filesystem::remove(jPath, ec);
    return StorageError::None;
}

StorageError WorldStorage::readBinaryFile(const std::filesystem::path& path, std::vector<uint8_t>& outBuffer) {
    if (!std::filesystem::exists(path)) {
        return StorageError::FileNotFound;
    }

    std::error_code ec;
    auto sz = std::filesystem::file_size(path, ec);
    if (ec) return StorageError::IoError;
    if (sz > MAX_FILE_SIZE) return StorageError::CorruptedData;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return StorageError::IoError;
    }

    outBuffer.resize(static_cast<size_t>(sz));
    file.read(reinterpret_cast<char*>(outBuffer.data()), sz);
    if (!file && !file.eof()) {
        return StorageError::IoError;
    }

    return StorageError::None;
}

StorageError WorldStorage::loadMetadata(WorldMetadata& outMeta) {
    std::vector<uint8_t> buffer;
    StorageError err = readBinaryFile(getMetadataPath(), buffer);
    if (err != StorageError::None) return err;

    SerializationResult res = WorldFormat::deserializeMetadata(buffer, outMeta);
    switch (res) {
        case SerializationResult::Success: return StorageError::None;
        case SerializationResult::InvalidMagic: return StorageError::InvalidMagic;
        case SerializationResult::UnsupportedVersion: return StorageError::UnsupportedVersion;
        case SerializationResult::ChecksumMismatch: return StorageError::ChecksumMismatch;
        default: return StorageError::CorruptedData;
    }
}

StorageError WorldStorage::loadPlayer(PlayerSaveData& outPlayer) { return loadPlayer(outPlayer, ""); }

StorageError WorldStorage::loadPlayer(PlayerSaveData& outPlayer, uint32_t entityId) {
    if (entityId == 0 || entityId == 1) {
        return loadPlayer(outPlayer, "");
    }
    return loadPlayer(outPlayer, std::to_string(entityId));
}

StorageError WorldStorage::loadPlayer(PlayerSaveData& outPlayer, const std::string& identifier) {
    std::vector<uint8_t> buffer;
    StorageError err = readBinaryFile(getPlayerPath(identifier), buffer);
    if (err != StorageError::None) return err;

    SerializationResult res = WorldFormat::deserializePlayer(buffer, outPlayer);
    switch (res) {
        case SerializationResult::Success: return StorageError::None;
        case SerializationResult::InvalidMagic: return StorageError::InvalidMagic;
        case SerializationResult::UnsupportedVersion: return StorageError::UnsupportedVersion;
        case SerializationResult::ChecksumMismatch: return StorageError::ChecksumMismatch;
        default: return StorageError::CorruptedData;
    }
}

StorageError WorldStorage::loadChunk(int cx, int cz, Chunk& outChunk, BlockRegistry& registry,
                                     std::vector<MobSaveData>* outMobs, DimensionType dim) {
    std::vector<uint8_t> buffer;
    StorageError err = readBinaryFile(getChunkPath(cx, cz, dim), buffer);
    if (err != StorageError::None) return err;

    ChunkSaveData data;
    SerializationResult res = WorldFormat::deserializeChunk(buffer, data);
    if (res != SerializationResult::Success) {
        switch (res) {
            case SerializationResult::InvalidMagic: return StorageError::InvalidMagic;
            case SerializationResult::UnsupportedVersion: return StorageError::UnsupportedVersion;
            case SerializationResult::ChecksumMismatch: return StorageError::ChecksumMismatch;
            default: return StorageError::CorruptedData;
        }
    }

    WorldFormat::applyChunkData(data, outChunk, registry);
    if (outMobs) {
        *outMobs = std::move(data.mobs);
    }
    outChunk.clearDirty();
    return StorageError::None;
}

StorageError WorldStorage::loadEntities(std::vector<ItemEntitySaveData>& outEntities) {
    std::vector<uint8_t> buffer;
    StorageError err = readBinaryFile(getEntitiesPath(), buffer);
    if (err == StorageError::FileNotFound) {
        outEntities.clear();
        return StorageError::None;
    }
    if (err != StorageError::None) return err;

    SerializationResult res = WorldFormat::deserializeEntities(buffer, outEntities);
    switch (res) {
        case SerializationResult::Success: return StorageError::None;
        case SerializationResult::InvalidMagic: return StorageError::InvalidMagic;
        case SerializationResult::UnsupportedVersion: return StorageError::UnsupportedVersion;
        case SerializationResult::ChecksumMismatch: return StorageError::ChecksumMismatch;
        default: return StorageError::CorruptedData;
    }
}

StorageError WorldStorage::beginTransaction() {
    if (m_inTransaction) {
        rollbackTransaction();
    }
    m_stagedFiles.clear();
    m_inTransaction = true;
    return StorageError::None;
}

StorageError WorldStorage::stageMetadata(const WorldMetadata& meta) {
    if (!m_inTransaction) return StorageError::TransactionAborted;

    StagedFile staged;
    staged.finalPath = getMetadataPath();
    staged.tmpPath = getMetadataPath().string() + ".tmp";
    SerializationResult res = WorldFormat::serializeMetadata(meta, staged.data);
    if (res != SerializationResult::Success) return StorageError::CorruptedData;

    m_stagedFiles.push_back(std::move(staged));
    return StorageError::None;
}

StorageError WorldStorage::stagePlayer(const PlayerSaveData& player) { return stagePlayer(player, ""); }

StorageError WorldStorage::stagePlayer(const PlayerSaveData& player, uint32_t entityId) {
    if (entityId == 0 || entityId == 1) {
        return stagePlayer(player, "");
    }
    return stagePlayer(player, std::to_string(entityId));
}

StorageError WorldStorage::stagePlayer(const PlayerSaveData& player, const std::string& identifier) {
    if (!m_inTransaction) return StorageError::TransactionAborted;

    StagedFile staged;
    staged.finalPath = getPlayerPath(identifier);
    staged.tmpPath = staged.finalPath.string() + ".tmp";
    SerializationResult res = WorldFormat::serializePlayer(player, staged.data);
    if (res != SerializationResult::Success) return StorageError::CorruptedData;

    m_stagedFiles.push_back(std::move(staged));
    return StorageError::None;
}

StorageError WorldStorage::stageChunk(const Chunk& chunk, const std::vector<MobSaveData>& mobs, DimensionType dim) {
    if (!m_inTransaction) return StorageError::TransactionAborted;

    StagedFile staged;
    staged.finalPath = getChunkPath(chunk.getChunkX(), chunk.getChunkZ(), dim);
    staged.tmpPath = staged.finalPath.string() + ".tmp";
    SerializationResult res = WorldFormat::serializeChunk(chunk, staged.data, mobs);
    if (res != SerializationResult::Success) return StorageError::CorruptedData;

    m_stagedFiles.push_back(std::move(staged));
    return StorageError::None;
}

StorageError WorldStorage::stageEntities(const std::vector<ItemEntitySaveData>& entities) {
    if (!m_inTransaction) return StorageError::TransactionAborted;

    StagedFile staged;
    staged.finalPath = getEntitiesPath();
    staged.tmpPath = getEntitiesPath().string() + ".tmp";
    SerializationResult res = WorldFormat::serializeEntities(entities, staged.data);
    if (res != SerializationResult::Success) return StorageError::CorruptedData;

    m_stagedFiles.push_back(std::move(staged));
    return StorageError::None;
}

StorageError WorldStorage::commitTransaction() {
    if (!m_inTransaction) return StorageError::TransactionAborted;
    if (m_stagedFiles.empty()) {
        m_inTransaction = false;
        return StorageError::None;
    }

    // 1. Escribir cada archivo staged a su ruta temporal .tmp
    for (const auto& staged : m_stagedFiles) {
        std::error_code ec;
        std::filesystem::create_directories(staged.tmpPath.parent_path(), ec);

        StorageError werr = writeFileDurable(staged.tmpPath, staged.data);
        if (werr != StorageError::None) {
            rollbackTransaction();
            return werr;
        }
    }

    // 2. Escribir manifiesto journal.dat con estado COMMITTED (1)
    std::vector<uint8_t> journalData;
    // Magic
    journalData.push_back(static_cast<uint8_t>(MAGIC_JOURNAL & 0xFF));
    journalData.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 8) & 0xFF));
    journalData.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 16) & 0xFF));
    journalData.push_back(static_cast<uint8_t>((MAGIC_JOURNAL >> 24) & 0xFF));
    // Status COMMITTED
    journalData.push_back(1);
    // Count
    uint16_t count = static_cast<uint16_t>(m_stagedFiles.size());
    journalData.push_back(static_cast<uint8_t>(count & 0xFF));
    journalData.push_back(static_cast<uint8_t>((count >> 8) & 0xFF));

    for (const auto& staged : m_stagedFiles) {
        std::string relTmp = std::filesystem::relative(staged.tmpPath, m_worldDir).string();
        std::string relFinal = std::filesystem::relative(staged.finalPath, m_worldDir).string();

        uint16_t tmpLen = static_cast<uint16_t>(relTmp.size());
        journalData.push_back(static_cast<uint8_t>(tmpLen & 0xFF));
        journalData.push_back(static_cast<uint8_t>((tmpLen >> 8) & 0xFF));
        for (char c : relTmp) journalData.push_back(static_cast<uint8_t>(c));

        uint16_t finalLen = static_cast<uint16_t>(relFinal.size());
        journalData.push_back(static_cast<uint8_t>(finalLen & 0xFF));
        journalData.push_back(static_cast<uint8_t>((finalLen >> 8) & 0xFF));
        for (char c : relFinal) journalData.push_back(static_cast<uint8_t>(c));
    }

    {
        // Los temporales deben estar en disco antes de que el journal los declare confirmados
        std::set<std::filesystem::path> tmpDirs;
        for (const auto& staged : m_stagedFiles) tmpDirs.insert(staged.tmpPath.parent_path());
        for (const auto& d : tmpDirs) syncDirectory(d);

        StorageError jerr = writeFileDurable(getJournalPath(), journalData);
        if (jerr != StorageError::None) {
            rollbackTransaction();
            return jerr;
        }
        syncDirectory(m_worldDir);
    }

    // 3. Renombrar atómicamente los archivos temporales a su destino final
    std::error_code ec;
    for (const auto& staged : m_stagedFiles) {
        std::filesystem::create_directories(staged.finalPath.parent_path(), ec);
        std::filesystem::rename(staged.tmpPath, staged.finalPath, ec);
        if (ec) {
            Log::error("Fallo al renombrar atómicamente '{}' a '{}': {}", staged.tmpPath.string(),
                       staged.finalPath.string(), ec.message());
            // El journal permitirá completar el proceso en el siguiente arranque
            return StorageError::IoError;
        }
    }

    // 4. Limpiar journal una vez los renombres son duraderos
    {
        std::set<std::filesystem::path> finalDirs;
        for (const auto& staged : m_stagedFiles) finalDirs.insert(staged.finalPath.parent_path());
        for (const auto& d : finalDirs) syncDirectory(d);
    }
    std::filesystem::remove(getJournalPath(), ec);
    syncDirectory(m_worldDir);

    m_stagedFiles.clear();
    m_inTransaction = false;
    return StorageError::None;
}

void WorldStorage::rollbackTransaction() {
    std::error_code ec;
    for (const auto& staged : m_stagedFiles) {
        if (std::filesystem::exists(staged.tmpPath)) {
            std::filesystem::remove(staged.tmpPath, ec);
        }
    }
    if (std::filesystem::exists(getJournalPath())) {
        std::filesystem::remove(getJournalPath(), ec);
    }
    m_stagedFiles.clear();
    m_inTransaction = false;
}

std::vector<uint32_t> WorldStorage::getSavedPlayerIds() const {
    std::vector<uint32_t> ids;
    std::filesystem::path playersDir = m_worldDir / "players";
    std::error_code ec;
    if (!std::filesystem::exists(playersDir, ec) || !std::filesystem::is_directory(playersDir, ec)) {
        return ids;
    }

    for (const auto& entry : std::filesystem::directory_iterator(playersDir, ec)) {
        if (!entry.is_regular_file()) continue;
        auto p = entry.path();
        if (p.extension() != ".dat") continue;
        std::string stem = p.stem().string();
        if (stem == "player") continue; // Jugador local por defecto
        if (stem.rfind("player_", 0) == 0) {
            std::string idStr = stem.substr(7);
            try {
                uint32_t id = static_cast<uint32_t>(std::stoul(idStr));
                ids.push_back(id);
            } catch (...) {}
        } else {
            try {
                uint32_t id = static_cast<uint32_t>(std::stoul(stem));
                ids.push_back(id);
            } catch (...) {}
        }
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

std::vector<std::string> WorldStorage::getSavedPlayerIdentifiers() const {
    std::vector<std::string> identifiers;
    std::filesystem::path playersDir = m_worldDir / "players";
    std::error_code ec;
    if (!std::filesystem::exists(playersDir, ec) || !std::filesystem::is_directory(playersDir, ec)) {
        return identifiers;
    }

    for (const auto& entry : std::filesystem::directory_iterator(playersDir, ec)) {
        if (!entry.is_regular_file()) continue;
        auto p = entry.path();
        if (p.extension() != ".dat") continue;
        identifiers.push_back(p.stem().string());
    }
    std::sort(identifiers.begin(), identifiers.end());
    return identifiers;
}

} // namespace OpenCraft
