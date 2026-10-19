#pragma once

#include <set>
#include <tuple>

#include <optional>

#include <array>
#include <vector>
#include <memory>
#include "world/World.hpp"
#include "world/Dimension.hpp"
#include "world/BlockRegistry.hpp"
#include "sim/Entity.hpp"
#include "sim/SimulationClock.hpp"
#include "sim/GameCommand.hpp"
#include "sim/GameEvent.hpp"
#include "sim/Snapshot.hpp"
#include "storage/WorldStorage.hpp"

#include "sim/ItemEntity.hpp"
#include "sim/MobEntity.hpp"
#include "sim/ProjectileEntity.hpp"
#include "sim/ExperienceOrbEntity.hpp"
#include "sim/MobSpawner.hpp"
#include "item/Item.hpp"
#include "item/Inventory.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/FurnaceBlockEntity.hpp"
#include "crafting/RecipeRegistry.hpp"
#include "world/ScheduledUpdates.hpp"
#include "world/FluidSimulator.hpp"
#include "sim/Weather.hpp"
#include "sim/BoatEntity.hpp"
#include "sim/AnimalBreeding.hpp"
#include "world/Agriculture.hpp"
#include "world/CinnabarEngine.hpp"
#include "sim/MinecartEntity.hpp"
#include "sim/CommandEngine.hpp"
#include "sim/FunctionRunner.hpp"
#include "content/ContentPackManager.hpp"
#include "progression/AdvancementManager.hpp"
#include "progression/StatisticsManager.hpp"

namespace OpenCraft {

class WorldSession {
public:
    static constexpr float MAX_REACH_DISTANCE = 5.0f;

    WorldSession();
    ~WorldSession() = default;

    void init(int renderDistance = 3, uint64_t seed = 0);

    // Integración con Persistencia
    StorageError saveToStorage(WorldStorage& storage);
    StorageError loadFromStorage(WorldStorage& storage);

    // Entrada de comandos
    void queueCommand(const GameCommand& cmd);
    CommandResult executeCommand(const GameCommand& cmd);

    // Bucle de actualización
    void update(double dt);
    void tick();

    // Consulta de estado
    World& getWorld() noexcept { return m_world; }
    const World& getWorld() const noexcept { return m_world; }

    PlayerEntity& getPlayer() noexcept { return m_player; }
    const PlayerEntity& getPlayer() const noexcept { return m_player; }

    // Gestión Multi-Jugador (Fase 09)
    PlayerEntity* addPlayer(EntityId id, const std::string& name, const Math::Vec3& spawnPos = {8.0f, 68.0f, 8.0f});
    bool removePlayer(EntityId id);
    PlayerEntity* getPlayerById(EntityId id) noexcept;
    const PlayerEntity* getPlayerById(EntityId id) const noexcept;
    PlayerEntity* getPlayerByName(const std::string& name) noexcept;
    const PlayerEntity* getPlayerByName(const std::string& name) const noexcept;
    EntityId getPlayerIdByName(const std::string& name) const noexcept;
    const std::unordered_map<EntityId, PlayerEntity>& getConnectedPlayers() const noexcept {
        return m_connectedPlayers;
    }
    std::unordered_map<EntityId, PlayerEntity>& getConnectedPlayers() noexcept { return m_connectedPlayers; }
    const std::string& getPlayerName(EntityId id) const;

    SimulationClock& getClock() noexcept { return m_clock; }
    const SimulationClock& getClock() const noexcept { return m_clock; }

    WorldSnapshot getSnapshot() const;

    // Gestión de eventos
    void addEventListener(IGameEventListener* listener);
    void removeEventListener(IGameEventListener* listener);

    // Gestión de objetos tirados (ItemEntity)
    ItemEntity* spawnDroppedItem(ItemStack stack, const Math::Vec3& pos, const Math::Vec3& vel = {0.0f, 0.0f, 0.0f},
                                 uint32_t delay = 20);
    const std::vector<ItemEntity>& getItemEntities() const noexcept { return m_itemEntities; }
    std::vector<ItemEntity>& getItemEntities() noexcept { return m_itemEntities; }

    // Gestión de Criaturas y Entidades (Fase 08)
    MobEntity* spawnMob(MobType type, const Math::Vec3& pos);
    const std::vector<MobEntity>& getMobs() const noexcept { return m_mobs; }
    std::vector<MobEntity>& getMobs() noexcept { return m_mobs; }

    ProjectileEntity* spawnProjectile(EntityId shooterId, const Math::Vec3& pos, const Math::Vec3& vel,
                                      float damage = 4.0f);
    const std::vector<ProjectileEntity>& getProjectiles() const noexcept { return m_projectiles; }
    std::vector<ProjectileEntity>& getProjectiles() noexcept { return m_projectiles; }

    ExperienceOrbEntity* spawnExperienceOrb(const Math::Vec3& pos, uint32_t value);
    const std::vector<ExperienceOrbEntity>& getExperienceOrbs() const noexcept { return m_experienceOrbs; }
    std::vector<ExperienceOrbEntity>& getExperienceOrbs() noexcept { return m_experienceOrbs; }

    BoatEntity* spawnBoat(const Math::Vec3& pos);
    const std::vector<BoatEntity>& getBoats() const noexcept { return m_boats; }
    std::vector<BoatEntity>& getBoats() noexcept { return m_boats; }

    MinecartEntity* spawnMinecart(const Math::Vec3& pos, MinecartType type = MinecartType::Standard);
    const std::vector<MinecartEntity>& getMinecarts() const noexcept { return m_minecarts; }
    std::vector<MinecartEntity>& getMinecarts() noexcept { return m_minecarts; }

    CinnabarEngine& getCinnabarEngine() noexcept { return m_cinnabarEngine; }
    const CinnabarEngine& getCinnabarEngine() const noexcept { return m_cinnabarEngine; }

    MobSpawner& getMobSpawner() noexcept { return m_mobSpawner; }
    const MobSpawner& getMobSpawner() const noexcept { return m_mobSpawner; }

    EntityId& getNextEntityId() noexcept { return m_nextEntityId; }

    void explode(const Math::Vec3& pos, float radius = 3.0f, bool causeBlockGriefing = true);
    void notifyTerrainChange(int x, int y, int z);

    // Estado de contenedor abierto
    bool isContainerOpen() const noexcept { return m_isContainerOpen; }
    OpenContainerKind getOpenContainerKind() const noexcept { return m_openContainerKind; }
    const BlockPos& getOpenContainerPos() const noexcept { return m_openContainerPos; }
    void returnCursorItemToInventory();

    // Fabricación
    const std::vector<ItemStack>& getCraftingGrid() const noexcept { return m_craftingGrid; }
    std::vector<ItemStack>& getCraftingGrid() noexcept { return m_craftingGrid; }
    const ItemStack& getCraftingOutput() const noexcept { return m_craftingOutput; }
    void updateCraftingOutput();
    void returnCraftingGridToInventory();

    // Minado progresivo
    float getMiningProgress() const noexcept { return m_miningProgress; }
    const BlockPos& getMiningPos() const noexcept { return m_miningPos; }
    bool isMining() const noexcept { return m_isMining; }

    // Mecánicas de supervivencia y daño
    void applyDamage(float amount, DamageCause cause = DamageCause::Generic, const Math::Vec3* sourcePos = nullptr);
    void dropAllInventoryOnDeath();
    void respawnPlayer();
    Math::Vec3 findSafeSpawn(const Math::Vec3& target);
    // Busca suelo seco a cielo abierto en espiral alrededor de (x, z) dentro de los chunks cargados
    std::optional<Math::Vec3> findSurfaceSpawn(int x, int z, int searchRadius = 32) const;
    // Coloca al jugador sobre la superficie si es un mundo recién creado o quedó enterrado
    bool ensurePlayerAboveGround(bool freshWorld);
    // Máquina de estados del generador de desafío integrada en el tick (A07)
    void tickTrialSpawner(class TrialSpawnerBlockEntity& spawner);
    bool isHeadInWater() const;
    bool isHeadInSolid() const;
    bool isFeetInWater() const;
    bool isFeetInLava() const;
    bool isHeadInLava() const;

    // Clima
    const WeatherStatus& getWeather() const noexcept { return m_weather; }
    WeatherStatus& getWeather() noexcept { return m_weather; }
    void setWeather(WeatherState state, uint32_t ticks = 24000);

    // Modo Extremo (Hardcore) y Comandos / Paquetes / Avances (Fase 13)
    bool isHardcore() const noexcept { return m_isHardcore; }
    void setHardcore(bool hc) noexcept {
        m_isHardcore = hc;
        if (hc) m_difficulty = 3;
    }

    CommandEngine& getCommandEngine() noexcept { return m_commandEngine; }
    const CommandEngine& getCommandEngine() const noexcept { return m_commandEngine; }

    FunctionRunner& getFunctionRunner() noexcept { return m_functionRunner; }
    const FunctionRunner& getFunctionRunner() const noexcept { return m_functionRunner; }

    ContentPackManager& getContentPackManager() noexcept { return m_contentPackManager; }
    const ContentPackManager& getContentPackManager() const noexcept { return m_contentPackManager; }

    AdvancementManager& getAdvancementManager() noexcept { return m_advancementManager; }
    const AdvancementManager& getAdvancementManager() const noexcept { return m_advancementManager; }

    StatisticsManager& getStatisticsManager() noexcept { return m_statisticsManager; }
    const StatisticsManager& getStatisticsManager() const noexcept { return m_statisticsManager; }

    // Dimensiones (Fase 12): la sesión simula una dimensión activa; las demás quedan aparcadas
    // en memoria (mundo y entidades) y se guardan en la misma transacción que la activa.
    uint8_t getCurrentDimension() const noexcept { return m_currentDimension; }
    // Cambia la dimensión activa conservando la posición del jugador local
    bool setCurrentDimension(uint8_t dim);
    // Transfiere al jugador local a la dimensión indicada: aparca la dimensión actual y activa la de
    // destino desde memoria, desde el almacenamiento asociado o mediante su generador canónico.
    bool changeDimension(DimensionType target, const Math::Vec3& targetPos);
    bool isDimensionResident(DimensionType dim) const noexcept;
    // Almacenamiento del que se materializan las dimensiones no residentes. loadFromStorage() y
    // saveToStorage() lo asocian automáticamente; debe sobrevivir a la sesión o desasociarse con nullptr.
    void attachStorage(WorldStorage* storage) noexcept { m_attachedStorage = storage; }

    RecipeRegistry& getRecipeRegistry() noexcept { return RecipeRegistry::global(); }
    const RecipeRegistry& getRecipeRegistry() const noexcept { return RecipeRegistry::global(); }

    // Actualizaciones de bloques y fluidos
    ScheduledUpdateQueue& getScheduledUpdates() noexcept { return m_scheduledUpdates; }
    const ScheduledUpdateQueue& getScheduledUpdates() const noexcept { return m_scheduledUpdates; }
    FluidSimulator& getFluidSimulator() noexcept { return m_fluidSimulator; }

    uint32_t getDifficulty() const noexcept { return m_difficulty; }
    void setDifficulty(uint32_t diff) noexcept { m_difficulty = diff; }

    // Hash de estado de simulación (Mundo + Entidades) para pruebas de determinismo
    uint64_t computeStateHash() const noexcept;

private:
    // Estado de simulación de una dimensión que no es la activa
    struct ParkedDimension {
        World world;
        std::vector<ItemEntity> itemEntities;
        std::vector<MobEntity> mobs;
        std::vector<ProjectileEntity> projectiles;
        std::vector<ExperienceOrbEntity> experienceOrbs;
        std::vector<BoatEntity> boats;
        std::vector<MinecartEntity> minecarts;
        ScheduledUpdateQueue scheduledUpdates;
        CinnabarEngine cinnabarEngine;
    };

    World m_world;
    PlayerEntity m_player;
    std::unordered_map<EntityId, PlayerEntity> m_connectedPlayers;
    std::unordered_map<EntityId, std::string> m_playerNames;
    std::unordered_map<EntityId, PlayerEntity> m_offlinePlayers;
    std::unordered_map<EntityId, std::string> m_offlinePlayerNames;
    SimulationClock m_clock;
    uint32_t m_difficulty = 2; // 0 = Peaceful, 1 = Easy, 2 = Normal, 3 = Hard
    bool m_isHardcore = false;
    uint8_t m_currentDimension = 0;

    CommandEngine m_commandEngine;
    FunctionRunner m_functionRunner;
    ContentPackManager m_contentPackManager;
    AdvancementManager m_advancementManager;
    StatisticsManager m_statisticsManager;

    ScheduledUpdateQueue m_scheduledUpdates;
    FluidSimulator m_fluidSimulator;
    WeatherStatus m_weather;

    std::vector<ItemEntity> m_itemEntities;
    std::vector<MobEntity> m_mobs;
    std::vector<ProjectileEntity> m_projectiles;
    std::vector<ExperienceOrbEntity> m_experienceOrbs;
    std::vector<BoatEntity> m_boats;
    std::vector<MinecartEntity> m_minecarts;
    CinnabarEngine m_cinnabarEngine;
    MobSpawner m_mobSpawner;

    EntityId m_nextEntityId = 1000;

    bool m_isContainerOpen = false;
    OpenContainerKind m_openContainerKind = OpenContainerKind::None;
    BlockPos m_openContainerPos{0, 0, 0};

    // Cuadrícula de fabricación (4 ranuras para 2x2, 9 ranuras para 3x3) y salida
    std::vector<ItemStack> m_craftingGrid;
    ItemStack m_craftingOutput;

    // Minado progresivo
    BlockPos m_miningPos{0, 0, 0};
    float m_miningProgress = 0.0f;
    bool m_isMining = false;

    Math::Vec3 m_pendingMoveInput{0.0f, 0.0f, 0.0f};
    bool m_pendingJump = false;

    std::vector<GameCommand> m_commandQueue;
    std::vector<GameEvent> m_pendingEvents;
    std::vector<IGameEventListener*> m_listeners;

    std::array<std::unique_ptr<ParkedDimension>, 3> m_parkedDimensions;
    WorldStorage* m_attachedStorage = nullptr;

    void resolveCollisions(Math::Vec3& vel, float dt);
    void dispatchEvents();

    void swapActiveDimension(ParkedDimension& slot);
    ParkedDimension& getOrCreateParkedDimension(DimensionType dim);
    StorageError populateDimensionArea(World& world, std::vector<MobEntity>& mobs, DimensionType dim, int centerChunkX,
                                       int centerChunkZ, WorldStorage* storage, bool generateMissing = true);
    StorageError stageDimensionChunks(WorldStorage& storage, const World& world, const std::vector<MobEntity>& mobs,
                                      DimensionType dim, std::vector<Chunk*>& outStaged,
                                      std::vector<std::pair<std::tuple<int, int, int>, bool>>& outMobChunks) const;
    // Chunks cuyo archivo guardado contiene criaturas (dimensión, x, z): deben reescribirse cuando las
    // criaturas se marchan aunque sus bloques no cambien, o al recargar aparecerían duplicadas
    std::set<std::tuple<int, int, int>> m_chunksWithSavedMobs;
    void tickPortals();
};

} // namespace OpenCraft
