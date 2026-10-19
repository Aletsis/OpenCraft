#include "storage/WorldManager.hpp"
#include "core/Log.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif
#include <mutex>
#include <unordered_set>

namespace OpenCraft {

static int getCurrentProcessId() noexcept {
#ifdef _WIN32
    return static_cast<int>(GetCurrentProcessId());
#else
    return static_cast<int>(getpid());
#endif
}

static bool isProcessAlive(int pid) noexcept {
    if (pid <= 0) return false;
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (process == nullptr) return false;
    DWORD exitCode = 0;
    GetExitCodeProcess(process, &exitCode);
    CloseHandle(process);
    return exitCode == STILL_ACTIVE;
#else
    return (kill(pid, 0) == 0);
#endif
}

static int64_t getCurrentTimeSeconds() noexcept {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

WorldManager::WorldManager(std::filesystem::path savesDir, std::filesystem::path backupsDir)
    : m_savesDir(std::move(savesDir)),
      m_backupsDir(std::move(backupsDir)) {
    std::error_code ec;
    std::filesystem::create_directories(m_savesDir, ec);
    std::filesystem::create_directories(m_backupsDir, ec);
}

WorldManager::~WorldManager() {
    for (const auto& lockedId : m_openWorldLocks) {
        releaseLock(m_savesDir / lockedId);
    }
    m_openWorldLocks.clear();
}

std::string WorldManager::sanitizeWorldId(const std::string& name) {
    std::string result;
    result.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        } else if (c == ' ' || c == '-' || c == '_') {
            if (!result.empty() && result.back() != '_') {
                result.push_back('_');
            }
        }
    }
    if (result.empty()) {
        result = "world";
    }
    return result;
}

static std::unordered_set<std::string> s_activeInProcessLocks;
static std::mutex s_activeLocksMutex;

bool WorldManager::acquireLock(const std::filesystem::path& worldDir) {
    std::string worldId = worldDir.filename().string();
    {
        std::lock_guard<std::mutex> lock(s_activeLocksMutex);
        if (s_activeInProcessLocks.contains(worldId)) {
            Log::warn("El mundo '{}' ya está bloqueado en esta sesión", worldDir.string());
            return false;
        }
    }

    std::filesystem::path lockPath = worldDir / "session.lock";
    if (std::filesystem::exists(lockPath)) {
        std::ifstream in(lockPath);
        int storedPid = 0;
        in >> storedPid;
        if (storedPid > 0 && storedPid != getCurrentProcessId() && isProcessAlive(storedPid)) {
            Log::warn("El mundo '{}' está bloqueado por el proceso PID {}", worldDir.string(), storedPid);
            return false;
        }
    }

    std::ofstream out(lockPath, std::ios::trunc);
    if (!out.is_open()) return false;
    out << getCurrentProcessId() << " " << getCurrentTimeSeconds() << "\n";
    out.flush();

    {
        std::lock_guard<std::mutex> lock(s_activeLocksMutex);
        s_activeInProcessLocks.insert(worldId);
    }
    return true;
}

void WorldManager::releaseLock(const std::filesystem::path& worldDir) {
    std::string worldId = worldDir.filename().string();
    {
        std::lock_guard<std::mutex> lock(s_activeLocksMutex);
        s_activeInProcessLocks.erase(worldId);
    }
    std::filesystem::path lockPath = worldDir / "session.lock";
    std::error_code ec;
    std::filesystem::remove(lockPath, ec);
}

bool WorldManager::isWorldLocked(const std::string& worldId) const {
    {
        std::lock_guard<std::mutex> lock(s_activeLocksMutex);
        if (s_activeInProcessLocks.contains(worldId)) {
            return true;
        }
    }

    std::filesystem::path lockPath = m_savesDir / worldId / "session.lock";
    if (!std::filesystem::exists(lockPath)) return false;

    std::ifstream in(lockPath);
    int storedPid = 0;
    in >> storedPid;
    if (storedPid > 0 && isProcessAlive(storedPid)) {
        return true;
    }
    return false;
}

std::vector<WorldSummary> WorldManager::listWorlds() const {
    std::vector<WorldSummary> results;
    std::error_code ec;

    if (!std::filesystem::exists(m_savesDir, ec)) {
        return results;
    }

    for (const auto& entry : std::filesystem::directory_iterator(m_savesDir, ec)) {
        if (!entry.is_directory()) continue;

        std::string worldId = entry.path().filename().string();
        WorldStorage storage(entry.path());
        if (!storage.exists()) continue;

        WorldMetadata meta;
        if (storage.loadMetadata(meta) == StorageError::None) {
            uint64_t totalSize = 0;
            for (const auto& file : std::filesystem::recursive_directory_iterator(entry.path(), ec)) {
                if (file.is_regular_file()) {
                    totalSize += file.file_size(ec);
                }
            }

            results.push_back(WorldSummary{.id = worldId,
                                           .name = meta.worldName.empty() ? worldId : meta.worldName,
                                           .seed = meta.seed,
                                           .schemaVersion = meta.schemaVersion,
                                           .generatorVersion = meta.generatorVersion,
                                           .lastPlayed = meta.lastPlayedTime,
                                           .sizeOnDisk = totalSize,
                                           .gameMode = meta.gameMode,
                                           .hardcore = meta.isHardcore,
                                           .locked = isWorldLocked(worldId)});
        }
    }

    std::sort(results.begin(), results.end(),
              [](const WorldSummary& a, const WorldSummary& b) { return a.lastPlayed > b.lastPlayed; });

    return results;
}

std::string WorldManager::createWorld(const std::string& name, uint64_t seed, uint32_t gameMode, uint32_t schemaVersion,
                                      bool hardcore) {
    std::string baseId = sanitizeWorldId(name);
    std::string worldId = baseId;
    int suffix = 1;

    while (std::filesystem::exists(m_savesDir / worldId)) {
        worldId = baseId + "_" + std::to_string(suffix++);
    }

    std::filesystem::path worldDir = m_savesDir / worldId;
    WorldStorage storage(worldDir);
    storage.init();

    int64_t now = getCurrentTimeSeconds();

    WorldMetadata meta;
    meta.schemaVersion = schemaVersion;
    meta.generatorVersion = CURRENT_GENERATOR_VERSION;
    meta.worldName = name;
    meta.worldId = worldId;
    meta.seed = seed;
    meta.creationTime = now;
    meta.lastPlayedTime = now;
    meta.gameMode = gameMode;
    meta.isHardcore = hardcore;
    meta.difficulty = hardcore ? 3 : 1;
    meta.simulationTick = 0;

    // Poblar catálogo inicial con los bloques registrados en el sistema
    auto& reg = BlockRegistry::global();
    for (size_t i = 0; i < reg.getStateCount(); ++i) {
        meta.blockCatalog.push_back(reg.getNamespacedId(static_cast<BlockStateId>(i)));
    }

    PlayerSaveData player;
    player.schemaVersion = schemaVersion;
    player.position = Math::Vec3(8.0f, 68.0f, 8.0f);
    player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
    player.yaw = 0.0f;
    player.pitch = 0.0f;
    player.isGrounded = true;
    player.health = 20.0f;
    player.hunger = 20.0f;
    player.selectedSlot = 0;
    player.hotbarNames = {"opencraft:grass_block", "opencraft:dirt", "opencraft:stone",
                          "opencraft:cobblestone", "opencraft:wood", "opencraft:planks",
                          "opencraft:leaves",      "opencraft:sand", "opencraft:glass"};

    storage.beginTransaction();
    storage.stageMetadata(meta);
    storage.stagePlayer(player);
    storage.commitTransaction();

    Log::info("Mundo creado exitosamente: '{}' (ID: {}, Semilla: {})", name, worldId, seed);
    return worldId;
}

std::unique_ptr<WorldStorage> WorldManager::openWorld(const std::string& worldId, StorageError& outError) {
    std::filesystem::path worldDir = m_savesDir / worldId;
    if (!std::filesystem::exists(worldDir)) {
        outError = StorageError::FileNotFound;
        return nullptr;
    }

    if (!acquireLock(worldDir)) {
        outError = StorageError::Locked;
        return nullptr;
    }

    auto storage = std::make_unique<WorldStorage>(worldDir);
    outError = storage->init();
    if (outError != StorageError::None) {
        releaseLock(worldDir);
        return nullptr;
    }

    m_openWorldLocks.push_back(worldId);
    outError = StorageError::None;
    return storage;
}

void WorldManager::closeWorld(const std::string& worldId) {
    releaseLock(m_savesDir / worldId);
    std::erase(m_openWorldLocks, worldId);
}

bool WorldManager::copyDirectoryTree(const std::filesystem::path& src, const std::filesystem::path& dst,
                                     std::error_code& ec) {
    // Copia explícita archivo a archivo: std::filesystem::copy recursivo sobre un destino existente falla en
    // algunas implementaciones (libstdc++ de MinGW devuelve «File exists»)
    ec.clear();
    std::filesystem::create_directories(dst, ec);
    if (ec) return false;
    for (auto it = std::filesystem::recursive_directory_iterator(src, ec);
         !ec && it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        std::filesystem::path rel = std::filesystem::relative(it->path(), src, ec);
        if (ec) return false;
        std::filesystem::path target = dst / rel;
        if (it->is_directory(ec)) {
            std::filesystem::create_directories(target, ec);
        } else if (it->is_regular_file(ec)) {
            std::filesystem::copy_file(it->path(), target, std::filesystem::copy_options::overwrite_existing, ec);
        }
        if (ec) return false;
    }
    return !ec;
}

std::filesystem::path WorldManager::backupWorld(const std::string& worldId) {
    std::filesystem::path srcDir = m_savesDir / worldId;
    if (!std::filesystem::exists(srcDir)) {
        return {};
    }

    int64_t timestamp = getCurrentTimeSeconds();
    std::filesystem::path dstDir = m_backupsDir / (worldId + "_backup_" + std::to_string(timestamp));

    std::error_code ec;
    std::filesystem::create_directories(dstDir, ec);

    if (!ec) copyDirectoryTree(srcDir, dstDir, ec);

    // No conservar session.lock dentro de la copia de seguridad (sin pisar el error de la copia)
    std::error_code lockEc;
    std::filesystem::remove(dstDir / "session.lock", lockEc);

    if (ec) {
        Log::error("Fallo al crear copia de seguridad de '{}': {}", worldId, ec.message());
        return {};
    }

    Log::info("Copia de seguridad creada en: '{}'", dstDir.string());
    return dstDir;
}

StorageError WorldManager::migrateWorld(const std::string& worldId, uint32_t targetSchema) {
    std::filesystem::path worldDir = m_savesDir / worldId;
    WorldStorage storage(worldDir);
    if (!storage.exists()) {
        return StorageError::FileNotFound;
    }

    WorldMetadata meta;
    StorageError err = storage.loadMetadata(meta);
    if (err != StorageError::None) {
        return err;
    }

    if (meta.schemaVersion >= targetSchema) {
        // Idempotente: Si ya está en la versión objetivo, no hace nada
        return StorageError::None;
    }

    // 1. Crear respaldo obligatorio antes de modificar los datos
    std::filesystem::path backupPath = backupWorld(worldId);
    if (backupPath.empty()) {
        return StorageError::IoError;
    }

    // 2. Cargar jugador
    PlayerSaveData player;
    storage.loadPlayer(player);

    // 3. Actualizar versiones de esquema
    meta.schemaVersion = targetSchema;
    player.schemaVersion = targetSchema;

    storage.beginTransaction();
    storage.stageMetadata(meta);
    storage.stagePlayer(player);

    // 4. Migrar los chunks de las tres dimensiones. Un chunk ilegible aborta la migración en lugar
    // de omitirse en silencio (se perdería al reescribir el mundo con el esquema nuevo)
    std::error_code ec;
    const std::pair<const char*, DimensionType> dims[] = {
        {"chunks", DimensionType::Overworld}, {"DIM-1", DimensionType::Abyss}, {"DIM1", DimensionType::Rim}};
    size_t migrated = 0;
    for (const auto& [folder, dim] : dims) {
        std::filesystem::path chunksDir = worldDir / folder;
        if (!std::filesystem::exists(chunksDir, ec)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(chunksDir, ec)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".dat") continue;
            std::vector<uint8_t> buffer;
            std::ifstream f(entry.path(), std::ios::binary);
            if (!f) {
                storage.rollbackTransaction();
                Log::error("Migración cancelada: no se pudo leer '{}'", entry.path().string());
                return StorageError::IoError;
            }
            buffer.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
            ChunkSaveData cdata;
            if (WorldFormat::deserializeChunk(buffer, cdata) != SerializationResult::Success) {
                storage.rollbackTransaction();
                Log::error("Migración cancelada: chunk dañado '{}' (el respaldo previo sigue intacto)",
                           entry.path().string());
                return StorageError::CorruptedData;
            }
            cdata.schemaVersion = targetSchema;
            Chunk tempChunk(cdata.chunkX, cdata.chunkZ);
            WorldFormat::applyChunkData(cdata, tempChunk);
            StorageError serr = storage.stageChunk(tempChunk, cdata.mobs, dim); // conservar criaturas guardadas
            if (serr != StorageError::None) {
                storage.rollbackTransaction();
                return serr;
            }
            ++migrated;
        }
    }
    Log::info("Migración: {} chunks reescritos con el esquema v{}", migrated, targetSchema);

    err = storage.commitTransaction();
    if (err == StorageError::None) {
        Log::info("Mundo '{}' migrado exitosamente de versión {} a {}", worldId, meta.schemaVersion, targetSchema);
    }
    return err;
}

bool WorldManager::deleteWorld(const std::string& worldId) {
    std::filesystem::path worldDir = m_savesDir / worldId;
    if (!std::filesystem::exists(worldDir)) return false;

    if (isWorldLocked(worldId)) {
        Log::warn("No se puede eliminar el mundo '{}': está en uso por otra sesión", worldId);
        return false;
    }

    // Copia de seguridad previa: borrar un mundo es irreversible para el jugador
    if (backupWorld(worldId).empty()) {
        Log::warn("No se pudo respaldar el mundo '{}' antes de eliminarlo; se cancela el borrado", worldId);
        return false;
    }

    std::error_code ec;
    std::filesystem::remove_all(worldDir, ec);
    return !ec;
}

} // namespace OpenCraft
