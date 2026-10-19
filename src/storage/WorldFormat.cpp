#include "storage/WorldFormat.hpp"
#include "item/Item.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/FurnaceBlockEntity.hpp"
#include "world/HopperBlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "world/DropperBlockEntity.hpp"
#include "world/CrafterBlockEntity.hpp"
#include "world/TrialSpawnerBlockEntity.hpp"
#include "world/VaultBlockEntity.hpp"
#include "sim/MinecartEntity.hpp"
#include <cstring>
#include <unordered_map>
#include <algorithm>

namespace OpenCraft {

// Tabla precalculada de CRC32 (Polinomio IEEE 802.3 0xEDB88320)
static const std::array<uint32_t, 256>& getCRC32Table() noexcept {
    static const auto table = []() constexpr {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();
    return table;
}

uint32_t computeCRC32(std::span<const uint8_t> data, uint32_t previousCrc) noexcept {
    const auto& table = getCRC32Table();
    uint32_t crc = previousCrc ^ 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

const char* serializationResultToString(SerializationResult res) noexcept {
    switch (res) {
        case SerializationResult::Success: return "Éxito";
        case SerializationResult::BufferTooSmall: return "Búfer demasiado pequeño o truncado";
        case SerializationResult::InvalidMagic: return "Identificador mágico inválido";
        case SerializationResult::UnsupportedVersion: return "Versión de esquema no soportada";
        case SerializationResult::ChecksumMismatch: return "Fallo de suma de comprobación (CRC32 Mismatch)";
        case SerializationResult::CorruptedData: return "Estructura de datos corrupta";
        case SerializationResult::NameTooLong: return "Cadena excede longitud máxima permitida";
        case SerializationResult::PaletteTooLarge: return "Paleta de bloques excede tamaño máximo permitido";
    }
    return "Error desconocido";
}

// ==============================================================================
// Utilidades de lectura y escritura seguras con endianness explícito
// ==============================================================================
namespace {

class BufferWriter {
public:
    explicit BufferWriter(std::vector<uint8_t>& buf) : m_buf(buf) {}

    void writeU8(uint8_t v) { m_buf.push_back(v); }

    void writeU16(uint16_t v) {
        m_buf.push_back(static_cast<uint8_t>(v & 0xFF));
        m_buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }

    void writeU32(uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            m_buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
        }
    }

    void writeU64(uint64_t v) {
        for (int i = 0; i < 8; ++i) {
            m_buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
        }
    }

    void writeI16(int16_t v) {
        uint16_t uv = 0;
        std::memcpy(&uv, &v, sizeof(uv));
        writeU16(uv);
    }

    void writeI32(int32_t v) {
        uint32_t uv = 0;
        std::memcpy(&uv, &v, sizeof(uv));
        writeU32(uv);
    }

    void writeI64(int64_t v) {
        uint64_t uv = 0;
        std::memcpy(&uv, &v, sizeof(uv));
        writeU64(uv);
    }

    void writeFloat(float v) {
        uint32_t uv = 0;
        std::memcpy(&uv, &v, sizeof(uv));
        writeU32(uv);
    }

    void writeString(const std::string& str) {
        uint16_t len = static_cast<uint16_t>(std::min(str.size(), MAX_NAME_LENGTH));
        writeU16(len);
        for (uint16_t i = 0; i < len; ++i) {
            m_buf.push_back(static_cast<uint8_t>(str[i]));
        }
    }

private:
    std::vector<uint8_t>& m_buf;
};

class BufferReader {
public:
    explicit BufferReader(std::span<const uint8_t> buf) : m_buf(buf), m_offset(0) {}

    bool hasRemaining(size_t bytes = 1) const noexcept { return m_offset + bytes <= m_buf.size(); }

    size_t getOffset() const noexcept { return m_offset; }

    bool readU8(uint8_t& out) noexcept {
        if (!hasRemaining(1)) return false;
        out = m_buf[m_offset++];
        return true;
    }

    bool readU16(uint16_t& out) noexcept {
        if (!hasRemaining(2)) return false;
        out = static_cast<uint16_t>(m_buf[m_offset]) | (static_cast<uint16_t>(m_buf[m_offset + 1]) << 8);
        m_offset += 2;
        return true;
    }

    bool readI16(int16_t& out) noexcept {
        uint16_t uv = 0;
        if (!readU16(uv)) return false;
        std::memcpy(&out, &uv, sizeof(out));
        return true;
    }

    bool readU32(uint32_t& out) noexcept {
        if (!hasRemaining(4)) return false;
        out = 0;
        for (int i = 0; i < 4; ++i) {
            out |= (static_cast<uint32_t>(m_buf[m_offset + i]) << (i * 8));
        }
        m_offset += 4;
        return true;
    }

    bool readU64(uint64_t& out) noexcept {
        if (!hasRemaining(8)) return false;
        out = 0;
        for (int i = 0; i < 8; ++i) {
            out |= (static_cast<uint64_t>(m_buf[m_offset + i]) << (i * 8));
        }
        m_offset += 8;
        return true;
    }

    bool readI32(int32_t& out) noexcept {
        uint32_t uv = 0;
        if (!readU32(uv)) return false;
        std::memcpy(&out, &uv, sizeof(out));
        return true;
    }

    bool readI64(int64_t& out) noexcept {
        uint64_t uv = 0;
        if (!readU64(uv)) return false;
        std::memcpy(&out, &uv, sizeof(out));
        return true;
    }

    bool readFloat(float& out) noexcept {
        uint32_t uv = 0;
        if (!readU32(uv)) return false;
        std::memcpy(&out, &uv, sizeof(out));
        return true;
    }

    bool readString(std::string& out) noexcept {
        uint16_t len = 0;
        if (!readU16(len)) return false;
        if (len > MAX_NAME_LENGTH || !hasRemaining(len)) return false;
        if (len == 0) {
            out.clear();
            return true;
        }
        out.assign(reinterpret_cast<const char*>(m_buf.data() + m_offset), len);
        m_offset += len;
        return true;
    }

private:
    std::span<const uint8_t> m_buf;
    size_t m_offset = 0;
};

} // namespace

// ==============================================================================
// Serialización y Deserialización de Metadatos
// ==============================================================================
SerializationResult WorldFormat::serializeMetadata(const WorldMetadata& meta, std::vector<uint8_t>& outBuffer) {
    outBuffer.clear();
    BufferWriter writer(outBuffer);

    // Cabecera: Magic (4 bytes), SchemaVersion (4 bytes), CRC32 Placeholder (4 bytes)
    writer.writeU32(MAGIC_WORLD_META);
    writer.writeU32(meta.schemaVersion);
    writer.writeU32(0); // Posición 8..11

    // Payload de Metadatos
    writer.writeU32(meta.generatorVersion);
    writer.writeString(meta.worldName);
    writer.writeString(meta.worldId);
    writer.writeU64(meta.seed);
    writer.writeI64(meta.creationTime);
    writer.writeI64(meta.lastPlayedTime);
    writer.writeU32(meta.gameMode);
    writer.writeU32(meta.difficulty);
    writer.writeU64(meta.simulationTick);

    // Catálogo de bloques
    uint16_t catSize = static_cast<uint16_t>(std::min(meta.blockCatalog.size(), MAX_PALETTE_SIZE));
    writer.writeU16(catSize);
    for (uint16_t i = 0; i < catSize; ++i) {
        writer.writeString(meta.blockCatalog[i]);
    }

    // Esquema v6: Clima y tiempo atmosférico
    if (meta.schemaVersion >= SCHEMA_VERSION_6) {
        writer.writeU8(meta.weatherState);
        writer.writeU32(meta.weatherDuration);
        writer.writeFloat(meta.rainStrength);
        writer.writeFloat(meta.thunderStrength);
    }

    // Esquema v11: Modo Extremo (Hardcore) y reglas de juego
    if (meta.schemaVersion >= SCHEMA_VERSION_11) {
        writer.writeU8(meta.isHardcore ? 1 : 0);
        uint16_t ruleCount = static_cast<uint16_t>(meta.gameRules.size());
        writer.writeU16(ruleCount);
        for (const auto& r : meta.gameRules) {
            writer.writeString(r.first);
            writer.writeString(r.second);
        }
    }

    // Calcular CRC32 sobre todo el contenido posterior al campo CRC32 (offset 12 hasta fin)
    std::span<const uint8_t> payloadSpan(outBuffer.data() + 12, outBuffer.size() - 12);
    uint32_t crc = computeCRC32(payloadSpan);

    // Escribir CRC en offset 8..11
    outBuffer[8] = static_cast<uint8_t>(crc & 0xFF);
    outBuffer[9] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    outBuffer[10] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    outBuffer[11] = static_cast<uint8_t>((crc >> 24) & 0xFF);

    return SerializationResult::Success;
}

SerializationResult WorldFormat::deserializeMetadata(std::span<const uint8_t> buffer, WorldMetadata& outMeta) {
    if (buffer.size() < 12) {
        return SerializationResult::BufferTooSmall;
    }
    if (buffer.size() > MAX_FILE_SIZE) {
        return SerializationResult::CorruptedData;
    }

    BufferReader reader(buffer);
    uint32_t magic = 0;
    uint32_t schema = 0;
    uint32_t storedCrc = 0;

    if (!reader.readU32(magic) || magic != MAGIC_WORLD_META) {
        return SerializationResult::InvalidMagic;
    }
    if (!reader.readU32(schema) || schema < SCHEMA_VERSION_1 || schema > CURRENT_SCHEMA_VERSION) {
        return SerializationResult::UnsupportedVersion;
    }
    if (!reader.readU32(storedCrc)) {
        return SerializationResult::BufferTooSmall;
    }

    // Verificar CRC32
    std::span<const uint8_t> payloadSpan(buffer.data() + 12, buffer.size() - 12);
    uint32_t calculatedCrc = computeCRC32(payloadSpan);
    if (storedCrc != calculatedCrc) {
        return SerializationResult::ChecksumMismatch;
    }

    outMeta.schemaVersion = schema;
    if (!reader.readU32(outMeta.generatorVersion)) return SerializationResult::CorruptedData;
    if (!reader.readString(outMeta.worldName)) return SerializationResult::CorruptedData;
    if (!reader.readString(outMeta.worldId)) return SerializationResult::CorruptedData;
    if (!reader.readU64(outMeta.seed)) return SerializationResult::CorruptedData;
    if (!reader.readI64(outMeta.creationTime)) return SerializationResult::CorruptedData;
    if (!reader.readI64(outMeta.lastPlayedTime)) return SerializationResult::CorruptedData;
    if (!reader.readU32(outMeta.gameMode)) return SerializationResult::CorruptedData;
    if (!reader.readU32(outMeta.difficulty)) return SerializationResult::CorruptedData;
    if (!reader.readU64(outMeta.simulationTick)) return SerializationResult::CorruptedData;

    uint16_t catSize = 0;
    if (!reader.readU16(catSize)) return SerializationResult::CorruptedData;
    if (catSize > MAX_PALETTE_SIZE) return SerializationResult::PaletteTooLarge;

    outMeta.blockCatalog.clear();
    outMeta.blockCatalog.reserve(catSize);
    for (uint16_t i = 0; i < catSize; ++i) {
        std::string blockName;
        if (!reader.readString(blockName)) return SerializationResult::CorruptedData;
        outMeta.blockCatalog.push_back(std::move(blockName));
    }

    if (schema >= SCHEMA_VERSION_6) {
        if (!reader.readU8(outMeta.weatherState)) return SerializationResult::CorruptedData;
        if (!reader.readU32(outMeta.weatherDuration)) return SerializationResult::CorruptedData;
        if (!reader.readFloat(outMeta.rainStrength)) return SerializationResult::CorruptedData;
        if (!reader.readFloat(outMeta.thunderStrength)) return SerializationResult::CorruptedData;
    } else {
        outMeta.weatherState = 0;
        outMeta.weatherDuration = 0;
        outMeta.rainStrength = 0.0f;
        outMeta.thunderStrength = 0.0f;
    }

    if (schema >= SCHEMA_VERSION_11) {
        uint8_t hc = 0;
        if (!reader.readU8(hc)) return SerializationResult::CorruptedData;
        outMeta.isHardcore = (hc != 0);
        uint16_t ruleCount = 0;
        if (!reader.readU16(ruleCount)) return SerializationResult::CorruptedData;
        outMeta.gameRules.clear();
        outMeta.gameRules.reserve(ruleCount);
        for (uint16_t i = 0; i < ruleCount; ++i) {
            std::string k, v;
            if (!reader.readString(k) || !reader.readString(v)) return SerializationResult::CorruptedData;
            outMeta.gameRules.emplace_back(std::move(k), std::move(v));
        }
    } else {
        outMeta.isHardcore = false;
        outMeta.gameRules.clear();
    }

    return SerializationResult::Success;
}

// ==============================================================================
// Serialización y Deserialización de Jugador
// ==============================================================================
SerializationResult WorldFormat::serializePlayer(const PlayerSaveData& player, std::vector<uint8_t>& outBuffer) {
    outBuffer.clear();
    BufferWriter writer(outBuffer);

    writer.writeU32(MAGIC_PLAYER);
    writer.writeU32(player.schemaVersion);
    writer.writeU32(0); // CRC32 placeholder

    writer.writeFloat(player.position.x);
    writer.writeFloat(player.position.y);
    writer.writeFloat(player.position.z);

    writer.writeFloat(player.velocity.x);
    writer.writeFloat(player.velocity.y);
    writer.writeFloat(player.velocity.z);

    writer.writeFloat(player.yaw);
    writer.writeFloat(player.pitch);
    writer.writeU8(player.isGrounded ? 1 : 0);

    writer.writeFloat(player.health);
    writer.writeFloat(player.hunger);
    writer.writeU32(player.selectedSlot);

    uint16_t hotbarCount = static_cast<uint16_t>(std::min(player.hotbarNames.size(), size_t{36}));
    writer.writeU16(hotbarCount);
    for (uint16_t i = 0; i < hotbarCount; ++i) {
        writer.writeString(player.hotbarNames[i]);
    }

    if (player.schemaVersion >= SCHEMA_VERSION_3) {
        uint16_t invCount = static_cast<uint16_t>(player.inventorySlots.size());
        writer.writeU16(invCount);
        for (const auto& item : player.inventorySlots) {
            writer.writeString(item.itemName);
            writer.writeU16(item.count);
            writer.writeU16(item.durability);
            writer.writeU16(static_cast<uint16_t>(item.components.size()));
            for (uint8_t b : item.components) {
                writer.writeU8(b);
            }
        }

        // Cursor Item
        writer.writeString(player.cursorItem.itemName);
        writer.writeU16(player.cursorItem.count);
        writer.writeU16(player.cursorItem.durability);
        writer.writeU16(static_cast<uint16_t>(player.cursorItem.components.size()));
        for (uint8_t b : player.cursorItem.components) {
            writer.writeU8(b);
        }

        // Offhand Item
        writer.writeString(player.offhandItem.itemName);
        writer.writeU16(player.offhandItem.count);
        writer.writeU16(player.offhandItem.durability);
        writer.writeU16(static_cast<uint16_t>(player.offhandItem.components.size()));
        for (uint8_t b : player.offhandItem.components) {
            writer.writeU8(b);
        }
    }

    if (player.schemaVersion >= SCHEMA_VERSION_5) {
        writer.writeFloat(player.saturation);
        writer.writeFloat(player.exhaustion);
        writer.writeI32(player.air);
        writer.writeU32(player.gameMode);
        writer.writeFloat(player.spawnPoint.x);
        writer.writeFloat(player.spawnPoint.y);
        writer.writeFloat(player.spawnPoint.z);
        writer.writeU8(player.hasCustomSpawnPoint ? 1 : 0);
        writer.writeU8(player.isAlive ? 1 : 0);
    }

    // Esquema v7: Experiencia y Armadura
    if (player.schemaVersion >= SCHEMA_VERSION_7) {
        writer.writeU32(player.xpTotal);
        writer.writeU32(player.xpLevel);
        writer.writeFloat(player.xpProgress);

        uint16_t armorCount = static_cast<uint16_t>(player.armorSlots.size());
        writer.writeU16(armorCount);
        for (const auto& piece : player.armorSlots) {
            writer.writeString(piece.itemName);
            writer.writeU16(piece.count);
            writer.writeU16(piece.durability);
            writer.writeU16(static_cast<uint16_t>(piece.components.size()));
            for (uint8_t b : piece.components) {
                writer.writeU8(b);
            }
        }
    }

    // Esquema v10: Dimensión, planeo y efectos de estado
    if (player.schemaVersion >= SCHEMA_VERSION_10) {
        writer.writeU8(player.currentDimension);
        writer.writeU8(player.isGliding ? 1 : 0);

        uint16_t effectCount = static_cast<uint16_t>(player.activeEffects.size());
        writer.writeU16(effectCount);
        for (const auto& eff : player.activeEffects) {
            writer.writeU8(eff.type);
            writer.writeU32(eff.durationTicks);
            writer.writeU8(eff.amplifier);
            writer.writeU8(eff.isAmbient ? 1 : 0);
        }
    }

    // Esquema v11: Avances y Estadísticas
    if (player.schemaVersion >= SCHEMA_VERSION_11) {
        uint16_t advCount = static_cast<uint16_t>(player.unlockedAdvancements.size());
        writer.writeU16(advCount);
        for (const auto& adv : player.unlockedAdvancements) {
            writer.writeString(adv);
        }

        uint16_t statCount = static_cast<uint16_t>(player.generalStatistics.size());
        writer.writeU16(statCount);
        for (const auto& st : player.generalStatistics) {
            writer.writeString(st.first);
            writer.writeU64(st.second);
        }
    }

    // Esquema v12: Identidad persistente de jugador multijugador
    if (player.schemaVersion >= SCHEMA_VERSION_12) {
        writer.writeU32(player.entityId);
        writer.writeString(player.playerName);
    }

    std::span<const uint8_t> payloadSpan(outBuffer.data() + 12, outBuffer.size() - 12);
    uint32_t crc = computeCRC32(payloadSpan);
    outBuffer[8] = static_cast<uint8_t>(crc & 0xFF);
    outBuffer[9] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    outBuffer[10] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    outBuffer[11] = static_cast<uint8_t>((crc >> 24) & 0xFF);

    return SerializationResult::Success;
}

SerializationResult WorldFormat::deserializePlayer(std::span<const uint8_t> buffer, PlayerSaveData& outPlayer) {
    if (buffer.size() < 12) return SerializationResult::BufferTooSmall;
    if (buffer.size() > MAX_FILE_SIZE) return SerializationResult::CorruptedData;

    BufferReader reader(buffer);
    uint32_t magic = 0, schema = 0, storedCrc = 0;
    if (!reader.readU32(magic) || magic != MAGIC_PLAYER) return SerializationResult::InvalidMagic;
    if (!reader.readU32(schema) || schema < SCHEMA_VERSION_1 || schema > CURRENT_SCHEMA_VERSION) {
        return SerializationResult::UnsupportedVersion;
    }
    if (!reader.readU32(storedCrc)) return SerializationResult::BufferTooSmall;

    std::span<const uint8_t> payloadSpan(buffer.data() + 12, buffer.size() - 12);
    if (computeCRC32(payloadSpan) != storedCrc) return SerializationResult::ChecksumMismatch;

    outPlayer.schemaVersion = schema;
    if (!reader.readFloat(outPlayer.position.x)) return SerializationResult::CorruptedData;
    if (!reader.readFloat(outPlayer.position.y)) return SerializationResult::CorruptedData;
    if (!reader.readFloat(outPlayer.position.z)) return SerializationResult::CorruptedData;

    if (!reader.readFloat(outPlayer.velocity.x)) return SerializationResult::CorruptedData;
    if (!reader.readFloat(outPlayer.velocity.y)) return SerializationResult::CorruptedData;
    if (!reader.readFloat(outPlayer.velocity.z)) return SerializationResult::CorruptedData;

    if (!reader.readFloat(outPlayer.yaw)) return SerializationResult::CorruptedData;
    if (!reader.readFloat(outPlayer.pitch)) return SerializationResult::CorruptedData;

    uint8_t grounded = 0;
    if (!reader.readU8(grounded)) return SerializationResult::CorruptedData;
    outPlayer.isGrounded = (grounded != 0);

    if (!reader.readFloat(outPlayer.health)) return SerializationResult::CorruptedData;
    if (!reader.readFloat(outPlayer.hunger)) return SerializationResult::CorruptedData;
    if (!reader.readU32(outPlayer.selectedSlot)) return SerializationResult::CorruptedData;

    uint16_t hotbarCount = 0;
    if (!reader.readU16(hotbarCount)) return SerializationResult::CorruptedData;
    outPlayer.hotbarNames.clear();
    outPlayer.hotbarNames.reserve(hotbarCount);

    for (uint16_t i = 0; i < hotbarCount; ++i) {
        std::string slotName;
        if (!reader.readString(slotName)) return SerializationResult::CorruptedData;
        outPlayer.hotbarNames.push_back(std::move(slotName));
    }

    if (schema >= SCHEMA_VERSION_3) {
        uint16_t invCount = 0;
        if (!reader.readU16(invCount)) return SerializationResult::CorruptedData;
        outPlayer.inventorySlots.clear();
        outPlayer.inventorySlots.reserve(invCount);
        for (uint16_t i = 0; i < invCount; ++i) {
            ItemStackSaveData item;
            if (!reader.readString(item.itemName)) return SerializationResult::CorruptedData;
            if (!reader.readU16(item.count)) return SerializationResult::CorruptedData;
            if (!reader.readU16(item.durability)) return SerializationResult::CorruptedData;
            uint16_t compSize = 0;
            if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
            item.components.resize(compSize);
            for (uint16_t c = 0; c < compSize; ++c) {
                if (!reader.readU8(item.components[c])) return SerializationResult::CorruptedData;
            }
            outPlayer.inventorySlots.push_back(std::move(item));
        }

        // Cursor
        if (!reader.readString(outPlayer.cursorItem.itemName)) return SerializationResult::CorruptedData;
        if (!reader.readU16(outPlayer.cursorItem.count)) return SerializationResult::CorruptedData;
        if (!reader.readU16(outPlayer.cursorItem.durability)) return SerializationResult::CorruptedData;
        uint16_t curCompSize = 0;
        if (!reader.readU16(curCompSize)) return SerializationResult::CorruptedData;
        outPlayer.cursorItem.components.resize(curCompSize);
        for (uint16_t c = 0; c < curCompSize; ++c) {
            if (!reader.readU8(outPlayer.cursorItem.components[c])) return SerializationResult::CorruptedData;
        }

        // Offhand
        if (!reader.readString(outPlayer.offhandItem.itemName)) return SerializationResult::CorruptedData;
        if (!reader.readU16(outPlayer.offhandItem.count)) return SerializationResult::CorruptedData;
        if (!reader.readU16(outPlayer.offhandItem.durability)) return SerializationResult::CorruptedData;
        uint16_t offCompSize = 0;
        if (!reader.readU16(offCompSize)) return SerializationResult::CorruptedData;
        outPlayer.offhandItem.components.resize(offCompSize);
        for (uint16_t c = 0; c < offCompSize; ++c) {
            if (!reader.readU8(outPlayer.offhandItem.components[c])) return SerializationResult::CorruptedData;
        }
    } else {
        // Esquemas heredados v1 y v2: reconstruir slots desde hotbarNames
        outPlayer.inventorySlots.clear();
        outPlayer.inventorySlots.resize(36);
        for (size_t i = 0; i < outPlayer.hotbarNames.size() && i < 9; ++i) {
            if (!outPlayer.hotbarNames[i].empty() && outPlayer.hotbarNames[i] != "opencraft:air") {
                outPlayer.inventorySlots[i] = ItemStackSaveData{
                    .itemName = outPlayer.hotbarNames[i], .count = 64, .durability = 0, .components = {}};
            }
        }
        outPlayer.cursorItem = ItemStackSaveData{};
        outPlayer.offhandItem = ItemStackSaveData{};
    }

    if (schema >= SCHEMA_VERSION_5) {
        if (!reader.readFloat(outPlayer.saturation)) return SerializationResult::CorruptedData;
        if (!reader.readFloat(outPlayer.exhaustion)) return SerializationResult::CorruptedData;
        if (!reader.readI32(outPlayer.air)) return SerializationResult::CorruptedData;
        if (!reader.readU32(outPlayer.gameMode)) return SerializationResult::CorruptedData;
        if (!reader.readFloat(outPlayer.spawnPoint.x)) return SerializationResult::CorruptedData;
        if (!reader.readFloat(outPlayer.spawnPoint.y)) return SerializationResult::CorruptedData;
        if (!reader.readFloat(outPlayer.spawnPoint.z)) return SerializationResult::CorruptedData;
        uint8_t hasCustom = 0;
        if (!reader.readU8(hasCustom)) return SerializationResult::CorruptedData;
        outPlayer.hasCustomSpawnPoint = (hasCustom != 0);
        uint8_t alive = 1;
        if (!reader.readU8(alive)) return SerializationResult::CorruptedData;
        outPlayer.isAlive = (alive != 0);
    } else {
        outPlayer.saturation = 5.0f;
        outPlayer.exhaustion = 0.0f;
        outPlayer.air = 300;
        outPlayer.gameMode = 0;
        outPlayer.spawnPoint = Math::Vec3(8.0f, 68.0f, 8.0f);
        outPlayer.hasCustomSpawnPoint = false;
        outPlayer.isAlive = true;
    }

    if (schema >= SCHEMA_VERSION_7) {
        if (!reader.readU32(outPlayer.xpTotal)) return SerializationResult::CorruptedData;
        if (!reader.readU32(outPlayer.xpLevel)) return SerializationResult::CorruptedData;
        if (!reader.readFloat(outPlayer.xpProgress)) return SerializationResult::CorruptedData;

        uint16_t armorCount = 0;
        if (!reader.readU16(armorCount)) return SerializationResult::CorruptedData;
        outPlayer.armorSlots.clear();
        outPlayer.armorSlots.reserve(armorCount);
        for (uint16_t a = 0; a < armorCount; ++a) {
            ItemStackSaveData piece;
            if (!reader.readString(piece.itemName)) return SerializationResult::CorruptedData;
            if (!reader.readU16(piece.count)) return SerializationResult::CorruptedData;
            if (!reader.readU16(piece.durability)) return SerializationResult::CorruptedData;
            uint16_t compSize = 0;
            if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
            piece.components.resize(compSize);
            for (uint16_t b = 0; b < compSize; ++b) {
                if (!reader.readU8(piece.components[b])) return SerializationResult::CorruptedData;
            }
            outPlayer.armorSlots.push_back(std::move(piece));
        }
    } else {
        outPlayer.xpTotal = 0;
        outPlayer.xpLevel = 0;
        outPlayer.xpProgress = 0.0f;
        outPlayer.armorSlots.clear();
    }

    if (schema >= SCHEMA_VERSION_10) {
        if (!reader.readU8(outPlayer.currentDimension)) return SerializationResult::CorruptedData;
        uint8_t gliding = 0;
        if (!reader.readU8(gliding)) return SerializationResult::CorruptedData;
        outPlayer.isGliding = (gliding != 0);

        uint16_t effectCount = 0;
        if (!reader.readU16(effectCount)) return SerializationResult::CorruptedData;
        outPlayer.activeEffects.clear();
        outPlayer.activeEffects.reserve(effectCount);
        for (uint16_t e = 0; e < effectCount; ++e) {
            PlayerSaveData::ActiveEffectSaveData eff;
            if (!reader.readU8(eff.type)) return SerializationResult::CorruptedData;
            if (!reader.readU32(eff.durationTicks)) return SerializationResult::CorruptedData;
            if (!reader.readU8(eff.amplifier)) return SerializationResult::CorruptedData;
            uint8_t amb = 0;
            if (!reader.readU8(amb)) return SerializationResult::CorruptedData;
            eff.isAmbient = (amb != 0);
            outPlayer.activeEffects.push_back(eff);
        }
    } else {
        outPlayer.currentDimension = 0;
        outPlayer.isGliding = false;
        outPlayer.activeEffects.clear();
    }

    if (schema >= SCHEMA_VERSION_11) {
        uint16_t advCount = 0;
        if (!reader.readU16(advCount)) return SerializationResult::CorruptedData;
        outPlayer.unlockedAdvancements.clear();
        outPlayer.unlockedAdvancements.reserve(advCount);
        for (uint16_t i = 0; i < advCount; ++i) {
            std::string adv;
            if (!reader.readString(adv)) return SerializationResult::CorruptedData;
            outPlayer.unlockedAdvancements.push_back(std::move(adv));
        }

        uint16_t statCount = 0;
        if (!reader.readU16(statCount)) return SerializationResult::CorruptedData;
        outPlayer.generalStatistics.clear();
        outPlayer.generalStatistics.reserve(statCount);
        for (uint16_t i = 0; i < statCount; ++i) {
            std::string k;
            uint64_t v = 0;
            if (!reader.readString(k) || !reader.readU64(v)) return SerializationResult::CorruptedData;
            outPlayer.generalStatistics.emplace_back(std::move(k), v);
        }
    } else {
        outPlayer.unlockedAdvancements.clear();
        outPlayer.generalStatistics.clear();
    }

    // Esquema v12: Identidad persistente de jugador multijugador
    if (schema >= SCHEMA_VERSION_12 && reader.hasRemaining()) {
        if (!reader.readU32(outPlayer.entityId)) return SerializationResult::CorruptedData;
        if (!reader.readString(outPlayer.playerName)) return SerializationResult::CorruptedData;
    } else {
        outPlayer.entityId = 0;
        outPlayer.playerName.clear();
    }

    return SerializationResult::Success;
}

// ==============================================================================
// Serialización y Deserialización de Chunk
// ==============================================================================
SerializationResult WorldFormat::serializeChunk(const Chunk& chunk, std::vector<uint8_t>& outBuffer,
                                                const std::vector<MobSaveData>& mobs) {
    ChunkSaveData data;
    extractChunkData(chunk, data);
    data.mobs = mobs;
    return serializeChunkData(data, outBuffer);
}

SerializationResult WorldFormat::serializeChunkData(const ChunkSaveData& data, std::vector<uint8_t>& outBuffer) {
    outBuffer.clear();
    BufferWriter writer(outBuffer);

    writer.writeU32(MAGIC_CHUNK);
    writer.writeU32(data.schemaVersion);
    writer.writeU32(0); // CRC32 placeholder

    writer.writeI32(data.chunkX);
    writer.writeI32(data.chunkZ);

    if (data.schemaVersion == SCHEMA_VERSION_1) {
        // Formato Legacy v1
        uint16_t palSize = static_cast<uint16_t>(data.palette.size());
        writer.writeU16(palSize);
        for (const auto& name : data.palette) {
            writer.writeString(name);
        }
        writer.writeU32(static_cast<uint32_t>(data.blockIndices.size()));
        for (uint16_t idx : data.blockIndices) {
            writer.writeU16(idx);
        }
    } else {
        uint16_t sectionCount = static_cast<uint16_t>(data.sections.size());
        writer.writeU16(sectionCount);
        writer.writeI16(static_cast<int16_t>(Chunk::MIN_SECTION_Y));

        for (const auto& sec : data.sections) {
            if (sec.isEmpty) {
                writer.writeU8(1);
            } else {
                writer.writeU8(0);
                uint16_t palCount = static_cast<uint16_t>(sec.palette.size());
                writer.writeU16(palCount);
                for (const auto& name : sec.palette) {
                    writer.writeString(name);
                }
                writer.writeU32(static_cast<uint32_t>(sec.blockIndices.size()));
                for (uint16_t idx : sec.blockIndices) {
                    writer.writeU16(idx);
                }

                // Esquema v6: 4096 bytes de luz (sky y block light)
                if (data.schemaVersion >= SCHEMA_VERSION_6) {
                    if (sec.light.size() == ChunkSection::BLOCKS_PER_SECTION) {
                        writer.writeU8(1); // hasLight = 1
                        for (uint8_t l : sec.light) {
                            writer.writeU8(l);
                        }
                    } else {
                        writer.writeU8(0); // hasLight = 0
                    }
                }
            }
        }
    }

    // Esquema v3: Serializar cofres
    if (data.schemaVersion >= SCHEMA_VERSION_3) {
        uint16_t chestCount = static_cast<uint16_t>(data.chests.size());
        writer.writeU16(chestCount);
        for (const auto& chest : data.chests) {
            writer.writeI32(chest.x);
            writer.writeI32(chest.y);
            writer.writeI32(chest.z);
            writer.writeU32(chest.revision);
            uint16_t slotCount = static_cast<uint16_t>(chest.slots.size());
            writer.writeU16(slotCount);
            for (const auto& slot : chest.slots) {
                writer.writeString(slot.itemName);
                writer.writeU16(slot.count);
                writer.writeU16(slot.durability);
                writer.writeU16(static_cast<uint16_t>(slot.components.size()));
                for (uint8_t b : slot.components) {
                    writer.writeU8(b);
                }
            }
        }
    }

    // Esquema v4: Serializar hornos
    if (data.schemaVersion >= SCHEMA_VERSION_4) {
        uint16_t furnaceCount = static_cast<uint16_t>(data.furnaces.size());
        writer.writeU16(furnaceCount);
        for (const auto& furnace : data.furnaces) {
            writer.writeI32(furnace.x);
            writer.writeI32(furnace.y);
            writer.writeI32(furnace.z);
            writer.writeU32(furnace.revision);
            writer.writeU32(furnace.litTime);
            writer.writeU32(furnace.litDuration);
            writer.writeU32(furnace.cookProgress);
            writer.writeU32(furnace.cookTotal);
            uint16_t slotCount = static_cast<uint16_t>(furnace.slots.size());
            writer.writeU16(slotCount);
            for (const auto& slot : furnace.slots) {
                writer.writeString(slot.itemName);
                writer.writeU16(slot.count);
                writer.writeU16(slot.durability);
                writer.writeU16(static_cast<uint16_t>(slot.components.size()));
                for (uint8_t b : slot.components) {
                    writer.writeU8(b);
                }
            }
        }
    }

    // Esquema v6: Serializar actualizaciones de bloque programadas
    if (data.schemaVersion >= SCHEMA_VERSION_6) {
        uint16_t updateCount = static_cast<uint16_t>(data.scheduledUpdates.size());
        writer.writeU16(updateCount);
        for (const auto& su : data.scheduledUpdates) {
            writer.writeI32(su.x);
            writer.writeI32(su.y);
            writer.writeI32(su.z);
            writer.writeString(su.blockName);
            writer.writeU64(su.targetTick);
            writer.writeI32(su.priority);
        }
    }

    // Esquema v7 y v8: Serializar criaturas en el chunk
    if (data.schemaVersion >= SCHEMA_VERSION_7) {
        uint16_t mobCount = static_cast<uint16_t>(data.mobs.size());
        writer.writeU16(mobCount);
        for (const auto& mob : data.mobs) {
            writer.writeString(mob.mobType);
            writer.writeFloat(mob.position.x);
            writer.writeFloat(mob.position.y);
            writer.writeFloat(mob.position.z);
            writer.writeFloat(mob.velocity.x);
            writer.writeFloat(mob.velocity.y);
            writer.writeFloat(mob.velocity.z);
            writer.writeFloat(mob.yaw);
            writer.writeFloat(mob.pitch);
            writer.writeFloat(mob.health);
            writer.writeU8(mob.aiState);
            writer.writeU8(mob.isPersistent ? 1 : 0);

            if (data.schemaVersion >= SCHEMA_VERSION_8) {
                writer.writeU8(mob.inLove ? 1 : 0);
                writer.writeU32(mob.breedingCooldown);
                writer.writeU8(mob.isBaby ? 1 : 0);
                writer.writeU8(mob.isTamed ? 1 : 0);
                writer.writeI64(mob.ownerId);
                writer.writeU8(mob.isSitting ? 1 : 0);
                writer.writeU8(mob.villagerProfession);
            }
        }
    }

    // Esquema v9: Serializar tolvas, soltadores, dispensadores, crafters y vagonetas
    if (data.schemaVersion >= SCHEMA_VERSION_9) {
        // Hoppers
        uint16_t hopperCount = static_cast<uint16_t>(data.hoppers.size());
        writer.writeU16(hopperCount);
        for (const auto& hopper : data.hoppers) {
            writer.writeI32(hopper.x);
            writer.writeI32(hopper.y);
            writer.writeI32(hopper.z);
            writer.writeU32(hopper.revision);
            writer.writeU8(hopper.facing);
            writer.writeU32(hopper.cooldown);
            writer.writeU8(hopper.isLocked ? 1 : 0);
            writer.writeU16(static_cast<uint16_t>(hopper.slots.size()));
            for (const auto& slot : hopper.slots) {
                writer.writeString(slot.itemName);
                writer.writeU16(slot.count);
                writer.writeU16(slot.durability);
                writer.writeU16(static_cast<uint16_t>(slot.components.size()));
                for (uint8_t b : slot.components) writer.writeU8(b);
            }
        }

        // Droppers
        uint16_t dropperCount = static_cast<uint16_t>(data.droppers.size());
        writer.writeU16(dropperCount);
        for (const auto& dropper : data.droppers) {
            writer.writeI32(dropper.x);
            writer.writeI32(dropper.y);
            writer.writeI32(dropper.z);
            writer.writeU32(dropper.revision);
            writer.writeU8(dropper.facing);
            writer.writeU16(static_cast<uint16_t>(dropper.slots.size()));
            for (const auto& slot : dropper.slots) {
                writer.writeString(slot.itemName);
                writer.writeU16(slot.count);
                writer.writeU16(slot.durability);
                writer.writeU16(static_cast<uint16_t>(slot.components.size()));
                for (uint8_t b : slot.components) writer.writeU8(b);
            }
        }

        // Dispensers
        uint16_t dispenserCount = static_cast<uint16_t>(data.dispensers.size());
        writer.writeU16(dispenserCount);
        for (const auto& dispenser : data.dispensers) {
            writer.writeI32(dispenser.x);
            writer.writeI32(dispenser.y);
            writer.writeI32(dispenser.z);
            writer.writeU32(dispenser.revision);
            writer.writeU8(dispenser.facing);
            writer.writeU16(static_cast<uint16_t>(dispenser.slots.size()));
            for (const auto& slot : dispenser.slots) {
                writer.writeString(slot.itemName);
                writer.writeU16(slot.count);
                writer.writeU16(slot.durability);
                writer.writeU16(static_cast<uint16_t>(slot.components.size()));
                for (uint8_t b : slot.components) writer.writeU8(b);
            }
        }

        // Crafters
        uint16_t crafterCount = static_cast<uint16_t>(data.crafters.size());
        writer.writeU16(crafterCount);
        for (const auto& crafter : data.crafters) {
            writer.writeI32(crafter.x);
            writer.writeI32(crafter.y);
            writer.writeI32(crafter.z);
            writer.writeU32(crafter.revision);
            writer.writeU8(crafter.facing);
            writer.writeU16(crafter.disabledSlotsMask);
            writer.writeU8(crafter.triggered ? 1 : 0);
            writer.writeU16(static_cast<uint16_t>(crafter.slots.size()));
            for (const auto& slot : crafter.slots) {
                writer.writeString(slot.itemName);
                writer.writeU16(slot.count);
                writer.writeU16(slot.durability);
                writer.writeU16(static_cast<uint16_t>(slot.components.size()));
                for (uint8_t b : slot.components) writer.writeU8(b);
            }
        }

        // Minecarts
        uint16_t minecartCount = static_cast<uint16_t>(data.minecarts.size());
        writer.writeU16(minecartCount);
        for (const auto& cart : data.minecarts) {
            writer.writeU8(cart.minecartType);
            writer.writeFloat(cart.position.x);
            writer.writeFloat(cart.position.y);
            writer.writeFloat(cart.position.z);
            writer.writeFloat(cart.velocity.x);
            writer.writeFloat(cart.velocity.y);
            writer.writeFloat(cart.velocity.z);
            writer.writeU16(static_cast<uint16_t>(cart.slots.size()));
            for (const auto& slot : cart.slots) {
                writer.writeString(slot.itemName);
                writer.writeU16(slot.count);
                writer.writeU16(slot.durability);
                writer.writeU16(static_cast<uint16_t>(slot.components.size()));
                for (uint8_t b : slot.components) writer.writeU8(b);
            }
        }
    }

    // Esquema v12: Serializar bloques-entidad 1.21 (cámaras de desafío)
    if (data.schemaVersion >= SCHEMA_VERSION_12) {
        // Trial Spawners
        uint16_t tsCount = static_cast<uint16_t>(data.trialSpawners.size());
        writer.writeU16(tsCount);
        for (const auto& ts : data.trialSpawners) {
            writer.writeI32(ts.x);
            writer.writeI32(ts.y);
            writer.writeI32(ts.z);
            writer.writeU8(ts.state);
            writer.writeU8(ts.isOminous ? 1 : 0);
            writer.writeU32(ts.currentWave);
            writer.writeU32(ts.totalWaves);
            writer.writeU32(ts.cooldownTicks);
        }

        // Vaults
        uint16_t vaultCount = static_cast<uint16_t>(data.vaults.size());
        writer.writeU16(vaultCount);
        for (const auto& v : data.vaults) {
            writer.writeI32(v.x);
            writer.writeI32(v.y);
            writer.writeI32(v.z);
            writer.writeU8(v.isOminous ? 1 : 0);
            uint16_t pCount = static_cast<uint16_t>(v.unlockedPlayers.size());
            writer.writeU16(pCount);
            for (uint32_t pId : v.unlockedPlayers) {
                writer.writeU32(pId);
            }
        }
    }

    std::span<const uint8_t> payloadSpan(outBuffer.data() + 12, outBuffer.size() - 12);
    uint32_t crc = computeCRC32(payloadSpan);
    outBuffer[8] = static_cast<uint8_t>(crc & 0xFF);
    outBuffer[9] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    outBuffer[10] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    outBuffer[11] = static_cast<uint8_t>((crc >> 24) & 0xFF);

    return SerializationResult::Success;
}

SerializationResult WorldFormat::deserializeChunk(std::span<const uint8_t> buffer, ChunkSaveData& outChunkData) {
    if (buffer.size() < 12) return SerializationResult::BufferTooSmall;
    if (buffer.size() > MAX_CHUNK_DATA_SIZE) return SerializationResult::CorruptedData;

    BufferReader reader(buffer);
    uint32_t magic = 0, schema = 0, storedCrc = 0;
    if (!reader.readU32(magic) || magic != MAGIC_CHUNK) return SerializationResult::InvalidMagic;
    if (!reader.readU32(schema) || schema < SCHEMA_VERSION_1 || schema > CURRENT_SCHEMA_VERSION) {
        return SerializationResult::UnsupportedVersion;
    }
    if (!reader.readU32(storedCrc)) return SerializationResult::BufferTooSmall;

    std::span<const uint8_t> payloadSpan(buffer.data() + 12, buffer.size() - 12);
    if (computeCRC32(payloadSpan) != storedCrc) return SerializationResult::ChecksumMismatch;

    outChunkData.schemaVersion = schema;
    if (!reader.readI32(outChunkData.chunkX)) return SerializationResult::CorruptedData;
    if (!reader.readI32(outChunkData.chunkZ)) return SerializationResult::CorruptedData;

    if (schema == SCHEMA_VERSION_1) {
        // Formato Legacy v1 (16x64x16)
        uint16_t palSize = 0;
        if (!reader.readU16(palSize)) return SerializationResult::CorruptedData;
        if (palSize > MAX_PALETTE_SIZE) return SerializationResult::PaletteTooLarge;

        outChunkData.palette.clear();
        outChunkData.palette.reserve(palSize);
        for (uint16_t i = 0; i < palSize; ++i) {
            std::string blockName;
            if (!reader.readString(blockName)) return SerializationResult::CorruptedData;
            outChunkData.palette.push_back(std::move(blockName));
        }

        uint32_t blockCount = 0;
        if (!reader.readU32(blockCount) || blockCount != Chunk::LEGACY_BLOCK_COUNT) {
            return SerializationResult::CorruptedData;
        }

        outChunkData.blockIndices.resize(blockCount);
        for (size_t i = 0; i < blockCount; ++i) {
            uint16_t idx = 0;
            if (!reader.readU16(idx)) return SerializationResult::CorruptedData;
            if (idx >= palSize && palSize > 0) idx = 0;
            outChunkData.blockIndices[i] = idx;
        }
        return SerializationResult::Success;
    }

    // Formato v2: Secciones de 16³
    uint16_t secCount = 0;
    if (!reader.readU16(secCount)) return SerializationResult::CorruptedData;
    int16_t minSecY = 0;
    if (!reader.readI16(minSecY)) return SerializationResult::CorruptedData;

    outChunkData.sections.clear();
    outChunkData.sections.resize(secCount);

    for (uint16_t i = 0; i < secCount; ++i) {
        auto& sec = outChunkData.sections[i];
        sec.sectionY = minSecY + i;
        uint8_t isEmpty = 0;
        if (!reader.readU8(isEmpty)) return SerializationResult::CorruptedData;

        if (isEmpty) {
            sec.isEmpty = true;
        } else {
            sec.isEmpty = false;
            uint16_t palSize = 0;
            if (!reader.readU16(palSize)) return SerializationResult::CorruptedData;
            if (palSize > MAX_PALETTE_SIZE) return SerializationResult::PaletteTooLarge;

            sec.palette.clear();
            sec.palette.reserve(palSize);
            for (uint16_t p = 0; p < palSize; ++p) {
                std::string blockName;
                if (!reader.readString(blockName)) return SerializationResult::CorruptedData;
                sec.palette.push_back(std::move(blockName));
            }

            uint32_t bCount = 0;
            if (!reader.readU32(bCount) || bCount != ChunkSection::BLOCKS_PER_SECTION) {
                return SerializationResult::CorruptedData;
            }

            sec.blockIndices.resize(bCount);
            for (size_t b = 0; b < bCount; ++b) {
                uint16_t idx = 0;
                if (!reader.readU16(idx)) return SerializationResult::CorruptedData;
                if (idx >= palSize && palSize > 0) idx = 0;
                sec.blockIndices[b] = idx;
            }

            // Esquema v6: Deserializar 4096 bytes de luz si están presentes
            if (schema >= SCHEMA_VERSION_6) {
                uint8_t hasLight = 0;
                if (!reader.readU8(hasLight)) return SerializationResult::CorruptedData;
                if (hasLight) {
                    sec.light.resize(ChunkSection::BLOCKS_PER_SECTION);
                    for (size_t b = 0; b < ChunkSection::BLOCKS_PER_SECTION; ++b) {
                        if (!reader.readU8(sec.light[b])) return SerializationResult::CorruptedData;
                    }
                } else {
                    sec.light.clear();
                }
            }
        }
    }

    // Esquema v3: Deserializar cofres
    if (schema >= SCHEMA_VERSION_3) {
        uint16_t chestCount = 0;
        if (!reader.readU16(chestCount)) return SerializationResult::CorruptedData;
        outChunkData.chests.clear();
        outChunkData.chests.reserve(chestCount);
        for (uint16_t c = 0; c < chestCount; ++c) {
            ChestSaveData chest;
            if (!reader.readI32(chest.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(chest.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(chest.z)) return SerializationResult::CorruptedData;
            if (!reader.readU32(chest.revision)) return SerializationResult::CorruptedData;
            uint16_t slotCount = 0;
            if (!reader.readU16(slotCount)) return SerializationResult::CorruptedData;
            chest.slots.resize(slotCount);
            for (uint16_t s = 0; s < slotCount; ++s) {
                if (!reader.readString(chest.slots[s].itemName)) return SerializationResult::CorruptedData;
                if (!reader.readU16(chest.slots[s].count)) return SerializationResult::CorruptedData;
                if (!reader.readU16(chest.slots[s].durability)) return SerializationResult::CorruptedData;
                uint16_t compSize = 0;
                if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
                chest.slots[s].components.resize(compSize);
                for (uint16_t b = 0; b < compSize; ++b) {
                    if (!reader.readU8(chest.slots[s].components[b])) return SerializationResult::CorruptedData;
                }
            }
            outChunkData.chests.push_back(std::move(chest));
        }
    }

    // Esquema v4: Deserializar hornos
    if (schema >= SCHEMA_VERSION_4) {
        uint16_t furnaceCount = 0;
        if (!reader.readU16(furnaceCount)) return SerializationResult::CorruptedData;
        outChunkData.furnaces.clear();
        outChunkData.furnaces.reserve(furnaceCount);
        for (uint16_t f = 0; f < furnaceCount; ++f) {
            FurnaceSaveData furnace;
            if (!reader.readI32(furnace.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(furnace.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(furnace.z)) return SerializationResult::CorruptedData;
            if (!reader.readU32(furnace.revision)) return SerializationResult::CorruptedData;
            if (!reader.readU32(furnace.litTime)) return SerializationResult::CorruptedData;
            if (!reader.readU32(furnace.litDuration)) return SerializationResult::CorruptedData;
            if (!reader.readU32(furnace.cookProgress)) return SerializationResult::CorruptedData;
            if (!reader.readU32(furnace.cookTotal)) return SerializationResult::CorruptedData;
            uint16_t slotCount = 0;
            if (!reader.readU16(slotCount)) return SerializationResult::CorruptedData;
            furnace.slots.resize(slotCount);
            for (uint16_t s = 0; s < slotCount; ++s) {
                if (!reader.readString(furnace.slots[s].itemName)) return SerializationResult::CorruptedData;
                if (!reader.readU16(furnace.slots[s].count)) return SerializationResult::CorruptedData;
                if (!reader.readU16(furnace.slots[s].durability)) return SerializationResult::CorruptedData;
                uint16_t compSize = 0;
                if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
                furnace.slots[s].components.resize(compSize);
                for (uint16_t b = 0; b < compSize; ++b) {
                    if (!reader.readU8(furnace.slots[s].components[b])) return SerializationResult::CorruptedData;
                }
            }
            outChunkData.furnaces.push_back(std::move(furnace));
        }
    }

    // Esquema v6: Deserializar actualizaciones de bloque programadas
    if (schema >= SCHEMA_VERSION_6) {
        uint16_t updateCount = 0;
        if (!reader.readU16(updateCount)) return SerializationResult::CorruptedData;
        outChunkData.scheduledUpdates.clear();
        outChunkData.scheduledUpdates.reserve(updateCount);
        for (uint16_t u = 0; u < updateCount; ++u) {
            ScheduledUpdateSaveData su;
            if (!reader.readI32(su.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(su.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(su.z)) return SerializationResult::CorruptedData;
            if (!reader.readString(su.blockName)) return SerializationResult::CorruptedData;
            if (!reader.readU64(su.targetTick)) return SerializationResult::CorruptedData;
            if (!reader.readI32(su.priority)) return SerializationResult::CorruptedData;
            outChunkData.scheduledUpdates.push_back(std::move(su));
        }
    }

    // Esquema v7: Deserializar criaturas en el chunk
    if (schema >= SCHEMA_VERSION_7) {
        uint16_t mobCount = 0;
        if (!reader.readU16(mobCount)) return SerializationResult::CorruptedData;
        outChunkData.mobs.clear();
        outChunkData.mobs.reserve(mobCount);
        for (uint16_t m = 0; m < mobCount; ++m) {
            MobSaveData mob;
            if (!reader.readString(mob.mobType)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.position.x)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.position.y)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.position.z)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.velocity.x)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.velocity.y)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.velocity.z)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.yaw)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.pitch)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(mob.health)) return SerializationResult::CorruptedData;
            if (!reader.readU8(mob.aiState)) return SerializationResult::CorruptedData;
            uint8_t persistent = 0;
            if (!reader.readU8(persistent)) return SerializationResult::CorruptedData;
            mob.isPersistent = (persistent != 0);

            if (schema >= SCHEMA_VERSION_8) {
                uint8_t inLove = 0;
                if (!reader.readU8(inLove)) return SerializationResult::CorruptedData;
                mob.inLove = (inLove != 0);
                if (!reader.readU32(mob.breedingCooldown)) return SerializationResult::CorruptedData;
                uint8_t isBaby = 0;
                if (!reader.readU8(isBaby)) return SerializationResult::CorruptedData;
                mob.isBaby = (isBaby != 0);
                uint8_t isTamed = 0;
                if (!reader.readU8(isTamed)) return SerializationResult::CorruptedData;
                mob.isTamed = (isTamed != 0);
                if (!reader.readI64(mob.ownerId)) return SerializationResult::CorruptedData;
                uint8_t isSitting = 0;
                if (!reader.readU8(isSitting)) return SerializationResult::CorruptedData;
                mob.isSitting = (isSitting != 0);
                if (!reader.readU8(mob.villagerProfession)) return SerializationResult::CorruptedData;
            }

            outChunkData.mobs.push_back(std::move(mob));
        }
    } else {
        outChunkData.mobs.clear();
    }

    // Esquema v9: Deserializar tolvas, soltadores, dispensadores, crafters y vagonetas
    if (schema >= SCHEMA_VERSION_9) {
        // Hoppers
        uint16_t hopperCount = 0;
        if (!reader.readU16(hopperCount)) return SerializationResult::CorruptedData;
        outChunkData.hoppers.clear();
        outChunkData.hoppers.reserve(hopperCount);
        for (uint16_t i = 0; i < hopperCount; ++i) {
            HopperSaveData hopper;
            if (!reader.readI32(hopper.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(hopper.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(hopper.z)) return SerializationResult::CorruptedData;
            if (!reader.readU32(hopper.revision)) return SerializationResult::CorruptedData;
            if (!reader.readU8(hopper.facing)) return SerializationResult::CorruptedData;
            if (!reader.readU32(hopper.cooldown)) return SerializationResult::CorruptedData;
            uint8_t locked = 0;
            if (!reader.readU8(locked)) return SerializationResult::CorruptedData;
            hopper.isLocked = (locked != 0);
            uint16_t slotCount = 0;
            if (!reader.readU16(slotCount)) return SerializationResult::CorruptedData;
            hopper.slots.reserve(slotCount);
            for (uint16_t s = 0; s < slotCount; ++s) {
                ItemStackSaveData slot;
                if (!reader.readString(slot.itemName)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.count)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.durability)) return SerializationResult::CorruptedData;
                uint16_t compSize = 0;
                if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
                slot.components.resize(compSize);
                for (uint16_t c = 0; c < compSize; ++c) {
                    if (!reader.readU8(slot.components[c])) return SerializationResult::CorruptedData;
                }
                hopper.slots.push_back(std::move(slot));
            }
            outChunkData.hoppers.push_back(std::move(hopper));
        }

        // Droppers
        uint16_t dropperCount = 0;
        if (!reader.readU16(dropperCount)) return SerializationResult::CorruptedData;
        outChunkData.droppers.clear();
        outChunkData.droppers.reserve(dropperCount);
        for (uint16_t i = 0; i < dropperCount; ++i) {
            DropperSaveData dropper;
            if (!reader.readI32(dropper.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(dropper.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(dropper.z)) return SerializationResult::CorruptedData;
            if (!reader.readU32(dropper.revision)) return SerializationResult::CorruptedData;
            if (!reader.readU8(dropper.facing)) return SerializationResult::CorruptedData;
            uint16_t slotCount = 0;
            if (!reader.readU16(slotCount)) return SerializationResult::CorruptedData;
            dropper.slots.reserve(slotCount);
            for (uint16_t s = 0; s < slotCount; ++s) {
                ItemStackSaveData slot;
                if (!reader.readString(slot.itemName)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.count)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.durability)) return SerializationResult::CorruptedData;
                uint16_t compSize = 0;
                if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
                slot.components.resize(compSize);
                for (uint16_t c = 0; c < compSize; ++c) {
                    if (!reader.readU8(slot.components[c])) return SerializationResult::CorruptedData;
                }
                dropper.slots.push_back(std::move(slot));
            }
            outChunkData.droppers.push_back(std::move(dropper));
        }

        // Dispensers
        uint16_t dispenserCount = 0;
        if (!reader.readU16(dispenserCount)) return SerializationResult::CorruptedData;
        outChunkData.dispensers.clear();
        outChunkData.dispensers.reserve(dispenserCount);
        for (uint16_t i = 0; i < dispenserCount; ++i) {
            DispenserSaveData dispenser;
            if (!reader.readI32(dispenser.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(dispenser.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(dispenser.z)) return SerializationResult::CorruptedData;
            if (!reader.readU32(dispenser.revision)) return SerializationResult::CorruptedData;
            if (!reader.readU8(dispenser.facing)) return SerializationResult::CorruptedData;
            uint16_t slotCount = 0;
            if (!reader.readU16(slotCount)) return SerializationResult::CorruptedData;
            dispenser.slots.reserve(slotCount);
            for (uint16_t s = 0; s < slotCount; ++s) {
                ItemStackSaveData slot;
                if (!reader.readString(slot.itemName)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.count)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.durability)) return SerializationResult::CorruptedData;
                uint16_t compSize = 0;
                if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
                slot.components.resize(compSize);
                for (uint16_t c = 0; c < compSize; ++c) {
                    if (!reader.readU8(slot.components[c])) return SerializationResult::CorruptedData;
                }
                dispenser.slots.push_back(std::move(slot));
            }
            outChunkData.dispensers.push_back(std::move(dispenser));
        }

        // Crafters
        uint16_t crafterCount = 0;
        if (!reader.readU16(crafterCount)) return SerializationResult::CorruptedData;
        outChunkData.crafters.clear();
        outChunkData.crafters.reserve(crafterCount);
        for (uint16_t i = 0; i < crafterCount; ++i) {
            CrafterSaveData crafter;
            if (!reader.readI32(crafter.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(crafter.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(crafter.z)) return SerializationResult::CorruptedData;
            if (!reader.readU32(crafter.revision)) return SerializationResult::CorruptedData;
            if (!reader.readU8(crafter.facing)) return SerializationResult::CorruptedData;
            if (!reader.readU16(crafter.disabledSlotsMask)) return SerializationResult::CorruptedData;
            uint8_t trig = 0;
            if (!reader.readU8(trig)) return SerializationResult::CorruptedData;
            crafter.triggered = (trig != 0);
            uint16_t slotCount = 0;
            if (!reader.readU16(slotCount)) return SerializationResult::CorruptedData;
            crafter.slots.reserve(slotCount);
            for (uint16_t s = 0; s < slotCount; ++s) {
                ItemStackSaveData slot;
                if (!reader.readString(slot.itemName)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.count)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.durability)) return SerializationResult::CorruptedData;
                uint16_t compSize = 0;
                if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
                slot.components.resize(compSize);
                for (uint16_t c = 0; c < compSize; ++c) {
                    if (!reader.readU8(slot.components[c])) return SerializationResult::CorruptedData;
                }
                crafter.slots.push_back(std::move(slot));
            }
            outChunkData.crafters.push_back(std::move(crafter));
        }

        // Minecarts
        uint16_t minecartCount = 0;
        if (!reader.readU16(minecartCount)) return SerializationResult::CorruptedData;
        outChunkData.minecarts.clear();
        outChunkData.minecarts.reserve(minecartCount);
        for (uint16_t i = 0; i < minecartCount; ++i) {
            MinecartSaveData cart;
            if (!reader.readU8(cart.minecartType)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(cart.position.x)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(cart.position.y)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(cart.position.z)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(cart.velocity.x)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(cart.velocity.y)) return SerializationResult::CorruptedData;
            if (!reader.readFloat(cart.velocity.z)) return SerializationResult::CorruptedData;
            uint16_t slotCount = 0;
            if (!reader.readU16(slotCount)) return SerializationResult::CorruptedData;
            cart.slots.reserve(slotCount);
            for (uint16_t s = 0; s < slotCount; ++s) {
                ItemStackSaveData slot;
                if (!reader.readString(slot.itemName)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.count)) return SerializationResult::CorruptedData;
                if (!reader.readU16(slot.durability)) return SerializationResult::CorruptedData;
                uint16_t compSize = 0;
                if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
                slot.components.resize(compSize);
                for (uint16_t c = 0; c < compSize; ++c) {
                    if (!reader.readU8(slot.components[c])) return SerializationResult::CorruptedData;
                }
                cart.slots.push_back(std::move(slot));
            }
            outChunkData.minecarts.push_back(std::move(cart));
        }
    } else {
        outChunkData.hoppers.clear();
        outChunkData.droppers.clear();
        outChunkData.dispensers.clear();
        outChunkData.crafters.clear();
        outChunkData.minecarts.clear();
    }

    // Esquema v12: Deserializar bloques-entidad 1.21 (cámaras de desafío)
    if (schema >= SCHEMA_VERSION_12) {
        uint16_t tsCount = 0;
        if (!reader.readU16(tsCount)) return SerializationResult::CorruptedData;
        outChunkData.trialSpawners.clear();
        outChunkData.trialSpawners.reserve(tsCount);
        for (uint16_t i = 0; i < tsCount; ++i) {
            TrialSpawnerSaveData ts;
            if (!reader.readI32(ts.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(ts.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(ts.z)) return SerializationResult::CorruptedData;
            if (!reader.readU8(ts.state)) return SerializationResult::CorruptedData;
            uint8_t omin = 0;
            if (!reader.readU8(omin)) return SerializationResult::CorruptedData;
            ts.isOminous = (omin != 0);
            if (!reader.readU32(ts.currentWave)) return SerializationResult::CorruptedData;
            if (!reader.readU32(ts.totalWaves)) return SerializationResult::CorruptedData;
            if (!reader.readU32(ts.cooldownTicks)) return SerializationResult::CorruptedData;
            outChunkData.trialSpawners.push_back(std::move(ts));
        }

        uint16_t vaultCount = 0;
        if (!reader.readU16(vaultCount)) return SerializationResult::CorruptedData;
        outChunkData.vaults.clear();
        outChunkData.vaults.reserve(vaultCount);
        for (uint16_t i = 0; i < vaultCount; ++i) {
            VaultSaveData v;
            if (!reader.readI32(v.x)) return SerializationResult::CorruptedData;
            if (!reader.readI32(v.y)) return SerializationResult::CorruptedData;
            if (!reader.readI32(v.z)) return SerializationResult::CorruptedData;
            uint8_t omin = 0;
            if (!reader.readU8(omin)) return SerializationResult::CorruptedData;
            v.isOminous = (omin != 0);
            uint16_t pCount = 0;
            if (!reader.readU16(pCount)) return SerializationResult::CorruptedData;
            v.unlockedPlayers.reserve(pCount);
            for (uint16_t p = 0; p < pCount; ++p) {
                uint32_t pId = 0;
                if (!reader.readU32(pId)) return SerializationResult::CorruptedData;
                v.unlockedPlayers.push_back(pId);
            }
            outChunkData.vaults.push_back(std::move(v));
        }
    } else {
        outChunkData.trialSpawners.clear();
        outChunkData.vaults.clear();
    }

    return SerializationResult::Success;
}

void WorldFormat::extractChunkData(const Chunk& chunk, ChunkSaveData& outData, BlockRegistry& registry) {
    outData.schemaVersion = CURRENT_SCHEMA_VERSION;
    outData.chunkX = chunk.getChunkX();
    outData.chunkZ = chunk.getChunkZ();
    outData.sections.clear();
    outData.sections.resize(Chunk::SECTION_COUNT);

    std::string airName = registry.getNamespacedId(AIR_BLOCK_STATE);

    for (int secIdx = 0; secIdx < Chunk::SECTION_COUNT; ++secIdx) {
        int secY = Chunk::indexToSectionY(secIdx);
        auto& secData = outData.sections[secIdx];
        secData.sectionY = secY;

        const ChunkSection* section = chunk.getSectionByIndex(secIdx);
        if (!section || section->isEmpty()) {
            secData.isEmpty = true;
            continue;
        }

        secData.isEmpty = false;
        secData.palette.clear();
        secData.blockIndices.resize(ChunkSection::BLOCKS_PER_SECTION);

        std::unordered_map<std::string, uint16_t> paletteMap;
        paletteMap[airName] = 0;
        secData.palette.push_back(airName);

        const auto& raw = section->getRawBlocks();
        for (size_t i = 0; i < ChunkSection::BLOCKS_PER_SECTION; ++i) {
            BlockStateId st = raw[i];
            const std::string& name = registry.getNamespacedId(st);
            auto it = paletteMap.find(name);
            if (it == paletteMap.end()) {
                uint16_t newIdx = static_cast<uint16_t>(secData.palette.size());
                paletteMap[name] = newIdx;
                secData.palette.push_back(name);
                secData.blockIndices[i] = newIdx;
            } else {
                secData.blockIndices[i] = it->second;
            }
        }

        // Esquema v6: Guardar datos de iluminación si la sección está cargada
        const auto& rawLight = section->getRawLight();
        secData.light.assign(rawLight.begin(), rawLight.end());
    }

    // Esquema v3: Extraer bloques-entidad de cofre
    outData.chests.clear();
    auto& itemReg = ItemRegistry::global();
    for (const auto& [pos, be] : chunk.getBlockEntities()) {
        if (be && be->getType() == BlockEntityType::Chest) {
            auto chest = std::static_pointer_cast<ChestBlockEntity>(be);
            ChestSaveData cdata;
            cdata.x = pos.x;
            cdata.y = pos.y;
            cdata.z = pos.z;
            cdata.revision = chest->getRevision();
            cdata.slots.reserve(chest->getInventory().getSize());
            for (size_t s = 0; s < chest->getInventory().getSize(); ++s) {
                const auto& stack = chest->getInventory().getSlot(s);
                ItemStackSaveData isData;
                if (!stack.isEmpty()) {
                    isData.itemName = itemReg.get(stack.itemId).namespacedId;
                    isData.count = stack.count;
                    isData.durability = stack.durability;
                    isData.components = stack.components;
                } else {
                    isData.itemName = "opencraft:air";
                    isData.count = 0;
                    isData.durability = 0;
                }
                cdata.slots.push_back(std::move(isData));
            }
            outData.chests.push_back(std::move(cdata));
        }
    }

    // Esquema v4: Extraer bloques-entidad de horno
    outData.furnaces.clear();
    for (const auto& [pos, be] : chunk.getBlockEntities()) {
        if (be && be->getType() == BlockEntityType::Furnace) {
            auto furnace = std::static_pointer_cast<FurnaceBlockEntity>(be);
            FurnaceSaveData fdata;
            fdata.x = pos.x;
            fdata.y = pos.y;
            fdata.z = pos.z;
            fdata.revision = furnace->getRevision();
            fdata.litTime = furnace->getLitTimeTicks();
            fdata.litDuration = furnace->getLitDurationTicks();
            fdata.cookProgress = furnace->getCookProgressTicks();
            fdata.cookTotal = furnace->getCookTotalTicks();
            fdata.slots.reserve(furnace->getInventory().getSize());
            for (size_t s = 0; s < furnace->getInventory().getSize(); ++s) {
                const auto& stack = furnace->getInventory().getSlot(s);
                ItemStackSaveData isData;
                if (!stack.isEmpty()) {
                    isData.itemName = itemReg.get(stack.itemId).namespacedId;
                    isData.count = stack.count;
                    isData.durability = stack.durability;
                    isData.components = stack.components;
                } else {
                    isData.itemName = "opencraft:air";
                    isData.count = 0;
                    isData.durability = 0;
                }
                fdata.slots.push_back(std::move(isData));
            }
            outData.furnaces.push_back(std::move(fdata));
        }
    }

    // Esquema v9: Extraer tolvas, soltadores, dispensadores y crafters
    outData.hoppers.clear();
    outData.droppers.clear();
    outData.dispensers.clear();
    outData.crafters.clear();
    for (const auto& [pos, be] : chunk.getBlockEntities()) {
        if (!be) continue;
        if (be->getType() == BlockEntityType::Hopper) {
            auto hopper = std::static_pointer_cast<HopperBlockEntity>(be);
            HopperSaveData hdata;
            hdata.x = pos.x;
            hdata.y = pos.y;
            hdata.z = pos.z;
            hdata.revision = hopper->getRevision();
            hdata.facing = static_cast<uint8_t>(hopper->getFacing());
            hdata.cooldown = hopper->getCooldown();
            hdata.isLocked = hopper->isLocked();
            for (size_t s = 0; s < hopper->getInventory().getSize(); ++s) {
                const auto& stack = hopper->getInventory().getSlot(s);
                ItemStackSaveData isData;
                if (!stack.isEmpty()) {
                    isData.itemName = itemReg.get(stack.itemId).namespacedId;
                    isData.count = stack.count;
                    isData.durability = stack.durability;
                    isData.components = stack.components;
                } else {
                    isData.itemName = "opencraft:air";
                    isData.count = 0;
                    isData.durability = 0;
                }
                hdata.slots.push_back(std::move(isData));
            }
            outData.hoppers.push_back(std::move(hdata));
        } else if (be->getType() == BlockEntityType::Dropper) {
            auto dropper = std::static_pointer_cast<DropperBlockEntity>(be);
            DropperSaveData ddata;
            ddata.x = pos.x;
            ddata.y = pos.y;
            ddata.z = pos.z;
            ddata.revision = dropper->getRevision();
            ddata.facing = static_cast<uint8_t>(dropper->getFacing());
            for (size_t s = 0; s < dropper->getInventory().getSize(); ++s) {
                const auto& stack = dropper->getInventory().getSlot(s);
                ItemStackSaveData isData;
                if (!stack.isEmpty()) {
                    isData.itemName = itemReg.get(stack.itemId).namespacedId;
                    isData.count = stack.count;
                    isData.durability = stack.durability;
                    isData.components = stack.components;
                } else {
                    isData.itemName = "opencraft:air";
                    isData.count = 0;
                    isData.durability = 0;
                }
                ddata.slots.push_back(std::move(isData));
            }
            outData.droppers.push_back(std::move(ddata));
        } else if (be->getType() == BlockEntityType::Dispenser) {
            auto dispenser = std::static_pointer_cast<DispenserBlockEntity>(be);
            DispenserSaveData ddata;
            ddata.x = pos.x;
            ddata.y = pos.y;
            ddata.z = pos.z;
            ddata.revision = dispenser->getRevision();
            ddata.facing = static_cast<uint8_t>(dispenser->getFacing());
            for (size_t s = 0; s < dispenser->getInventory().getSize(); ++s) {
                const auto& stack = dispenser->getInventory().getSlot(s);
                ItemStackSaveData isData;
                if (!stack.isEmpty()) {
                    isData.itemName = itemReg.get(stack.itemId).namespacedId;
                    isData.count = stack.count;
                    isData.durability = stack.durability;
                    isData.components = stack.components;
                } else {
                    isData.itemName = "opencraft:air";
                    isData.count = 0;
                    isData.durability = 0;
                }
                ddata.slots.push_back(std::move(isData));
            }
            outData.dispensers.push_back(std::move(ddata));
        } else if (be->getType() == BlockEntityType::Crafter) {
            auto crafter = std::static_pointer_cast<CrafterBlockEntity>(be);
            CrafterSaveData cdata;
            cdata.x = pos.x;
            cdata.y = pos.y;
            cdata.z = pos.z;
            cdata.revision = crafter->getRevision();
            cdata.facing = static_cast<uint8_t>(crafter->getFacing());
            cdata.disabledSlotsMask = crafter->getDisabledSlotsMask();
            cdata.triggered = crafter->isTriggered();
            for (size_t s = 0; s < crafter->getInventory().getSize(); ++s) {
                const auto& stack = crafter->getInventory().getSlot(s);
                ItemStackSaveData isData;
                if (!stack.isEmpty()) {
                    isData.itemName = itemReg.get(stack.itemId).namespacedId;
                    isData.count = stack.count;
                    isData.durability = stack.durability;
                    isData.components = stack.components;
                } else {
                    isData.itemName = "opencraft:air";
                    isData.count = 0;
                    isData.durability = 0;
                }
                cdata.slots.push_back(std::move(isData));
            }
            outData.crafters.push_back(std::move(cdata));
        } else if (be->getType() == BlockEntityType::TrialSpawner) {
            auto ts = std::static_pointer_cast<TrialSpawnerBlockEntity>(be);
            TrialSpawnerSaveData tsData;
            tsData.x = pos.x;
            tsData.y = pos.y;
            tsData.z = pos.z;
            tsData.state = static_cast<uint8_t>(ts->getState());
            tsData.isOminous = ts->isOminous();
            tsData.currentWave = ts->getCurrentWave();
            tsData.totalWaves = ts->getTotalWaves();
            tsData.cooldownTicks = ts->getCooldownTicks();
            outData.trialSpawners.push_back(std::move(tsData));
        } else if (be->getType() == BlockEntityType::Vault) {
            auto vault = std::static_pointer_cast<VaultBlockEntity>(be);
            VaultSaveData vData;
            vData.x = pos.x;
            vData.y = pos.y;
            vData.z = pos.z;
            vData.isOminous = vault->isOminous();
            for (uint32_t pId : vault->getUnlockedPlayers()) {
                vData.unlockedPlayers.push_back(pId);
            }
            outData.vaults.push_back(std::move(vData));
        }
    }
}

void WorldFormat::applyChunkData(const ChunkSaveData& data, Chunk& chunk, BlockRegistry& registry) {
    if (data.schemaVersion == SCHEMA_VERSION_1) {
        // Mapear datos heredados v1 (y = 0..63)
        std::vector<BlockStateId> resolvedStates;
        resolvedStates.reserve(data.palette.size());
        for (const auto& name : data.palette) {
            resolvedStates.push_back(registry.getStateId(name));
        }

        for (int y = 0; y < Chunk::LEGACY_HEIGHT; ++y) {
            for (int z = 0; z < Chunk::CHUNK_DEPTH; ++z) {
                for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x) {
                    size_t idx =
                        static_cast<size_t>(y * (Chunk::CHUNK_WIDTH * Chunk::CHUNK_DEPTH) + z * Chunk::CHUNK_WIDTH + x);
                    if (idx < data.blockIndices.size()) {
                        uint16_t palIdx = data.blockIndices[idx];
                        BlockStateId st = (palIdx < resolvedStates.size()) ? resolvedStates[palIdx] : AIR_BLOCK_STATE;
                        chunk.setBlockState(x, y, z, st);
                    }
                }
            }
        }
        chunk.clearDirty();
        return;
    }

    // Esquema v2 y v3: Secciones de 16³
    for (const auto& secData : data.sections) {
        if (secData.isEmpty) {
            continue;
        }

        std::vector<BlockStateId> resolvedStates;
        resolvedStates.reserve(secData.palette.size());
        for (const auto& name : secData.palette) {
            resolvedStates.push_back(registry.getStateId(name));
        }

        auto& sec = chunk.getOrCreateSection(secData.sectionY);
        for (int ly = 0; ly < Chunk::SECTION_SIZE; ++ly) {
            for (int lz = 0; lz < Chunk::SECTION_SIZE; ++lz) {
                for (int lx = 0; lx < Chunk::SECTION_SIZE; ++lx) {
                    size_t idx = ChunkSection::getIndex(lx, ly, lz);
                    if (idx < secData.blockIndices.size()) {
                        uint16_t palIdx = secData.blockIndices[idx];
                        BlockStateId st = (palIdx < resolvedStates.size()) ? resolvedStates[palIdx] : AIR_BLOCK_STATE;
                        sec.setBlockState(lx, ly, lz, st);
                    }
                }
            }
        }

        // Esquema v6: Restaurar datos de iluminación si están disponibles
        if (!secData.light.empty() && secData.light.size() == ChunkSection::BLOCKS_PER_SECTION) {
            std::array<uint8_t, ChunkSection::BLOCKS_PER_SECTION> lightArr{};
            std::copy(secData.light.begin(), secData.light.end(), lightArr.begin());
            sec.setRawLight(lightArr);
        }
    }

    // Esquema v3: Aplicar bloques-entidad de cofre
    auto& itemReg = ItemRegistry::global();
    for (const auto& cdata : data.chests) {
        BlockPos pos{cdata.x, cdata.y, cdata.z};
        auto chest = std::make_shared<ChestBlockEntity>(pos);
        for (size_t s = 0; s < cdata.slots.size() && s < chest->getInventory().getSize(); ++s) {
            const auto& sdata = cdata.slots[s];
            if (!sdata.itemName.empty() && sdata.itemName != "opencraft:air" && sdata.count > 0) {
                ItemId id = itemReg.getId(sdata.itemName);
                if (id == EMPTY_ITEM_ID) {
                    // Fallback para nombres de bloques desconocidos o registrados por id
                    id = itemReg.getItemForBlock(registry.getStateId(sdata.itemName));
                }
                if (id != EMPTY_ITEM_ID) {
                    chest->getInventory().setSlot(s, ItemStack(id, sdata.count, sdata.durability, sdata.components));
                }
            }
        }
        chunk.setBlockEntity(pos, chest);
    }

    // Esquema v4: Aplicar bloques-entidad de horno
    for (const auto& fdata : data.furnaces) {
        BlockPos pos{fdata.x, fdata.y, fdata.z};
        auto furnace = std::make_shared<FurnaceBlockEntity>(pos);
        furnace->setLitTimeTicks(fdata.litTime);
        furnace->setLitDurationTicks(fdata.litDuration);
        furnace->setCookProgressTicks(fdata.cookProgress);
        furnace->setCookTotalTicks(fdata.cookTotal);
        for (size_t s = 0; s < fdata.slots.size() && s < furnace->getInventory().getSize(); ++s) {
            const auto& sdata = fdata.slots[s];
            if (!sdata.itemName.empty() && sdata.itemName != "opencraft:air" && sdata.count > 0) {
                ItemId id = itemReg.getId(sdata.itemName);
                if (id == EMPTY_ITEM_ID) {
                    id = itemReg.getItemForBlock(registry.getStateId(sdata.itemName));
                }
                if (id != EMPTY_ITEM_ID) {
                    furnace->getInventory().setSlot(s, ItemStack(id, sdata.count, sdata.durability, sdata.components));
                }
            }
        }
        chunk.setBlockEntity(pos, furnace);
    }

    // Esquema v9: Aplicar bloques-entidad de tolva
    for (const auto& hdata : data.hoppers) {
        BlockPos pos{hdata.x, hdata.y, hdata.z};
        auto hopper = std::make_shared<HopperBlockEntity>(pos, static_cast<HopperFacing>(hdata.facing));
        hopper->setCooldown(hdata.cooldown);
        hopper->setLocked(hdata.isLocked);
        for (size_t s = 0; s < hdata.slots.size() && s < hopper->getInventory().getSize(); ++s) {
            const auto& sdata = hdata.slots[s];
            if (!sdata.itemName.empty() && sdata.itemName != "opencraft:air" && sdata.count > 0) {
                ItemId id = itemReg.getId(sdata.itemName);
                if (id == EMPTY_ITEM_ID) id = itemReg.getItemForBlock(registry.getStateId(sdata.itemName));
                if (id != EMPTY_ITEM_ID) {
                    hopper->getInventory().setSlot(s, ItemStack(id, sdata.count, sdata.durability, sdata.components));
                }
            }
        }
        chunk.setBlockEntity(pos, hopper);
    }

    // Esquema v9: Aplicar bloques-entidad de soltador
    for (const auto& ddata : data.droppers) {
        BlockPos pos{ddata.x, ddata.y, ddata.z};
        auto dropper = std::make_shared<DropperBlockEntity>(pos, static_cast<BlockDirection>(ddata.facing));
        for (size_t s = 0; s < ddata.slots.size() && s < dropper->getInventory().getSize(); ++s) {
            const auto& sdata = ddata.slots[s];
            if (!sdata.itemName.empty() && sdata.itemName != "opencraft:air" && sdata.count > 0) {
                ItemId id = itemReg.getId(sdata.itemName);
                if (id == EMPTY_ITEM_ID) id = itemReg.getItemForBlock(registry.getStateId(sdata.itemName));
                if (id != EMPTY_ITEM_ID) {
                    dropper->getInventory().setSlot(s, ItemStack(id, sdata.count, sdata.durability, sdata.components));
                }
            }
        }
        chunk.setBlockEntity(pos, dropper);
    }

    // Esquema v9: Aplicar bloques-entidad de dispensador
    for (const auto& ddata : data.dispensers) {
        BlockPos pos{ddata.x, ddata.y, ddata.z};
        auto dispenser = std::make_shared<DispenserBlockEntity>(pos, static_cast<BlockDirection>(ddata.facing));
        for (size_t s = 0; s < ddata.slots.size() && s < dispenser->getInventory().getSize(); ++s) {
            const auto& sdata = ddata.slots[s];
            if (!sdata.itemName.empty() && sdata.itemName != "opencraft:air" && sdata.count > 0) {
                ItemId id = itemReg.getId(sdata.itemName);
                if (id == EMPTY_ITEM_ID) id = itemReg.getItemForBlock(registry.getStateId(sdata.itemName));
                if (id != EMPTY_ITEM_ID) {
                    dispenser->getInventory().setSlot(s,
                                                      ItemStack(id, sdata.count, sdata.durability, sdata.components));
                }
            }
        }
        chunk.setBlockEntity(pos, dispenser);
    }

    // Esquema v9: Aplicar bloques-entidad de crafter
    for (const auto& cdata : data.crafters) {
        BlockPos pos{cdata.x, cdata.y, cdata.z};
        auto crafter = std::make_shared<CrafterBlockEntity>(pos, static_cast<BlockDirection>(cdata.facing));
        crafter->setDisabledSlotsMask(cdata.disabledSlotsMask);
        crafter->setTriggered(cdata.triggered);
        for (size_t s = 0; s < cdata.slots.size() && s < crafter->getInventory().getSize(); ++s) {
            const auto& sdata = cdata.slots[s];
            if (!sdata.itemName.empty() && sdata.itemName != "opencraft:air" && sdata.count > 0) {
                ItemId id = itemReg.getId(sdata.itemName);
                if (id == EMPTY_ITEM_ID) id = itemReg.getItemForBlock(registry.getStateId(sdata.itemName));
                if (id != EMPTY_ITEM_ID) {
                    crafter->getInventory().setSlot(s, ItemStack(id, sdata.count, sdata.durability, sdata.components));
                }
            }
        }
        chunk.setBlockEntity(pos, crafter);
    }

    // Esquema v12: Aplicar bloques-entidad de Trial Spawner y Vault
    for (const auto& tsData : data.trialSpawners) {
        BlockPos pos{tsData.x, tsData.y, tsData.z};
        auto ts = std::make_shared<TrialSpawnerBlockEntity>(pos);
        ts->setState(static_cast<TrialSpawnerState>(tsData.state));
        ts->setOminous(tsData.isOminous);
        ts->setTotalWaves(tsData.totalWaves);
        ts->setCooldownTicks(tsData.cooldownTicks);
        chunk.setBlockEntity(pos, ts);
    }

    for (const auto& vData : data.vaults) {
        BlockPos pos{vData.x, vData.y, vData.z};
        auto vault = std::make_shared<VaultBlockEntity>(pos);
        vault->setOminous(vData.isOminous);
        std::unordered_set<uint32_t> unlocked(vData.unlockedPlayers.begin(), vData.unlockedPlayers.end());
        vault->setUnlockedPlayers(std::move(unlocked));
        chunk.setBlockEntity(pos, vault);
    }

    chunk.clearDirty();
}

// ==============================================================================
// Serialización y Deserialización de Entidades (Items tirados en el mundo)
// ==============================================================================
SerializationResult WorldFormat::serializeEntities(const std::vector<ItemEntitySaveData>& entities,
                                                   std::vector<uint8_t>& outBuffer) {
    outBuffer.clear();
    BufferWriter writer(outBuffer);

    // Cabecera: Magic (4 bytes), SchemaVersion (4 bytes), CRC32 Placeholder (4 bytes)
    writer.writeU32(MAGIC_ENTITIES);
    writer.writeU32(CURRENT_SCHEMA_VERSION);
    writer.writeU32(0); // Offset 8..11 para CRC32

    uint32_t count = static_cast<uint32_t>(entities.size());
    writer.writeU32(count);

    for (const auto& entity : entities) {
        writer.writeU32(entity.id);

        // ItemStackSaveData
        writer.writeString(entity.stack.itemName);
        writer.writeU16(entity.stack.count);
        writer.writeU16(entity.stack.durability);
        uint16_t compSize = static_cast<uint16_t>(entity.stack.components.size());
        writer.writeU16(compSize);
        for (uint8_t b : entity.stack.components) {
            writer.writeU8(b);
        }

        // Posición y velocidad
        writer.writeFloat(entity.position.x);
        writer.writeFloat(entity.position.y);
        writer.writeFloat(entity.position.z);
        writer.writeFloat(entity.velocity.x);
        writer.writeFloat(entity.velocity.y);
        writer.writeFloat(entity.velocity.z);

        // Ticks
        writer.writeU32(entity.pickupDelayTicks);
        writer.writeU32(entity.ageTicks);

        // Esquema v13: dimensión
        writer.writeU8(entity.dimension);
    }

    // Calcular CRC32 sobre todo el contenido posterior al campo CRC32 (offset 12 hasta fin)
    std::span<const uint8_t> payloadSpan(outBuffer.data() + 12, outBuffer.size() - 12);
    uint32_t crc = computeCRC32(payloadSpan);

    outBuffer[8] = static_cast<uint8_t>(crc & 0xFF);
    outBuffer[9] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    outBuffer[10] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    outBuffer[11] = static_cast<uint8_t>((crc >> 24) & 0xFF);

    return SerializationResult::Success;
}

SerializationResult WorldFormat::deserializeEntities(std::span<const uint8_t> buffer,
                                                     std::vector<ItemEntitySaveData>& outEntities) {
    outEntities.clear();
    if (buffer.size() < 12) {
        return SerializationResult::BufferTooSmall;
    }
    if (buffer.size() > MAX_FILE_SIZE) {
        return SerializationResult::CorruptedData;
    }

    BufferReader reader(buffer);
    uint32_t magic = 0;
    if (!reader.readU32(magic)) {
        return SerializationResult::BufferTooSmall;
    }

    if (magic == MAGIC_ENTITIES) {
        uint32_t schema = 0;
        uint32_t storedCrc = 0;
        if (!reader.readU32(schema) || schema < SCHEMA_VERSION_1 || schema > CURRENT_SCHEMA_VERSION) {
            return SerializationResult::UnsupportedVersion;
        }
        if (!reader.readU32(storedCrc)) {
            return SerializationResult::BufferTooSmall;
        }

        std::span<const uint8_t> payloadSpan(buffer.data() + 12, buffer.size() - 12);
        uint32_t calculatedCrc = computeCRC32(payloadSpan);
        if (storedCrc != calculatedCrc) {
            return SerializationResult::ChecksumMismatch;
        }

        uint32_t count = 0;
        if (!reader.readU32(count)) {
            return SerializationResult::CorruptedData;
        }

        if (count > 65536) {
            return SerializationResult::CorruptedData;
        }

        outEntities.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            ItemEntitySaveData entity;
            if (!reader.readU32(entity.id)) return SerializationResult::CorruptedData;

            if (!reader.readString(entity.stack.itemName)) return SerializationResult::CorruptedData;
            if (!reader.readU16(entity.stack.count)) return SerializationResult::CorruptedData;
            if (!reader.readU16(entity.stack.durability)) return SerializationResult::CorruptedData;
            uint16_t compSize = 0;
            if (!reader.readU16(compSize)) return SerializationResult::CorruptedData;
            entity.stack.components.resize(compSize);
            for (uint16_t c = 0; c < compSize; ++c) {
                if (!reader.readU8(entity.stack.components[c])) return SerializationResult::CorruptedData;
            }

            if (!reader.readFloat(entity.position.x) || !reader.readFloat(entity.position.y) ||
                !reader.readFloat(entity.position.z)) {
                return SerializationResult::CorruptedData;
            }
            if (!reader.readFloat(entity.velocity.x) || !reader.readFloat(entity.velocity.y) ||
                !reader.readFloat(entity.velocity.z)) {
                return SerializationResult::CorruptedData;
            }

            if (!reader.readU32(entity.pickupDelayTicks)) return SerializationResult::CorruptedData;
            if (!reader.readU32(entity.ageTicks)) return SerializationResult::CorruptedData;

            if (schema >= SCHEMA_VERSION_13) {
                if (!reader.readU8(entity.dimension) || entity.dimension > 2) return SerializationResult::CorruptedData;
            }

            outEntities.push_back(std::move(entity));
        }

        return SerializationResult::Success;
    }

    // Fallback retrocompatible para formato legacy de items.dat (uint32_t count + n * 42 bytes)
    uint32_t legacyCount = magic;
    if (buffer.size() == 4 + static_cast<size_t>(legacyCount) * 42) {
        auto& itemReg = ItemRegistry::global();
        BufferReader legacyReader(buffer);
        uint32_t c = 0;
        legacyReader.readU32(c);
        outEntities.reserve(c);
        for (uint32_t i = 0; i < c; ++i) {
            ItemEntitySaveData entity;
            uint16_t rawItemId = 0;
            if (!legacyReader.readU32(entity.id)) return SerializationResult::CorruptedData;
            if (!legacyReader.readU16(rawItemId)) return SerializationResult::CorruptedData;
            if (!legacyReader.readU16(entity.stack.count)) return SerializationResult::CorruptedData;
            if (!legacyReader.readU16(entity.stack.durability)) return SerializationResult::CorruptedData;
            entity.stack.itemName = itemReg.get(rawItemId).namespacedId;
            entity.stack.components = {};

            if (!legacyReader.readFloat(entity.position.x) || !legacyReader.readFloat(entity.position.y) ||
                !legacyReader.readFloat(entity.position.z))
                return SerializationResult::CorruptedData;
            if (!legacyReader.readFloat(entity.velocity.x) || !legacyReader.readFloat(entity.velocity.y) ||
                !legacyReader.readFloat(entity.velocity.z))
                return SerializationResult::CorruptedData;
            if (!legacyReader.readU32(entity.pickupDelayTicks)) return SerializationResult::CorruptedData;
            if (!legacyReader.readU32(entity.ageTicks)) return SerializationResult::CorruptedData;

            outEntities.push_back(std::move(entity));
        }
        return SerializationResult::Success;
    }

    return SerializationResult::InvalidMagic;
}

} // namespace OpenCraft
