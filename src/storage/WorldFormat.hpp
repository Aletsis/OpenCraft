#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <span>
#include <optional>
#include "world/Chunk.hpp"
#include "world/BlockRegistry.hpp"
#include "math/Math.hpp"

namespace OpenCraft {

constexpr uint32_t MAGIC_WORLD_META = 0x444D434F; // 'OCMD' (OpenCraft MetaData)
constexpr uint32_t MAGIC_PLAYER = 0x4C50434F;     // 'OCPL' (OpenCraft PLayer)
constexpr uint32_t MAGIC_CHUNK = 0x4B43434F;      // 'OCCK' (OpenCraft ChunK)
constexpr uint32_t MAGIC_JOURNAL = 0x524A434F;    // 'OCJR' (OpenCraft JouRnal)
constexpr uint32_t MAGIC_ENTITIES = 0x4E45434F;   // 'OCEN' (OpenCraft ENtities)

constexpr uint32_t SCHEMA_VERSION_1 = 1;
constexpr uint32_t SCHEMA_VERSION_2 = 2;
constexpr uint32_t SCHEMA_VERSION_3 = 3;
constexpr uint32_t SCHEMA_VERSION_4 = 4;
constexpr uint32_t SCHEMA_VERSION_5 = 5;
constexpr uint32_t SCHEMA_VERSION_6 = 6;
constexpr uint32_t SCHEMA_VERSION_7 = 7;
constexpr uint32_t SCHEMA_VERSION_8 = 8;
constexpr uint32_t SCHEMA_VERSION_9 = 9;
constexpr uint32_t SCHEMA_VERSION_10 = 10;
constexpr uint32_t SCHEMA_VERSION_11 = 11;
constexpr uint32_t SCHEMA_VERSION_12 = 12;
constexpr uint32_t SCHEMA_VERSION_13 = 13;
constexpr uint32_t CURRENT_SCHEMA_VERSION = SCHEMA_VERSION_13;
constexpr uint32_t CURRENT_GENERATOR_VERSION = 3;

// Límites de seguridad para deserialización
constexpr size_t MAX_NAME_LENGTH = 256;
constexpr size_t MAX_PALETTE_SIZE = 4096;
constexpr size_t MAX_CHUNK_DATA_SIZE = 4 * 1024 * 1024; // 4 MB máx por chunk
constexpr size_t MAX_FILE_SIZE = 16 * 1024 * 1024;      // 16 MB máx

// Utilidad de Checksum CRC32 (IEEE 802.3)
uint32_t computeCRC32(std::span<const uint8_t> data, uint32_t previousCrc = 0) noexcept;

struct WorldMetadata {
    uint32_t schemaVersion = CURRENT_SCHEMA_VERSION;
    uint32_t generatorVersion = CURRENT_GENERATOR_VERSION;
    std::string worldName;
    std::string worldId;
    uint64_t seed = 0;
    int64_t creationTime = 0;
    int64_t lastPlayedTime = 0;
    uint32_t gameMode = 0;   // 0: Supervivencia, 1: Creativo
    uint32_t difficulty = 1; // 0: Pacífico, 1: Fácil, 2: Normal, 3: Difícil
    uint64_t simulationTick = 0;
    std::vector<std::string> blockCatalog; // Catálogo de nombres con namespace

    // Esquema v6: Clima y tiempo atmosférico
    uint8_t weatherState = 0;     // 0: Clear, 1: Rain, 2: Thunder
    uint32_t weatherDuration = 0; // Ticks restantes
    float rainStrength = 0.0f;    // 0.0f a 1.0f
    float thunderStrength = 0.0f; // 0.0f a 1.0f

    // Esquema v11: Modo Extremo (Hardcore) y reglas de juego (GameRules)
    bool isHardcore = false;
    std::vector<std::pair<std::string, std::string>> gameRules;
};

struct ItemStackSaveData {
    std::string itemName;
    uint16_t count = 0;
    uint16_t durability = 0;
    std::vector<uint8_t> components;
};

struct ItemEntitySaveData {
    uint32_t id = 0;
    ItemStackSaveData stack;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    uint32_t pickupDelayTicks = 0;
    uint32_t ageTicks = 0;
    // Esquema v13: dimensión en la que existe la entidad (0: Overworld, 1: Abismo, 2: TheEnd)
    uint8_t dimension = 0;
};

struct PlayerSaveData {
    uint32_t schemaVersion = CURRENT_SCHEMA_VERSION;
    Math::Vec3 position{8.0f, 68.0f, 8.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool isGrounded = true;
    float health = 20.0f;
    float hunger = 20.0f;
    uint32_t selectedSlot = 0;
    std::vector<std::string> hotbarNames;

    // Esquema v3: Inventario completo (36 ranuras), cursor y offhand
    std::vector<ItemStackSaveData> inventorySlots;
    ItemStackSaveData cursorItem;
    ItemStackSaveData offhandItem;

    // Esquema v5: Parámetros ampliados de supervivencia y modo de juego
    float saturation = 5.0f;
    float exhaustion = 0.0f;
    int32_t air = 300;
    uint32_t gameMode = 0;
    Math::Vec3 spawnPoint{8.0f, 68.0f, 8.0f};
    bool hasCustomSpawnPoint = false;
    bool isAlive = true;

    // Esquema v7: Experiencia y armadura equipada
    uint32_t xpTotal = 0;
    uint32_t xpLevel = 0;
    float xpProgress = 0.0f;
    std::vector<ItemStackSaveData> armorSlots;

    // Esquema v10: Dimensión actual, estado de planeo/élitros y efectos activos
    struct ActiveEffectSaveData {
        uint8_t type = 0;
        uint32_t durationTicks = 0;
        uint8_t amplifier = 0;
        bool isAmbient = false;
    };
    uint8_t currentDimension = 0; // 0: Overworld, 1: Abismo, 2: TheEnd
    bool isGliding = false;
    std::vector<ActiveEffectSaveData> activeEffects;

    // Esquema v11: Avances desbloqueados y estadísticas del jugador
    std::vector<std::string> unlockedAdvancements;
    std::vector<std::pair<std::string, uint64_t>> generalStatistics;

    // Esquema v12: Identidad persistente de jugador multijugador
    uint32_t entityId = 0;
    std::string playerName;
};

struct SectionSaveData {
    int32_t sectionY = 0;
    bool isEmpty = true;
    std::vector<std::string> palette;
    std::vector<uint16_t> blockIndices;
    // Esquema v6: 4096 bytes de luz (4 bits sky, 4 bits block)
    std::vector<uint8_t> light;
};

struct ChestSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    uint32_t revision = 0;
    std::vector<ItemStackSaveData> slots;
};

struct FurnaceSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    uint32_t revision = 0;
    uint32_t litTime = 0;
    uint32_t litDuration = 0;
    uint32_t cookProgress = 0;
    uint32_t cookTotal = 200;
    std::vector<ItemStackSaveData> slots;
};

struct ScheduledUpdateSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    std::string blockName;
    uint64_t targetTick = 0;
    int32_t priority = 0;
};

struct MobSaveData {
    std::string mobType = "opencraft:zombie";
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    float health = 20.0f;
    uint8_t aiState = 0;
    bool isPersistent = false;

    // Esquema v8: datos de cría, domesticación y aldeano
    bool inLove = false;
    uint32_t breedingCooldown = 0;
    bool isBaby = false;
    bool isTamed = false;
    int64_t ownerId = -1;
    bool isSitting = false;
    uint8_t villagerProfession = 0;
};

// Esquema v9: Bloques-entidad de tolva, soltador, dispensador, crafter y vagonetas
struct HopperSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    uint32_t revision = 0;
    uint8_t facing = 0;
    uint32_t cooldown = 0;
    bool isLocked = false;
    std::vector<ItemStackSaveData> slots;
};

struct DropperSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    uint32_t revision = 0;
    uint8_t facing = 0;
    std::vector<ItemStackSaveData> slots;
};

struct DispenserSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    uint32_t revision = 0;
    uint8_t facing = 0;
    std::vector<ItemStackSaveData> slots;
};

struct CrafterSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    uint32_t revision = 0;
    uint8_t facing = 0;
    uint16_t disabledSlotsMask = 0;
    bool triggered = false;
    std::vector<ItemStackSaveData> slots;
};

struct MinecartSaveData {
    uint8_t minecartType = 0;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    Math::Vec3 velocity{0.0f, 0.0f, 0.0f};
    std::vector<ItemStackSaveData> slots;
};

// Esquema v12: Bloques-entidad 1.21 (cámaras de desafío)
struct TrialSpawnerSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    uint8_t state = 0;
    bool isOminous = false;
    uint32_t currentWave = 0;
    uint32_t totalWaves = 2;
    uint32_t cooldownTicks = 0;
};

struct VaultSaveData {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    bool isOminous = false;
    std::vector<uint32_t> unlockedPlayers;
};

struct ChunkSaveData {
    uint32_t schemaVersion = CURRENT_SCHEMA_VERSION;
    int32_t chunkX = 0;
    int32_t chunkZ = 0;

    // Compatibilidad Esquema v1 (columna fija 16x64x16)
    std::vector<std::string> palette;
    std::vector<uint16_t> blockIndices;

    // Esquema v2 y v3 (secciones verticales 16³)
    std::vector<SectionSaveData> sections;

    // Esquema v3: Bloques-entidad de cofre
    std::vector<ChestSaveData> chests;

    // Esquema v4: Bloques-entidad de horno
    std::vector<FurnaceSaveData> furnaces;

    // Esquema v6: Actualizaciones de bloques programadas pendientes
    std::vector<ScheduledUpdateSaveData> scheduledUpdates;

    // Esquema v7: Criaturas en el chunk
    std::vector<MobSaveData> mobs;

    // Esquema v9: Bloques-entidad de automatización y vagonetas
    std::vector<HopperSaveData> hoppers;
    std::vector<DropperSaveData> droppers;
    std::vector<DispenserSaveData> dispensers;
    std::vector<CrafterSaveData> crafters;
    std::vector<MinecartSaveData> minecarts;

    // Esquema v12: Bloques-entidad 1.21 (cámaras de desafío)
    std::vector<TrialSpawnerSaveData> trialSpawners;
    std::vector<VaultSaveData> vaults;
};

enum class SerializationResult {
    Success,
    BufferTooSmall,
    InvalidMagic,
    UnsupportedVersion,
    ChecksumMismatch,
    CorruptedData,
    NameTooLong,
    PaletteTooLarge
};

const char* serializationResultToString(SerializationResult res) noexcept;

class WorldFormat {
public:
    // Serialización / Deserialización de Metadatos
    static SerializationResult serializeMetadata(const WorldMetadata& meta, std::vector<uint8_t>& outBuffer);
    static SerializationResult deserializeMetadata(std::span<const uint8_t> buffer, WorldMetadata& outMeta);

    // Serialización / Deserialización de Jugador
    static SerializationResult serializePlayer(const PlayerSaveData& player, std::vector<uint8_t>& outBuffer);
    static SerializationResult deserializePlayer(std::span<const uint8_t> buffer, PlayerSaveData& outPlayer);

    // Serialización / Deserialización de Chunk
    static SerializationResult serializeChunk(const Chunk& chunk, std::vector<uint8_t>& outBuffer,
                                              const std::vector<MobSaveData>& mobs = {});
    static SerializationResult serializeChunkData(const ChunkSaveData& data, std::vector<uint8_t>& outBuffer);
    static SerializationResult deserializeChunk(std::span<const uint8_t> buffer, ChunkSaveData& outChunkData);

    // Serialización / Deserialización de Entidades (Items tirados en el mundo)
    static SerializationResult serializeEntities(const std::vector<ItemEntitySaveData>& entities,
                                                 std::vector<uint8_t>& outBuffer);
    static SerializationResult deserializeEntities(std::span<const uint8_t> buffer,
                                                   std::vector<ItemEntitySaveData>& outEntities);

    // Reconstruir Chunk desde ChunkSaveData usando BlockRegistry
    static void applyChunkData(const ChunkSaveData& data, Chunk& chunk,
                               BlockRegistry& registry = BlockRegistry::global());

    // Extraer ChunkSaveData desde Chunk
    static void extractChunkData(const Chunk& chunk, ChunkSaveData& outData,
                                 BlockRegistry& registry = BlockRegistry::global());
};

} // namespace OpenCraft
