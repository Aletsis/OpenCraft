#include "sim/WorldSession.hpp"
#include "core/DeterministicRandom.hpp"
#include "world/HopperBlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "world/DropperBlockEntity.hpp"
#include "world/CrafterBlockEntity.hpp"
#include "world/AutomationBlockEntities.hpp"
#include "world/PistonSimulator.hpp"
#include "world/RailSystem.hpp"
#include "world/CommandBlockEntity.hpp"
#include "world/TrialSpawnerBlockEntity.hpp"
#include "world/VaultBlockEntity.hpp"
#include "world/PortalSystem.hpp"
#include "core/Log.hpp"
#include "progression/Enchantment.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <chrono>
#include <fstream>

namespace OpenCraft {

static inline int floorDiv16(int a) noexcept {
    int res = a / 16;
    int rem = a % 16;
    if (rem != 0 && (a < 0)) {
        res--;
    }
    return res;
}

WorldSession::WorldSession() {
    auto& reg = BlockRegistry::global();
    auto& itemReg = ItemRegistry::global();
    m_player.position = Math::Vec3(8.0f, 32.0f, 8.0f);
    m_player.prevPosition = m_player.position;

    m_player.hotbar = {
        reg.fromLegacyId(BlockID::Grass),       reg.fromLegacyId(BlockID::Dirt), reg.fromLegacyId(BlockID::Stone),
        reg.fromLegacyId(BlockID::Cobblestone), reg.fromLegacyId(BlockID::Wood), reg.fromLegacyId(BlockID::Planks),
        reg.fromLegacyId(BlockID::Leaves),      reg.fromLegacyId(BlockID::Sand), reg.fromLegacyId(BlockID::Glass)};
    m_player.selectedSlot = 0;

    // Inicializar inventario de 36 ranuras
    m_player.inventory.resize(36);
    for (size_t i = 0; i < 9; ++i) {
        ItemId itId = itemReg.getItemForBlock(m_player.hotbar[i]);
        if (itId != EMPTY_ITEM_ID) {
            m_player.inventory.setSlot(i, ItemStack(itId, 64));
        }
    }
    m_player.syncHotbar();
}

void WorldSession::init(int renderDistance, uint64_t seed) {
    m_world.init(renderDistance, seed);
    m_mobSpawner.setSeed(seed);
    m_clock.reset();
    m_craftingGrid.assign(4, ItemStack());
    m_craftingOutput.clear();
    m_isContainerOpen = false;
    m_openContainerKind = OpenContainerKind::None;
    m_connectedPlayers.clear();
    m_playerNames.clear();
    m_playerNames[m_player.id] = "Host";
    m_mobs.clear();
    m_mobs.reserve(256);
    m_projectiles.clear();
    m_projectiles.reserve(256);
    m_itemEntities.clear();
    m_itemEntities.reserve(256);
    for (auto& parked : m_parkedDimensions) {
        parked.reset();
    }
    m_currentDimension = 0;
    m_player.dimension = 0;
}

PlayerEntity* WorldSession::addPlayer(EntityId id, const std::string& name, const Math::Vec3& spawnPos) {
    if (id == m_player.id || id == LOCAL_PLAYER_ENTITY_ID) {
        m_player.position = spawnPos;
        m_playerNames[m_player.id] = name;
        return &m_player;
    }

    // Si ya está conectado, mantener su inventario y actualizar posición/nombre
    auto connIt = m_connectedPlayers.find(id);
    if (connIt != m_connectedPlayers.end()) {
        connIt->second.position = spawnPos;
        m_playerNames[id] = name;
        return &connIt->second;
    }

    // Si estaba en estado offline, restaurarlo manteniendo su inventario completo
    auto offIt = m_offlinePlayers.find(id);
    if (offIt != m_offlinePlayers.end()) {
        PlayerEntity p = std::move(offIt->second);
        m_offlinePlayers.erase(offIt);
        m_offlinePlayerNames.erase(id);
        p.position = spawnPos;
        m_playerNames[id] = name;
        auto [it, _] = m_connectedPlayers.insert_or_assign(id, std::move(p));
        return &it->second;
    }

    PlayerEntity p;
    p.id = id;
    p.position = spawnPos;
    p.prevPosition = spawnPos;
    p.health = 20.0f;
    p.hunger = 20.0f;
    p.saturation = 5.0f;
    p.inventory.resize(36);
    m_playerNames[id] = name;
    auto [it, inserted] = m_connectedPlayers.insert_or_assign(id, std::move(p));
    return &it->second;
}

bool WorldSession::removePlayer(EntityId id) {
    auto it = m_connectedPlayers.find(id);
    if (it != m_connectedPlayers.end()) {
        auto nameIt = m_playerNames.find(id);
        if (nameIt != m_playerNames.end()) {
            m_offlinePlayerNames[id] = nameIt->second;
            m_playerNames.erase(nameIt);
        }
        m_offlinePlayers[id] = std::move(it->second);
        m_connectedPlayers.erase(it);
        return true;
    }
    return false;
}

PlayerEntity* WorldSession::getPlayerById(EntityId id) noexcept {
    auto it = m_connectedPlayers.find(id);
    if (it != m_connectedPlayers.end()) {
        return &it->second;
    }
    auto offIt = m_offlinePlayers.find(id);
    if (offIt != m_offlinePlayers.end()) {
        return &offIt->second;
    }
    if (id == m_player.id || id == LOCAL_PLAYER_ENTITY_ID) {
        return &m_player;
    }
    return nullptr;
}

const PlayerEntity* WorldSession::getPlayerById(EntityId id) const noexcept {
    auto it = m_connectedPlayers.find(id);
    if (it != m_connectedPlayers.end()) {
        return &it->second;
    }
    auto offIt = m_offlinePlayers.find(id);
    if (offIt != m_offlinePlayers.end()) {
        return &offIt->second;
    }
    if (id == m_player.id || id == LOCAL_PLAYER_ENTITY_ID) {
        return &m_player;
    }
    return nullptr;
}

static const std::string s_defaultPlayerName = "Player";

PlayerEntity* WorldSession::getPlayerByName(const std::string& name) noexcept {
    EntityId id = getPlayerIdByName(name);
    if (id != INVALID_ENTITY_ID) {
        return getPlayerById(id);
    }
    return nullptr;
}

const PlayerEntity* WorldSession::getPlayerByName(const std::string& name) const noexcept {
    EntityId id = getPlayerIdByName(name);
    if (id != INVALID_ENTITY_ID) {
        return getPlayerById(id);
    }
    return nullptr;
}

EntityId WorldSession::getPlayerIdByName(const std::string& name) const noexcept {
    for (const auto& [id, pName] : m_playerNames) {
        if (pName == name) return id;
    }
    for (const auto& [id, pName] : m_offlinePlayerNames) {
        if (pName == name) return id;
    }
    if (name == s_defaultPlayerName ||
        (m_playerNames.find(m_player.id) != m_playerNames.end() && m_playerNames.at(m_player.id) == name)) {
        return m_player.id;
    }
    return INVALID_ENTITY_ID;
}

const std::string& WorldSession::getPlayerName(EntityId id) const {
    auto it = m_playerNames.find(id);
    if (it != m_playerNames.end()) {
        return it->second;
    }
    auto offIt = m_offlinePlayerNames.find(id);
    if (offIt != m_offlinePlayerNames.end()) {
        return offIt->second;
    }
    return s_defaultPlayerName;
}

void WorldSession::updateCraftingOutput() {
    int w = (m_craftingGrid.size() == 9) ? 3 : 2;
    int h = w;
    m_craftingOutput = RecipeRegistry::global().matchCrafting(m_craftingGrid, w, h);
}

void WorldSession::returnCraftingGridToInventory() {
    for (auto& stack : m_craftingGrid) {
        if (!stack.isEmpty()) {
            m_player.inventory.addItem(stack);
            if (!stack.isEmpty()) {
                spawnDroppedItem(stack, m_player.position + Math::Vec3(0, 0.5f, 0), {0, 1, 0}, 10);
                stack.clear();
            }
        }
    }
    m_craftingGrid.assign(4, ItemStack());
    m_craftingOutput.clear();
    m_player.syncHotbar();
}

void WorldSession::returnCursorItemToInventory() {
    if (!m_player.cursorItem.isEmpty()) {
        m_player.inventory.addItem(m_player.cursorItem);
        if (!m_player.cursorItem.isEmpty()) {
            // Inventario lleno: soltar lo restante en el mundo para evitar pérdida
            spawnDroppedItem(m_player.cursorItem, m_player.position + Math::Vec3(0, 0.5f, 0), {0, 1, 0}, 10);
            m_player.cursorItem.clear();
        }
        m_player.syncHotbar();
    }
}

ItemEntity* WorldSession::spawnDroppedItem(ItemStack stack, const Math::Vec3& pos, const Math::Vec3& vel,
                                           uint32_t delay) {
    if (stack.isEmpty()) return nullptr;
    EntityId id = m_nextEntityId++;
    m_itemEntities.emplace_back(id, std::move(stack), pos, vel, delay);
    m_pendingEvents.push_back(ItemSpawnedEvent{.entityId = id,
                                               .itemId = m_itemEntities.back().stack.itemId,
                                               .count = m_itemEntities.back().stack.count,
                                               .position = pos});
    return &m_itemEntities.back();
}

MobEntity* WorldSession::spawnMob(MobType type, const Math::Vec3& pos) {
    EntityId id = m_nextEntityId++;
    m_mobs.emplace_back(id, type, pos);
    m_pendingEvents.push_back(MobSpawnedEvent{.entityId = id, .mobType = mobTypeToString(type), .position = pos});
    return &m_mobs.back();
}

ProjectileEntity* WorldSession::spawnProjectile(EntityId shooterId, const Math::Vec3& pos, const Math::Vec3& vel,
                                                float damage) {
    EntityId id = m_nextEntityId++;
    m_projectiles.emplace_back(id, shooterId, pos, vel, damage);
    m_pendingEvents.push_back(ProjectileSpawnedEvent{.entityId = id, .position = pos, .velocity = vel});
    return &m_projectiles.back();
}

ExperienceOrbEntity* WorldSession::spawnExperienceOrb(const Math::Vec3& pos, uint32_t value) {
    if (value == 0) return nullptr;
    EntityId id = m_nextEntityId++;
    m_experienceOrbs.emplace_back(id, pos, value);
    return &m_experienceOrbs.back();
}

BoatEntity* WorldSession::spawnBoat(const Math::Vec3& pos) {
    EntityId id = m_nextEntityId++;
    m_boats.emplace_back(id, pos);
    return &m_boats.back();
}

MinecartEntity* WorldSession::spawnMinecart(const Math::Vec3& pos, MinecartType type) {
    EntityId id = m_nextEntityId++;
    m_minecarts.emplace_back(id, pos, type);
    return &m_minecarts.back();
}

static PlayerSaveData serializePlayerData(const PlayerEntity& player, const std::string& name,
                                          const AdvancementManager* advMgr = nullptr,
                                          const StatisticsManager* statMgr = nullptr) {
    auto& reg = BlockRegistry::global();
    auto& itemReg = ItemRegistry::global();

    PlayerSaveData playerData;
    playerData.schemaVersion = CURRENT_SCHEMA_VERSION;
    playerData.entityId = player.id;
    playerData.playerName = name;
    playerData.position = player.position;
    playerData.velocity = player.velocity;
    playerData.yaw = player.yaw;
    playerData.pitch = player.pitch;
    playerData.isGrounded = player.isGrounded;
    playerData.health = player.health;
    playerData.hunger = player.hunger;
    playerData.saturation = player.saturation;
    playerData.exhaustion = player.exhaustion;
    playerData.air = player.air;
    playerData.gameMode = player.gameMode;
    playerData.spawnPoint = player.spawnPoint;
    playerData.hasCustomSpawnPoint = player.hasCustomSpawnPoint;
    playerData.isAlive = player.isAlive;
    playerData.selectedSlot = static_cast<uint32_t>(player.selectedSlot);

    playerData.hotbarNames.clear();
    for (BlockStateId slotState : player.hotbar) {
        playerData.hotbarNames.push_back(reg.getNamespacedId(slotState));
    }

    // Esquema v3: Serializar inventario completo (36 ranuras), cursor y offhand
    playerData.inventorySlots.clear();
    playerData.inventorySlots.reserve(player.inventory.getSize());
    for (size_t i = 0; i < player.inventory.getSize(); ++i) {
        const auto& stack = player.inventory.getSlot(i);
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
        playerData.inventorySlots.push_back(std::move(isData));
    }

    if (!player.cursorItem.isEmpty()) {
        playerData.cursorItem = ItemStackSaveData{.itemName = itemReg.get(player.cursorItem.itemId).namespacedId,
                                                  .count = player.cursorItem.count,
                                                  .durability = player.cursorItem.durability,
                                                  .components = player.cursorItem.components};
    } else {
        playerData.cursorItem =
            ItemStackSaveData{.itemName = "opencraft:air", .count = 0, .durability = 0, .components = {}};
    }

    if (!player.offhandItem.isEmpty()) {
        playerData.offhandItem = ItemStackSaveData{.itemName = itemReg.get(player.offhandItem.itemId).namespacedId,
                                                   .count = player.offhandItem.count,
                                                   .durability = player.offhandItem.durability,
                                                   .components = player.offhandItem.components};
    } else {
        playerData.offhandItem =
            ItemStackSaveData{.itemName = "opencraft:air", .count = 0, .durability = 0, .components = {}};
    }

    // Esquema v7: Experiencia y Armadura
    playerData.xpTotal = player.xpTotal;
    playerData.xpLevel = player.xpLevel;
    playerData.xpProgress = player.xpProgress;
    playerData.armorSlots.clear();
    for (const auto& piece : player.armorSlots) {
        if (!piece.isEmpty()) {
            playerData.armorSlots.push_back(ItemStackSaveData{.itemName = itemReg.get(piece.itemId).namespacedId,
                                                              .count = piece.count,
                                                              .durability = piece.durability,
                                                              .components = piece.components});
        } else {
            playerData.armorSlots.push_back(
                ItemStackSaveData{.itemName = "opencraft:air", .count = 0, .durability = 0, .components = {}});
        }
    }

    playerData.currentDimension = player.dimension;
    playerData.isGliding = player.isGliding;
    playerData.activeEffects.clear();
    for (const auto& eff : player.statusEffects.getActiveEffects()) {
        playerData.activeEffects.push_back(PlayerSaveData::ActiveEffectSaveData{.type = static_cast<uint8_t>(eff.type),
                                                                                .durationTicks = eff.durationTicks,
                                                                                .amplifier = eff.amplifier,
                                                                                .isAmbient = eff.isAmbient});
    }

    if (advMgr) {
        playerData.unlockedAdvancements = advMgr->exportCompletedAdvancements();
    }
    if (statMgr) {
        playerData.generalStatistics.clear();
        for (const auto& st : statMgr->getAllGeneral()) {
            playerData.generalStatistics.push_back(st);
        }
    }

    return playerData;
}

static void deserializePlayerData(const PlayerSaveData& playerData, PlayerEntity& player,
                                  AdvancementManager* advMgr = nullptr, StatisticsManager* statMgr = nullptr) {
    player.position = playerData.position;
    player.prevPosition = player.position;
    player.velocity = playerData.velocity;
    player.yaw = playerData.yaw;
    player.pitch = playerData.pitch;
    player.isGrounded = playerData.isGrounded;
    player.health = playerData.health;
    player.hunger = playerData.hunger;
    player.saturation = playerData.saturation;
    player.exhaustion = playerData.exhaustion;
    player.air = playerData.air;
    player.gameMode = playerData.gameMode;
    player.spawnPoint = playerData.spawnPoint;
    player.hasCustomSpawnPoint = playerData.hasCustomSpawnPoint;
    player.isAlive = playerData.isAlive;
    player.selectedSlot = playerData.selectedSlot;
    player.isGliding = playerData.isGliding;
    player.dimension = (playerData.currentDimension <= 2) ? playerData.currentDimension : 0;
    player.statusEffects.clearAllEffects();
    for (const auto& eff : playerData.activeEffects) {
        player.statusEffects.addEffect(ActiveEffect{.type = static_cast<EffectType>(eff.type),
                                                    .durationTicks = eff.durationTicks,
                                                    .amplifier = eff.amplifier,
                                                    .isAmbient = eff.isAmbient});
    }

    auto& reg = BlockRegistry::global();
    auto& itemReg = ItemRegistry::global();

    player.hotbar.clear();
    for (const auto& name : playerData.hotbarNames) {
        player.hotbar.push_back(reg.getStateId(name));
    }

    // Cargar inventario completo si viene de Esquema v3
    if (!playerData.inventorySlots.empty()) {
        player.inventory.resize(playerData.inventorySlots.size());
        for (size_t i = 0; i < playerData.inventorySlots.size(); ++i) {
            const auto& sdata = playerData.inventorySlots[i];
            if (!sdata.itemName.empty() && sdata.itemName != "opencraft:air" && sdata.count > 0) {
                ItemId id = itemReg.getId(sdata.itemName);
                if (id == EMPTY_ITEM_ID) {
                    id = itemReg.getItemForBlock(reg.getStateId(sdata.itemName));
                }
                if (id != EMPTY_ITEM_ID) {
                    player.inventory.setSlot(i, ItemStack(id, sdata.count, sdata.durability, sdata.components));
                } else {
                    player.inventory.setSlot(i, ItemStack());
                }
            } else {
                player.inventory.setSlot(i, ItemStack());
            }
        }

        if (!playerData.cursorItem.itemName.empty() && playerData.cursorItem.itemName != "opencraft:air") {
            ItemId cid = itemReg.getId(playerData.cursorItem.itemName);
            if (cid != EMPTY_ITEM_ID) {
                player.cursorItem = ItemStack(cid, playerData.cursorItem.count, playerData.cursorItem.durability,
                                              playerData.cursorItem.components);
            }
        } else {
            player.cursorItem.clear();
        }

        if (!playerData.offhandItem.itemName.empty() && playerData.offhandItem.itemName != "opencraft:air") {
            ItemId oid = itemReg.getId(playerData.offhandItem.itemName);
            if (oid != EMPTY_ITEM_ID) {
                player.offhandItem = ItemStack(oid, playerData.offhandItem.count, playerData.offhandItem.durability,
                                               playerData.offhandItem.components);
            }
        } else {
            player.offhandItem.clear();
        }

        if (playerData.hotbarNames.empty()) {
            player.syncHotbar();
        }
    } else {
        // Esquema heredado v1 y v2: reconstruir desde hotbarNames
        player.inventory.resize(36);
        for (size_t i = 0; i < playerData.hotbarNames.size() && i < 9; ++i) {
            BlockStateId st = reg.getStateId(playerData.hotbarNames[i]);
            ItemId itId = itemReg.getItemForBlock(st);
            if (itId != EMPTY_ITEM_ID) {
                player.inventory.setSlot(i, ItemStack(itId, 64));
            }
        }
        player.cursorItem.clear();
        player.offhandItem.clear();
        player.syncHotbar();
    }

    // Esquema v7: Experiencia y Armadura
    player.xpTotal = playerData.xpTotal;
    player.xpLevel = playerData.xpLevel;
    player.xpProgress = playerData.xpProgress;
    for (size_t a = 0; a < 4 && a < playerData.armorSlots.size(); ++a) {
        const auto& asd = playerData.armorSlots[a];
        if (!asd.itemName.empty() && asd.itemName != "opencraft:air") {
            ItemId aid = itemReg.getId(asd.itemName);
            if (aid != EMPTY_ITEM_ID) {
                player.armorSlots[a] = ItemStack(aid, asd.count, asd.durability, asd.components);
            }
        }
    }

    if (advMgr) {
        advMgr->loadCompletedAdvancements(playerData.unlockedAdvancements);
    }
    if (statMgr) {
        for (const auto& st : playerData.generalStatistics) {
            statMgr->incrementGeneral(st.first, st.second);
        }
    }
}

StorageError WorldSession::saveToStorage(WorldStorage& storage) {
    // Si hay un objeto en el cursor, devolverlo de forma segura al inventario antes de guardar
    returnCursorItemToInventory();

    WorldMetadata meta;
    if (storage.loadMetadata(meta) != StorageError::None) {
        meta.worldName = "OpenCraft World";
        meta.worldId = "world";
        meta.creationTime =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        meta.gameMode = 0;
        meta.difficulty = 1;
    }

    meta.schemaVersion = CURRENT_SCHEMA_VERSION;
    meta.generatorVersion = CURRENT_GENERATOR_VERSION;
    meta.seed = m_world.getSeed();
    meta.lastPlayedTime =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    meta.simulationTick = m_clock.getTickCount();

    auto& reg = BlockRegistry::global();
    auto& itemReg = ItemRegistry::global();
    meta.blockCatalog.clear();
    for (size_t i = 0; i < reg.getStateCount(); ++i) {
        meta.blockCatalog.push_back(reg.getNamespacedId(static_cast<BlockStateId>(i)));
    }

    meta.isHardcore = m_isHardcore;
    meta.gameRules.clear();
    for (const auto& r : m_commandEngine.getGameRules().getAll()) {
        meta.gameRules.push_back(r);
    }

    StorageError err = storage.beginTransaction();
    if (err != StorageError::None) return err;

    err = storage.stageMetadata(meta);
    if (err != StorageError::None) {
        storage.rollbackTransaction();
        return err;
    }

    // 1. Guardar jugador local (host)
    m_player.dimension = m_currentDimension;
    PlayerSaveData localPlayerData =
        serializePlayerData(m_player, getPlayerName(m_player.id), &m_advancementManager, &m_statisticsManager);
    err = storage.stagePlayer(localPlayerData);
    if (err != StorageError::None) {
        storage.rollbackTransaction();
        return err;
    }

    // 2. Guardar todos los jugadores remotos (conectados y offline)
    std::unordered_map<EntityId, const PlayerEntity*> allRemotePlayers;
    for (const auto& [pid, pEntity] : m_connectedPlayers) {
        if (pid != m_player.id && pid != LOCAL_PLAYER_ENTITY_ID) {
            allRemotePlayers[pid] = &pEntity;
        }
    }
    for (const auto& [pid, pEntity] : m_offlinePlayers) {
        if (pid != m_player.id && pid != LOCAL_PLAYER_ENTITY_ID) {
            allRemotePlayers[pid] = &pEntity;
        }
    }

    for (const auto& [pid, pPtr] : allRemotePlayers) {
        PlayerSaveData remData = serializePlayerData(*pPtr, getPlayerName(pid));
        remData.entityId = pid;
        err = storage.stagePlayer(remData, pid);
        if (err != StorageError::None) {
            storage.rollbackTransaction();
            return err;
        }
    }

    // 3. Chunks y criaturas de la dimensión activa y de las dimensiones aparcadas
    std::vector<Chunk*> savedChunks;
    std::vector<std::pair<std::tuple<int, int, int>, bool>> mobChunks;
    err = stageDimensionChunks(storage, m_world, m_mobs, static_cast<DimensionType>(m_currentDimension), savedChunks,
                               mobChunks);
    if (err != StorageError::None) {
        storage.rollbackTransaction();
        return err;
    }
    for (size_t d = 0; d < m_parkedDimensions.size(); ++d) {
        const auto& parked = m_parkedDimensions[d];
        if (!parked || d == m_currentDimension) continue;
        err = stageDimensionChunks(storage, parked->world, parked->mobs, static_cast<DimensionType>(d), savedChunks,
                                   mobChunks);
        if (err != StorageError::None) {
            storage.rollbackTransaction();
            return err;
        }
    }

    // 4. Objetos tirados de todas las dimensiones residentes
    std::vector<ItemEntitySaveData> entityData;
    entityData.reserve(m_itemEntities.size());
    auto collectItems = [&](const std::vector<ItemEntity>& items, uint8_t dim) {
        for (const auto& it : items) {
            if (!it.isDead && !it.stack.isEmpty()) {
                ItemEntitySaveData itemData;
                itemData.id = it.id;
                itemData.position = it.position;
                itemData.velocity = it.velocity;
                itemData.pickupDelayTicks = it.pickupDelayTicks;
                itemData.ageTicks = it.ageTicks;
                itemData.stack.itemName = itemReg.get(it.stack.itemId).namespacedId;
                itemData.stack.count = it.stack.count;
                itemData.stack.durability = it.stack.durability;
                itemData.stack.components = it.stack.components;
                itemData.dimension = dim;
                entityData.push_back(std::move(itemData));
            }
        }
    };
    collectItems(m_itemEntities, m_currentDimension);
    for (size_t d = 0; d < m_parkedDimensions.size(); ++d) {
        if (m_parkedDimensions[d] && d != m_currentDimension) {
            collectItems(m_parkedDimensions[d]->itemEntities, static_cast<uint8_t>(d));
        }
    }
    err = storage.stageEntities(entityData);
    if (err != StorageError::None) {
        storage.rollbackTransaction();
        return err;
    }

    err = storage.commitTransaction();
    if (err == StorageError::None) {
        for (Chunk* c : savedChunks) {
            c->clearDirty();
        }
        for (const auto& [key, hasMobs] : mobChunks) {
            if (hasMobs)
                m_chunksWithSavedMobs.insert(key);
            else
                m_chunksWithSavedMobs.erase(key);
        }
        m_attachedStorage = &storage;
    }
    return err;
}

StorageError WorldSession::loadFromStorage(WorldStorage& storage) {
    WorldMetadata meta;
    StorageError err = storage.loadMetadata(meta);
    if (err != StorageError::None) return err;

    m_world.setSeed(meta.seed);
    m_clock.setTickCount(meta.simulationTick);
    m_isHardcore = meta.isHardcore;
    if (m_isHardcore) {
        m_difficulty = 3;
    }
    for (const auto& r : meta.gameRules) {
        m_commandEngine.getGameRules().set(r.first, r.second);
    }

    PlayerSaveData playerData;
    if (storage.loadPlayer(playerData) == StorageError::None) {
        deserializePlayerData(playerData, m_player, &m_advancementManager, &m_statisticsManager);
    }

    // Cargar jugadores remotos guardados
    m_connectedPlayers.clear();
    m_offlinePlayers.clear();
    std::vector<uint32_t> savedPids = storage.getSavedPlayerIds();
    for (uint32_t pid : savedPids) {
        if (pid == m_player.id || pid == LOCAL_PLAYER_ENTITY_ID) continue;
        PlayerSaveData remData;
        if (storage.loadPlayer(remData, pid) == StorageError::None) {
            std::string pName = remData.playerName.empty() ? ("Player_" + std::to_string(pid)) : remData.playerName;
            PlayerEntity* rp = addPlayer(pid, pName, remData.position);
            if (rp) {
                deserializePlayerData(remData, *rp);
            }
        }
    }

    // Activar la dimensión guardada del jugador local: mundo vacío poblado alrededor del jugador
    // desde el almacenamiento de esa dimensión, generando solo los chunks que no existen en disco.
    m_currentDimension = m_player.dimension;
    for (auto& parked : m_parkedDimensions) {
        parked.reset();
    }
    {
        World fresh;
        fresh.setSeed(meta.seed);
        fresh.setRenderDistance(m_world.getRenderDistance());
        fresh.setSimulationDistance(m_world.getSimulationDistance());
        fresh.setGeneratorVersion(m_world.getGeneratorVersion());
        m_world = std::move(fresh);
    }
    m_mobs.clear();
    m_projectiles.clear();
    m_experienceOrbs.clear();
    m_boats.clear();
    m_minecarts.clear();
    m_scheduledUpdates.clear();
    m_cinnabarEngine.clear();
    // Área de aparición (origen) y área alrededor del jugador
    int playerChunkX = floorDiv16(static_cast<int>(std::floor(m_player.position.x)));
    int playerChunkZ = floorDiv16(static_cast<int>(std::floor(m_player.position.z)));
    auto activeDim = static_cast<DimensionType>(m_currentDimension);
    err = populateDimensionArea(m_world, m_mobs, activeDim, 0, 0, &storage);
    if (err == StorageError::None) {
        // Alrededor del jugador solo se recupera lo guardado; el streaming genera el resto
        err = populateDimensionArea(m_world, m_mobs, activeDim, playerChunkX, playerChunkZ, &storage, false);
    }
    if (err != StorageError::None) {
        return err;
    }
    m_attachedStorage = &storage;

    std::vector<ItemEntitySaveData> entityData;
    err = storage.loadEntities(entityData);
    if (err != StorageError::None) {
        return err;
    }

    auto& itemReg = ItemRegistry::global();
    m_itemEntities.clear();
    for (const auto& itemData : entityData) {
        ItemId itId = itemReg.getId(itemData.stack.itemName);
        if (itId == EMPTY_ITEM_ID && itemData.stack.itemName != "opencraft:air") {
            continue;
        }
        ItemStack stack(itId, itemData.stack.count, itemData.stack.durability, itemData.stack.components);
        ItemEntity ie(itemData.id, std::move(stack), itemData.position, itemData.velocity, itemData.pickupDelayTicks);
        ie.ageTicks = itemData.ageTicks;
        if (itemData.dimension == m_currentDimension) {
            m_itemEntities.push_back(std::move(ie));
        } else {
            // Objetos de otra dimensión: quedan aparcados hasta que esa dimensión se active
            getOrCreateParkedDimension(static_cast<DimensionType>(itemData.dimension))
                .itemEntities.push_back(std::move(ie));
        }
        if (itemData.id >= m_nextEntityId) {
            m_nextEntityId = itemData.id + 1;
        }
    }

    return StorageError::None;
}

StorageError
WorldSession::stageDimensionChunks(WorldStorage& storage, const World& world, const std::vector<MobEntity>& mobs,
                                   DimensionType dim, std::vector<Chunk*>& outStaged,
                                   std::vector<std::pair<std::tuple<int, int, int>, bool>>& outMobChunks) const {
    // Coordenadas de chunk con criaturas vivas (una pasada en lugar de una por chunk)
    std::set<std::pair<int, int>> occupied;
    for (const auto& mob : mobs) {
        if (mob.isAlive && !mob.isDead) {
            occupied.insert({static_cast<int>(std::floor(mob.position.x / 16.0f)),
                             static_cast<int>(std::floor(mob.position.z / 16.0f))});
        }
    }
    for (const auto& [coord, chunkPtr] : world.getChunks()) {
        if (!chunkPtr) continue;
        auto key = std::make_tuple(static_cast<int>(dim), coord.x, coord.z);
        bool hasMobs = occupied.contains({coord.x, coord.z});
        // Solo se escriben chunks modificados o cuya lista de criaturas guardada pueda haber cambiado:
        // reescribir todo lo cargado en cada autoguardado eran decenas de MB sincronizados a disco
        if (!chunkPtr->isDirty() && !hasMobs && !m_chunksWithSavedMobs.contains(key)) continue;
        outMobChunks.emplace_back(key, hasMobs);
        std::vector<MobSaveData> chunkMobs;
        for (const auto& mob : mobs) {
            if (mob.isAlive && !mob.isDead) {
                int mcx = static_cast<int>(std::floor(mob.position.x / 16.0f));
                int mcz = static_cast<int>(std::floor(mob.position.z / 16.0f));
                if (mcx == chunkPtr->getChunkX() && mcz == chunkPtr->getChunkZ()) {
                    chunkMobs.push_back(
                        MobSaveData{.mobType = mobTypeToString(mob.mobType),
                                    .position = mob.position,
                                    .velocity = mob.velocity,
                                    .yaw = mob.yaw,
                                    .pitch = mob.pitch,
                                    .health = mob.health,
                                    .aiState = static_cast<uint8_t>(mob.aiState),
                                    .isPersistent = mob.isPersistent,
                                    .inLove = mob.inLove,
                                    .breedingCooldown = mob.breedingCooldown,
                                    .isBaby = mob.isBaby,
                                    .isTamed = mob.isTamed,
                                    .ownerId = mob.ownerPlayerId,
                                    .isSitting = mob.isSitting,
                                    .villagerProfession = static_cast<uint8_t>(mob.villagerData.profession)});
                }
            }
        }
        StorageError err = storage.stageChunk(*chunkPtr, chunkMobs, dim);
        if (err != StorageError::None) {
            return err;
        }
        outStaged.push_back(chunkPtr.get());
    }
    return StorageError::None;
}

StorageError WorldSession::populateDimensionArea(World& world, std::vector<MobEntity>& mobs, DimensionType dim,
                                                 int centerChunkX, int centerChunkZ, WorldStorage* storage,
                                                 bool generateMissing) {
    auto& reg = BlockRegistry::global();
    int rd = world.getRenderDistance();
    for (int cx = centerChunkX - rd; cx <= centerChunkX + rd; ++cx) {
        for (int cz = centerChunkZ - rd; cz <= centerChunkZ + rd; ++cz) {
            if (world.isChunkLoaded(cx, cz)) continue;

            bool onDisk = storage && storage->hasChunk(cx, cz, dim);
            if (!onDisk && !generateMissing) continue;

            auto chunk = std::make_unique<Chunk>(cx, cz);
            if (onDisk) {
                // Un chunk existente que no se puede leer aborta la operación: nunca se regenera encima
                std::vector<MobSaveData> chunkMobs;
                StorageError cerr = storage->loadChunk(cx, cz, *chunk, reg, &chunkMobs, dim);
                if (cerr != StorageError::None) {
                    return cerr;
                }
                if (!chunkMobs.empty()) {
                    m_chunksWithSavedMobs.insert(std::make_tuple(static_cast<int>(dim), cx, cz));
                }
                for (const auto& msd : chunkMobs) {
                    MobEntity mob(m_nextEntityId++, mobTypeFromString(msd.mobType), msd.position);
                    mob.velocity = msd.velocity;
                    mob.yaw = msd.yaw;
                    mob.pitch = msd.pitch;
                    mob.health = msd.health;
                    mob.aiState = static_cast<AIState>(msd.aiState);
                    mob.isPersistent = msd.isPersistent;
                    mob.inLove = msd.inLove;
                    mob.breedingCooldown = msd.breedingCooldown;
                    mob.isBaby = msd.isBaby;
                    mob.isTamed = msd.isTamed;
                    mob.ownerPlayerId = msd.ownerId;
                    mob.isSitting = msd.isSitting;
                    if (mob.mobType == MobType::Villager) {
                        mob.villagerData.assignProfession(static_cast<VillagerProfession>(msd.villagerProfession));
                    }
                    mobs.push_back(std::move(mob));
                }
            } else if (dim == DimensionType::Overworld) {
                chunk->generateTerrain(world.getSeed());
            } else {
                Dimension::generateTerrain(dim, *chunk, world.getSeed(), world.getGeneratorVersion());
            }
            world.setChunk(cx, cz, std::move(chunk));
        }
    }
    return StorageError::None;
}

WorldSession::ParkedDimension& WorldSession::getOrCreateParkedDimension(DimensionType dim) {
    auto& slot = m_parkedDimensions[static_cast<size_t>(dim)];
    if (!slot) {
        slot = std::make_unique<ParkedDimension>();
        slot->world.setSeed(m_world.getSeed());
        slot->world.setRenderDistance(m_world.getRenderDistance());
        slot->world.setSimulationDistance(m_world.getSimulationDistance());
        slot->world.setGeneratorVersion(m_world.getGeneratorVersion());
    }
    return *slot;
}

void WorldSession::swapActiveDimension(ParkedDimension& slot) {
    std::swap(m_world, slot.world);
    std::swap(m_itemEntities, slot.itemEntities);
    std::swap(m_mobs, slot.mobs);
    std::swap(m_projectiles, slot.projectiles);
    std::swap(m_experienceOrbs, slot.experienceOrbs);
    std::swap(m_boats, slot.boats);
    std::swap(m_minecarts, slot.minecarts);
    std::swap(m_scheduledUpdates, slot.scheduledUpdates);
    std::swap(m_cinnabarEngine, slot.cinnabarEngine);
}

bool WorldSession::isDimensionResident(DimensionType dim) const noexcept {
    size_t idx = static_cast<size_t>(dim);
    return idx == m_currentDimension || (idx < m_parkedDimensions.size() && m_parkedDimensions[idx] != nullptr);
}

bool WorldSession::setCurrentDimension(uint8_t dim) {
    if (dim > 2) return false;
    return changeDimension(static_cast<DimensionType>(dim), m_player.position);
}

bool WorldSession::changeDimension(DimensionType target, const Math::Vec3& targetPos) {
    uint8_t targetIdx = static_cast<uint8_t>(target);
    if (targetIdx > 2) return false;

    if (targetIdx != m_currentDimension) {
        // Materializar el destino antes de tocar la dimensión activa: si falla la lectura de un
        // chunk existente, la sesión queda intacta y el jugador no se mueve.
        ParkedDimension& incoming = getOrCreateParkedDimension(target);
        int cx = floorDiv16(static_cast<int>(std::floor(targetPos.x)));
        int cz = floorDiv16(static_cast<int>(std::floor(targetPos.z)));
        StorageError err = populateDimensionArea(incoming.world, incoming.mobs, target, cx, cz, m_attachedStorage);
        if (err != StorageError::None) {
            Log::error("No se pudo cargar la dimensión {} para la transferencia: {}", dimensionTypeToString(target),
                       storageErrorToString(err));
            return false;
        }

        // Aparcar la dimensión actual y activar la de destino
        uint8_t sourceIdx = m_currentDimension;
        // Tras el intercambio, la ranura contiene el estado de la dimensión de origen
        std::unique_ptr<ParkedDimension> slot = std::move(m_parkedDimensions[targetIdx]);
        swapActiveDimension(*slot);
        m_parkedDimensions[sourceIdx] = std::move(slot);
        m_currentDimension = targetIdx;

        // Las interacciones abiertas apuntaban a bloques de la dimensión anterior
        m_isMining = false;
        m_miningProgress = 0.0f;
        m_isContainerOpen = false;
        m_openContainerKind = OpenContainerKind::None;
        returnCursorItemToInventory();
    }

    PortalSystem::transferPlayer(m_player, target, targetPos);
    // La sesión simula una única dimensión activa: los jugadores remotos acompañan al anfitrión
    for (auto& [pid, remote] : m_connectedPlayers) {
        if (pid != m_player.id) {
            PortalSystem::transferPlayer(remote, target, targetPos);
        }
    }
    return true;
}

void WorldSession::tickPortals() {
    if (m_player.portalCooldown > 0) {
        --m_player.portalCooldown;
    }
    if (!m_player.isAlive || m_player.isSpectator()) {
        m_player.portalTicks = 0;
        return;
    }

    int bx = static_cast<int>(std::floor(m_player.position.x));
    int by = static_cast<int>(std::floor(m_player.position.y));
    int bz = static_cast<int>(std::floor(m_player.position.z));
    BlockID feet = m_world.getBlock(bx, by, bz);
    BlockID head = m_world.getBlock(bx, by + 1, bz);
    auto current = static_cast<DimensionType>(m_currentDimension);

    if (feet == BlockID::RimPortal) {
        if (m_player.portalCooldown > 0) return;
        if (current == DimensionType::Rim) {
            // Portal de salida tras derrotar al dragón: regreso al punto de aparición del Overworld
            Math::Vec3 spawn = m_player.spawnPoint;
            if (changeDimension(DimensionType::Overworld, spawn)) {
                m_player.position = findSafeSpawn(spawn);
                m_player.prevPosition = m_player.position;
            }
        } else {
            if (changeDimension(DimensionType::Rim, Math::Vec3(100.0f, 49.0f, 0.0f))) {
                m_player.position = PortalSystem::setupEndSpawnPlatform(m_world);
                m_player.prevPosition = m_player.position;
            }
        }
        return;
    }

    if (feet != BlockID::AbyssPortal && head != BlockID::AbyssPortal) {
        m_player.portalTicks = 0;
        return;
    }
    if (current == DimensionType::Rim || m_player.portalCooldown > 0) return;

    ++m_player.portalTicks;
    uint32_t required = m_player.isCreative() ? 1u : PortalSystem::ABYSS_PORTAL_WAIT_TICKS;
    if (m_player.portalTicks < required) return;

    DimensionType target = (current == DimensionType::Abyss) ? DimensionType::Overworld : DimensionType::Abyss;
    Math::Vec3 mapped = PortalSystem::mapCoordinates(current, target, m_player.position);
    if (changeDimension(target, mapped)) {
        m_player.position = PortalSystem::findOrCreateAbyssPortalDestination(m_world, mapped);
        m_player.prevPosition = m_player.position;
    }
}

void WorldSession::queueCommand(const GameCommand& cmd) { m_commandQueue.push_back(cmd); }

void WorldSession::addEventListener(IGameEventListener* listener) {
    if (listener) {
        m_listeners.push_back(listener);
    }
}

void WorldSession::removeEventListener(IGameEventListener* listener) { std::erase(m_listeners, listener); }

void WorldSession::dispatchEvents() {
    for (const auto& ev : m_pendingEvents) {
        for (auto* listener : m_listeners) {
            if (listener) listener->onEvent(ev);
        }
    }
    m_pendingEvents.clear();
}

CommandResult WorldSession::executeCommand(const GameCommand& cmd) {
    return std::visit(
        [this](auto&& c) -> CommandResult {
            using T = std::decay_t<decltype(c)>;
            PlayerEntity* execPlayer = getPlayerById(c.entityId);
            if (!execPlayer) {
                execPlayer = &m_player;
            }

            if constexpr (std::is_same_v<T, MoveCommand>) {
                if (c.entityId == m_player.id || c.entityId == LOCAL_PLAYER_ENTITY_ID) {
                    m_pendingMoveInput = c.moveInput;
                    m_player.yaw = c.yaw;
                    m_player.pitch = c.pitch;
                    m_player.isSprinting = c.sprint && (m_player.hunger > 6.0f);
                    m_player.isSneaking = c.sneak;
                    if (c.jump) {
                        m_pendingJump = true;
                    }
                } else {
                    execPlayer->yaw = c.yaw;
                    execPlayer->pitch = c.pitch;
                    execPlayer->isSprinting = c.sprint && (execPlayer->hunger > 6.0f);
                    execPlayer->isSneaking = c.sneak;
                    // Desplazamiento remoto acotado a un paso por comando (un cliente no puede teletransportarse)
                    float lenSq = c.moveInput.dot(c.moveInput);
                    if (lenSq > 0.0001f) {
                        Math::Vec3 step = c.moveInput;
                        if (lenSq > 1.0f) step = step * (1.0f / std::sqrt(lenSq));
                        execPlayer->position = execPlayer->position + step;
                    }
                }
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, BreakBlockCommand>) {
                if (c.y < Chunk::MIN_Y || c.y >= Chunk::MAX_Y) {
                    return CommandResult::InvalidPosition;
                }

                Math::Vec3 eyePos = execPlayer->getEyePosition();
                Math::Vec3 blockCenter(static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.5f,
                                       static_cast<float>(c.z) + 0.5f);
                float distSq = (eyePos - blockCenter).dot(eyePos - blockCenter);
                if (distSq > (MAX_REACH_DISTANCE + 0.5f) * (MAX_REACH_DISTANCE + 0.5f)) {
                    return CommandResult::OutOfReach;
                }

                BlockStateId current = m_world.getBlockState(c.x, c.y, c.z);
                if (current == AIR_BLOCK_STATE) {
                    return CommandResult::InvalidBlock;
                }

                const auto& props = BlockRegistry::global().get(current);
                if (execPlayer->isSpectator() || execPlayer->isAdventure()) {
                    return CommandResult::Rejected;
                }
                if (props.legacyId == BlockID::Bedrock && execPlayer->isSurvival()) {
                    return CommandResult::Unbreakable;
                }

                // Consumo de durabilidad en la herramienta en mano
                auto& heldItem = (execPlayer->selectedSlot < execPlayer->inventory.getSize())
                                     ? execPlayer->inventory.getSlot(execPlayer->selectedSlot)
                                     : execPlayer->cursorItem;
                ToolType heldToolType = ToolType::None;
                ToolTier heldToolTier = ToolTier::None;
                if (!heldItem.isEmpty()) {
                    const auto& itemDef = ItemRegistry::global().get(heldItem.itemId);
                    heldToolType = itemDef.toolType;
                    heldToolTier = itemDef.toolTier;
                    if (itemDef.maxDurability > 0) {
                        heldItem.damage(1);
                        execPlayer->syncHotbar();
                    }
                }

                // Si es un cofre, vaciar su inventario soltando todos los ítems en el mundo
                if (props.legacyId == BlockID::Chest || props.namespacedId == "opencraft:chest") {
                    auto chest = m_world.getChestBlockEntity(c.x, c.y, c.z);
                    if (chest) {
                        for (size_t s = 0; s < chest->getInventory().getSize(); ++s) {
                            auto& stack = chest->getInventory().getSlot(s);
                            if (!stack.isEmpty()) {
                                spawnDroppedItem(stack, blockCenter, Math::Vec3(0.0f, 1.5f, 0.0f), 10);
                                stack.clear();
                            }
                        }
                    }
                    m_world.removeBlockEntity(c.x, c.y, c.z);
                    if (m_isContainerOpen && m_openContainerPos == BlockPos{c.x, c.y, c.z}) {
                        returnCursorItemToInventory();
                        returnCraftingGridToInventory();
                        m_isContainerOpen = false;
                        m_openContainerKind = OpenContainerKind::None;
                        m_pendingEvents.push_back(ContainerClosedEvent{.playerEntityId = m_player.id});
                    }
                }
                // Si es un horno, soltar sus 3 slots e ítem de horno
                else if (props.legacyId == BlockID::Furnace || props.namespacedId == "opencraft:furnace") {
                    auto furnace = m_world.getFurnaceBlockEntity(c.x, c.y, c.z);
                    if (furnace) {
                        for (size_t s = 0; s < furnace->getInventory().getSize(); ++s) {
                            auto& stack = furnace->getInventory().getSlot(s);
                            if (!stack.isEmpty()) {
                                spawnDroppedItem(stack, blockCenter, Math::Vec3(0.0f, 1.5f, 0.0f), 10);
                                stack.clear();
                            }
                        }
                    }
                    m_world.removeBlockEntity(c.x, c.y, c.z);
                    if (m_isContainerOpen && m_openContainerPos == BlockPos{c.x, c.y, c.z}) {
                        returnCursorItemToInventory();
                        returnCraftingGridToInventory();
                        m_isContainerOpen = false;
                        m_openContainerKind = OpenContainerKind::None;
                        m_pendingEvents.push_back(ContainerClosedEvent{.playerEntityId = m_player.id});
                    }
                } else if (props.legacyId == BlockID::CraftingTable ||
                           props.namespacedId == "opencraft:crafting_table") {
                    if (m_isContainerOpen && m_openContainerPos == BlockPos{c.x, c.y, c.z}) {
                        returnCursorItemToInventory();
                        returnCraftingGridToInventory();
                        m_isContainerOpen = false;
                        m_openContainerKind = OpenContainerKind::None;
                        m_pendingEvents.push_back(ContainerClosedEvent{.playerEntityId = m_player.id});
                    }
                }

                // Comprobar reglas de cosecha y botín (loot)
                bool canHarvest = (props.minHarvestTier == ToolTier::None);
                if (props.minHarvestTier != ToolTier::None) {
                    if (heldToolType == props.requiredTool &&
                        static_cast<int>(heldToolTier) >= static_cast<int>(props.minHarvestTier)) {
                        canHarvest = true;
                    }
                }

                if (canHarvest) {
                    ItemId dropId = EMPTY_ITEM_ID;
                    if (!props.dropItemName.empty()) {
                        if (props.dropItemName != "opencraft:air") {
                            dropId = ItemRegistry::global().getId(props.dropItemName);
                        }
                    } else {
                        dropId = ItemRegistry::global().getItemForBlock(current);
                    }
                    // Grava: 10 % de probabilidad de soltar pedernal (determinista por posición y tick)
                    if (props.legacyId == BlockID::Gravel) {
                        uint64_t roll = (static_cast<uint64_t>(static_cast<uint32_t>(c.x)) * 73856093ULL) ^
                                        (static_cast<uint64_t>(static_cast<uint32_t>(c.y)) * 19349663ULL) ^
                                        (static_cast<uint64_t>(static_cast<uint32_t>(c.z)) * 83492791ULL) ^
                                        m_clock.getTickCount();
                        if (roll % 10 == 0) {
                            ItemId flintId = ItemRegistry::global().getId("opencraft:flint");
                            if (flintId != EMPTY_ITEM_ID) dropId = flintId;
                        }
                    }
                    if (dropId != EMPTY_ITEM_ID) {
                        spawnDroppedItem(ItemStack(dropId, 1), blockCenter, Math::Vec3(0.0f, 1.5f, 0.0f), 10);
                    }
                }

                m_world.setBlockState(c.x, c.y, c.z, AIR_BLOCK_STATE);
                m_world.removeBlockEntity(c.x, c.y, c.z);
                m_statisticsManager.incrementMined(props.namespacedId);
                m_advancementManager.onBlockMined(props.namespacedId);
                m_cinnabarEngine.onBlockRemoved(m_world, {c.x, c.y, c.z}, props.legacyId, m_clock.getTickCount());
                notifyTerrainChange(c.x, c.y, c.z);

                // Gravedad de arena arriba
                if (c.y + 1 < Chunk::MAX_Y && m_world.getBlock(c.x, c.y + 1, c.z) == BlockID::Sand) {
                    m_scheduledUpdates.schedule({c.x, c.y + 1, c.z}, BlockID::Sand, m_clock.getTickCount() + 2, 0);
                }

                // Notificar vecinos con fluidos para flujo o actualización
                static constexpr std::array<BlockPos, 6> s_offsets = {
                    {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
                for (const auto& fo : s_offsets) {
                    BlockPos npos{c.x + fo.x, c.y + fo.y, c.z + fo.z};
                    BlockID nb = m_world.getBlock(npos.x, npos.y, npos.z);
                    if (FluidSimulator::isFluid(nb)) {
                        m_scheduledUpdates.schedule(npos, nb, m_clock.getTickCount() + 1, 0);
                    }
                }

                m_pendingEvents.push_back(BlockChangedEvent{.x = c.x,
                                                            .y = c.y,
                                                            .z = c.z,
                                                            .oldState = current,
                                                            .newState = AIR_BLOCK_STATE,
                                                            .causeEntityId = c.entityId});

                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, PlaceBlockCommand>) {
                if (c.y < Chunk::MIN_Y || c.y >= Chunk::MAX_Y) {
                    return CommandResult::InvalidPosition;
                }

                if (execPlayer->isSpectator() || execPlayer->isAdventure()) {
                    return CommandResult::Rejected;
                }

                Math::Vec3 eyePos = execPlayer->getEyePosition();
                Math::Vec3 blockCenter(static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.5f,
                                       static_cast<float>(c.z) + 0.5f);
                float distSq = (eyePos - blockCenter).dot(eyePos - blockCenter);
                if (distSq > (MAX_REACH_DISTANCE + 0.5f) * (MAX_REACH_DISTANCE + 0.5f)) {
                    return CommandResult::OutOfReach;
                }

                const auto& props = BlockRegistry::global().get(c.blockState);
                if (props.isSolid) {
                    Math::AABB blockBox{
                        .min = {static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z)},
                        .max = {static_cast<float>(c.x + 1), static_cast<float>(c.y + 1), static_cast<float>(c.z + 1)}};
                    if (execPlayer->getAABB().intersects(blockBox)) {
                        return CommandResult::Collision;
                    }
                }

                BlockStateId current = m_world.getBlockState(c.x, c.y, c.z);
                // Solo se coloca en huecos: aire, fluidos, fuego o nieve fina (nunca sobre otro bloque)
                {
                    const auto& cur = BlockRegistry::global().get(current);
                    bool replaceable = current == AIR_BLOCK_STATE || FluidSimulator::isFluid(cur.legacyId) ||
                                       cur.legacyId == BlockID::Fire || cur.namespacedId == "opencraft:snow";
                    if (!replaceable) {
                        return CommandResult::Collision;
                    }
                    if (c.blockState == AIR_BLOCK_STATE) {
                        return CommandResult::InvalidBlock;
                    }
                }
                m_world.setBlockState(c.x, c.y, c.z, c.blockState);
                notifyTerrainChange(c.x, c.y, c.z);

                // Bloques entidad: Cofre o Horno
                if (props.legacyId == BlockID::Chest || props.namespacedId == "opencraft:chest") {
                    m_world.setBlockEntity(c.x, c.y, c.z, std::make_shared<ChestBlockEntity>(BlockPos{c.x, c.y, c.z}));
                } else if (props.legacyId == BlockID::Furnace || props.namespacedId == "opencraft:furnace") {
                    m_world.setBlockEntity(c.x, c.y, c.z,
                                           std::make_shared<FurnaceBlockEntity>(BlockPos{c.x, c.y, c.z}));
                } else if (props.legacyId == BlockID::Hopper || props.namespacedId == "opencraft:hopper") {
                    m_world.setBlockEntity(c.x, c.y, c.z, std::make_shared<HopperBlockEntity>(BlockPos{c.x, c.y, c.z}));
                } else if (props.legacyId == BlockID::Dropper || props.namespacedId == "opencraft:dropper") {
                    m_world.setBlockEntity(c.x, c.y, c.z,
                                           std::make_shared<DropperBlockEntity>(BlockPos{c.x, c.y, c.z}));
                } else if (props.legacyId == BlockID::Dispenser || props.namespacedId == "opencraft:dispenser") {
                    m_world.setBlockEntity(c.x, c.y, c.z,
                                           std::make_shared<DispenserBlockEntity>(BlockPos{c.x, c.y, c.z}));
                } else if (props.legacyId == BlockID::Crafter || props.namespacedId == "opencraft:crafter") {
                    m_world.setBlockEntity(c.x, c.y, c.z,
                                           std::make_shared<CrafterBlockEntity>(BlockPos{c.x, c.y, c.z}));
                } else if (props.legacyId == BlockID::CommandBlock ||
                           props.legacyId == BlockID::RepeatingCommandBlock ||
                           props.legacyId == BlockID::ChainCommandBlock) {
                    auto cb = std::make_shared<CommandBlockEntity>(BlockPos{c.x, c.y, c.z});
                    if (props.legacyId == BlockID::RepeatingCommandBlock)
                        cb->setMode(CommandBlockMode::Repeating);
                    else if (props.legacyId == BlockID::ChainCommandBlock)
                        cb->setMode(CommandBlockMode::Chain);
                    m_world.setBlockEntity(c.x, c.y, c.z, cb);
                } else if (props.legacyId == BlockID::TrialSpawner || props.namespacedId == "opencraft:trial_spawner") {
                    m_world.setBlockEntity(c.x, c.y, c.z,
                                           std::make_shared<TrialSpawnerBlockEntity>(BlockPos{c.x, c.y, c.z}));
                } else if (props.legacyId == BlockID::Vault || props.namespacedId == "opencraft:vault") {
                    m_world.setBlockEntity(c.x, c.y, c.z, std::make_shared<VaultBlockEntity>(BlockPos{c.x, c.y, c.z}));
                } else if (FluidSimulator::isFluid(props.legacyId)) {
                    m_fluidSimulator.onFluidPlaced(m_world, m_scheduledUpdates, {c.x, c.y, c.z}, props.legacyId,
                                                   m_clock.getTickCount());
                } else if (props.legacyId == BlockID::Sand) {
                    m_scheduledUpdates.schedule({c.x, c.y, c.z}, BlockID::Sand, m_clock.getTickCount() + 1, 0);
                } else if (props.legacyId == BlockID::Fire) {
                    m_scheduledUpdates.schedule({c.x, c.y, c.z}, BlockID::Fire, m_clock.getTickCount() + 30, 0);
                }

                m_cinnabarEngine.onBlockPlaced(m_world, {c.x, c.y, c.z}, props.legacyId, m_clock.getTickCount());
                m_statisticsManager.incrementUsed(props.namespacedId);

                // Consumir del inventario si está en supervivencia y el slot actual tiene este bloque
                if (execPlayer->gameMode == 0 && execPlayer->selectedSlot < execPlayer->inventory.getSize()) {
                    auto& curStack = execPlayer->inventory.getSlot(execPlayer->selectedSlot);
                    if (!curStack.isEmpty() &&
                        ItemRegistry::global().get(curStack.itemId).placeableBlockState == c.blockState) {
                        curStack.consume(1);
                        execPlayer->syncHotbar();
                    }
                }

                m_pendingEvents.push_back(BlockChangedEvent{.x = c.x,
                                                            .y = c.y,
                                                            .z = c.z,
                                                            .oldState = current,
                                                            .newState = c.blockState,
                                                            .causeEntityId = c.entityId});

                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, SelectSlotCommand>) {
                if (c.slot < 9) {
                    m_player.selectedSlot = c.slot;
                    m_player.syncHotbar();
                    return CommandResult::Success;
                }
                return CommandResult::Rejected;
            } else if constexpr (std::is_same_v<T, InventoryTransactionCommand>) {
                PlayerEntity* curPlayer = execPlayer ? execPlayer : &m_player;
                auto resolveInventory = [this, curPlayer](InventoryContainerType type) -> Inventory* {
                    if (type == InventoryContainerType::PlayerInventory) return &curPlayer->inventory;
                    if (type == InventoryContainerType::ChestContainer) {
                        if (m_isContainerOpen && m_openContainerKind == OpenContainerKind::Chest) {
                            auto chest = m_world.getChestBlockEntity(m_openContainerPos.x, m_openContainerPos.y,
                                                                     m_openContainerPos.z);
                            if (chest) return &chest->getInventory();
                        }
                        if (curPlayer) {
                            int px = static_cast<int>(std::floor(curPlayer->position.x));
                            int py = static_cast<int>(std::floor(curPlayer->position.y));
                            int pz = static_cast<int>(std::floor(curPlayer->position.z));
                            for (int dy = -4; dy <= 4; ++dy) {
                                for (int dz = -5; dz <= 5; ++dz) {
                                    for (int dx = -5; dx <= 5; ++dx) {
                                        auto chest = m_world.getChestBlockEntity(px + dx, py + dy, pz + dz);
                                        if (chest) return &chest->getInventory();
                                    }
                                }
                            }
                        }
                        return nullptr;
                    }
                    if (type == InventoryContainerType::FurnaceContainer) {
                        if (m_isContainerOpen && m_openContainerKind == OpenContainerKind::Furnace) {
                            auto furnace = m_world.getFurnaceBlockEntity(m_openContainerPos.x, m_openContainerPos.y,
                                                                         m_openContainerPos.z);
                            if (furnace) return &furnace->getInventory();
                        }
                        if (curPlayer) {
                            int px = static_cast<int>(std::floor(curPlayer->position.x));
                            int py = static_cast<int>(std::floor(curPlayer->position.y));
                            int pz = static_cast<int>(std::floor(curPlayer->position.z));
                            for (int dy = -4; dy <= 4; ++dy) {
                                for (int dz = -5; dz <= 5; ++dz) {
                                    for (int dx = -5; dx <= 5; ++dx) {
                                        auto furnace = m_world.getFurnaceBlockEntity(px + dx, py + dy, pz + dz);
                                        if (furnace) return &furnace->getInventory();
                                    }
                                }
                            }
                        }
                        return nullptr;
                    }
                    if (type == InventoryContainerType::HopperContainer) {
                        if (m_isContainerOpen && m_openContainerKind == OpenContainerKind::Hopper) {
                            auto entity = m_world.getBlockEntity(m_openContainerPos.x, m_openContainerPos.y,
                                                                 m_openContainerPos.z);
                            if (entity && entity->getType() == BlockEntityType::Hopper) {
                                return &std::static_pointer_cast<HopperBlockEntity>(entity)->getInventory();
                            }
                        }
                        return nullptr;
                    }
                    if (type == InventoryContainerType::DropperContainer) {
                        if (m_isContainerOpen && m_openContainerKind == OpenContainerKind::Dropper) {
                            auto entity = m_world.getBlockEntity(m_openContainerPos.x, m_openContainerPos.y,
                                                                 m_openContainerPos.z);
                            if (entity && entity->getType() == BlockEntityType::Dropper) {
                                return &std::static_pointer_cast<DropperBlockEntity>(entity)->getInventory();
                            }
                        }
                        return nullptr;
                    }
                    if (type == InventoryContainerType::DispenserContainer) {
                        if (m_isContainerOpen && m_openContainerKind == OpenContainerKind::Dispenser) {
                            auto entity = m_world.getBlockEntity(m_openContainerPos.x, m_openContainerPos.y,
                                                                 m_openContainerPos.z);
                            if (entity && entity->getType() == BlockEntityType::Dispenser) {
                                return &std::static_pointer_cast<DispenserBlockEntity>(entity)->getInventory();
                            }
                        }
                        return nullptr;
                    }
                    if (type == InventoryContainerType::CrafterContainer) {
                        if (m_isContainerOpen && m_openContainerKind == OpenContainerKind::Crafter) {
                            auto entity = m_world.getBlockEntity(m_openContainerPos.x, m_openContainerPos.y,
                                                                 m_openContainerPos.z);
                            if (entity && entity->getType() == BlockEntityType::Crafter) {
                                return &std::static_pointer_cast<CrafterBlockEntity>(entity)->getInventory();
                            }
                        }
                        return nullptr;
                    }
                    return nullptr;
                };

                // Transacción desde la salida de crafteo
                if (c.fromContainer == InventoryContainerType::CraftingOutput) {
                    if (m_craftingOutput.isEmpty()) return CommandResult::Success;
                    int gridW = (m_craftingGrid.size() == 9) ? 3 : 2;

                    if (c.toContainer == InventoryContainerType::Cursor) {
                        if (!curPlayer->cursorItem.isEmpty()) {
                            if (!curPlayer->cursorItem.canStackWith(m_craftingOutput) ||
                                curPlayer->cursorItem.count + m_craftingOutput.count >
                                    curPlayer->cursorItem.getMaxStack()) {
                                return CommandResult::InventoryFull;
                            }
                        }
                        ItemStack res;
                        bool ok = RecipeRegistry::global().craft(m_craftingGrid, gridW, gridW, res, false);
                        if (ok) {
                            if (curPlayer->cursorItem.isEmpty()) {
                                curPlayer->cursorItem = res;
                            } else {
                                curPlayer->cursorItem.count += res.count;
                            }
                            updateCraftingOutput();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        }
                        return CommandResult::Rejected;
                    } else if (c.toContainer == InventoryContainerType::PlayerInventory) {
                        if (curPlayer->inventory.getAvailableSpace(m_craftingOutput) < m_craftingOutput.count) {
                            return CommandResult::InventoryFull;
                        }
                        ItemStack res;
                        bool ok = RecipeRegistry::global().craft(m_craftingGrid, gridW, gridW, res, false);
                        if (ok) {
                            curPlayer->inventory.addItem(res);
                            updateCraftingOutput();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        }
                        return CommandResult::Rejected;
                    }
                    return CommandResult::Rejected;
                }

                // Transacciones con la cuadrícula de crafteo
                if (c.fromContainer == InventoryContainerType::CraftingGrid ||
                    c.toContainer == InventoryContainerType::CraftingGrid) {
                    if (c.fromContainer == InventoryContainerType::Cursor &&
                        c.toContainer == InventoryContainerType::CraftingGrid) {
                        if (c.toSlot >= m_craftingGrid.size()) return CommandResult::InvalidSlot;
                        if (curPlayer->cursorItem.isEmpty()) return CommandResult::Success;
                        auto& gridSlot = m_craftingGrid[c.toSlot];
                        if (gridSlot.isEmpty()) {
                            uint16_t toMove = (c.amount == 0 || c.amount >= curPlayer->cursorItem.count)
                                                  ? curPlayer->cursorItem.count
                                                  : c.amount;
                            gridSlot = curPlayer->cursorItem.split(toMove);
                        } else if (gridSlot.canStackWith(curPlayer->cursorItem)) {
                            uint16_t space = gridSlot.getMaxStack() - gridSlot.count;
                            if (space == 0) return CommandResult::InventoryFull;
                            uint16_t toMove = (c.amount == 0 || c.amount >= curPlayer->cursorItem.count)
                                                  ? curPlayer->cursorItem.count
                                                  : c.amount;
                            uint16_t transfer = std::min(toMove, space);
                            gridSlot.count += transfer;
                            curPlayer->cursorItem.count -= transfer;
                            if (curPlayer->cursorItem.count == 0) curPlayer->cursorItem.clear();
                        } else {
                            std::swap(gridSlot, curPlayer->cursorItem);
                        }
                        updateCraftingOutput();
                        curPlayer->syncHotbar();
                        return CommandResult::Success;
                    } else if (c.fromContainer == InventoryContainerType::CraftingGrid &&
                               c.toContainer == InventoryContainerType::Cursor) {
                        if (c.fromSlot >= m_craftingGrid.size()) return CommandResult::InvalidSlot;
                        auto& gridSlot = m_craftingGrid[c.fromSlot];
                        if (gridSlot.isEmpty()) return CommandResult::Success;
                        if (curPlayer->cursorItem.isEmpty()) {
                            uint16_t toMove = (c.amount == 0 || c.amount >= gridSlot.count) ? gridSlot.count : c.amount;
                            curPlayer->cursorItem = gridSlot.split(toMove);
                        } else if (curPlayer->cursorItem.canStackWith(gridSlot)) {
                            uint16_t space = curPlayer->cursorItem.getMaxStack() - curPlayer->cursorItem.count;
                            if (space == 0) return CommandResult::InventoryFull;
                            uint16_t toMove = (c.amount == 0 || c.amount >= gridSlot.count) ? gridSlot.count : c.amount;
                            uint16_t transfer = std::min(toMove, space);
                            curPlayer->cursorItem.count += transfer;
                            gridSlot.count -= transfer;
                            if (gridSlot.count == 0) gridSlot.clear();
                        } else {
                            std::swap(gridSlot, curPlayer->cursorItem);
                        }
                        updateCraftingOutput();
                        curPlayer->syncHotbar();
                        return CommandResult::Success;
                    } else if (c.fromContainer == InventoryContainerType::CraftingGrid &&
                               c.toContainer == InventoryContainerType::PlayerInventory) {
                        if (c.fromSlot >= m_craftingGrid.size()) return CommandResult::InvalidSlot;
                        auto& gridSlot = m_craftingGrid[c.fromSlot];
                        if (!gridSlot.isEmpty()) {
                            curPlayer->inventory.addItem(gridSlot);
                            updateCraftingOutput();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        }
                    } else if (c.fromContainer == InventoryContainerType::PlayerInventory &&
                               c.toContainer == InventoryContainerType::CraftingGrid) {
                        if (c.fromSlot >= curPlayer->inventory.getSize() || c.toSlot >= m_craftingGrid.size())
                            return CommandResult::InvalidSlot;
                        auto& fromSlot = curPlayer->inventory.getSlot(c.fromSlot);
                        if (fromSlot.isEmpty()) return CommandResult::Success;
                        auto& gridSlot = m_craftingGrid[c.toSlot];
                        uint16_t toMove = (c.amount == 0 || c.amount >= fromSlot.count) ? fromSlot.count : c.amount;
                        if (gridSlot.isEmpty()) {
                            gridSlot = fromSlot.split(toMove);
                        } else if (gridSlot.canStackWith(fromSlot)) {
                            uint16_t space = gridSlot.getMaxStack() - gridSlot.count;
                            if (space == 0) return CommandResult::InventoryFull;
                            uint16_t transfer = std::min(toMove, space);
                            gridSlot.count += transfer;
                            fromSlot.count -= transfer;
                            if (fromSlot.count == 0) fromSlot.clear();
                        } else {
                            std::swap(gridSlot, fromSlot);
                        }
                        curPlayer->inventory.incrementRevision();
                        updateCraftingOutput();
                        curPlayer->syncHotbar();
                        return CommandResult::Success;
                    }
                    return CommandResult::Rejected;
                }

                // 1. Interacción con el cursor del jugador
                if (c.fromContainer == InventoryContainerType::Cursor ||
                    c.toContainer == InventoryContainerType::Cursor) {
                    InventoryContainerType targetType =
                        (c.fromContainer == InventoryContainerType::Cursor) ? c.toContainer : c.fromContainer;
                    size_t slotIdx = (c.fromContainer == InventoryContainerType::Cursor) ? c.toSlot : c.fromSlot;
                    Inventory* inv = resolveInventory(targetType);
                    if (!inv || slotIdx >= inv->getSize()) return CommandResult::InvalidSlot;

                    if (c.expectedRevision != 0 && inv->getRevision() != c.expectedRevision) {
                        return CommandResult::RevisionConflict;
                    }

                    if (c.fromContainer == InventoryContainerType::Cursor) {
                        // Colocar desde el cursor a la ranura
                        if (curPlayer->cursorItem.isEmpty()) return CommandResult::Success;

                        auto& slot = inv->getSlot(slotIdx);
                        if (slot.isEmpty()) {
                            uint16_t toMove = (c.amount == 0 || c.amount >= curPlayer->cursorItem.count)
                                                  ? curPlayer->cursorItem.count
                                                  : c.amount;
                            slot = curPlayer->cursorItem.split(toMove);
                            inv->incrementRevision();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        } else if (slot.canStackWith(curPlayer->cursorItem)) {
                            uint16_t space = slot.getMaxStack() - slot.count;
                            if (space == 0) return CommandResult::InventoryFull;
                            uint16_t toMove = (c.amount == 0 || c.amount >= curPlayer->cursorItem.count)
                                                  ? curPlayer->cursorItem.count
                                                  : c.amount;
                            uint16_t transfer = std::min(toMove, space);
                            slot.count += transfer;
                            curPlayer->cursorItem.count -= transfer;
                            if (curPlayer->cursorItem.count == 0) curPlayer->cursorItem.clear();
                            inv->incrementRevision();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        } else {
                            // Intercambiar ranura con cursor
                            std::swap(slot, curPlayer->cursorItem);
                            inv->incrementRevision();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        }
                    } else {
                        // Tomar desde la ranura al cursor
                        auto& slot = inv->getSlot(slotIdx);
                        if (slot.isEmpty()) return CommandResult::Success;

                        if (curPlayer->cursorItem.isEmpty()) {
                            uint16_t toMove = (c.amount == 0 || c.amount >= slot.count) ? slot.count : c.amount;
                            curPlayer->cursorItem = slot.split(toMove);
                            inv->incrementRevision();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        } else if (curPlayer->cursorItem.canStackWith(slot)) {
                            uint16_t space = curPlayer->cursorItem.getMaxStack() - curPlayer->cursorItem.count;
                            if (space == 0) return CommandResult::InventoryFull;
                            uint16_t toMove = (c.amount == 0 || c.amount >= slot.count) ? slot.count : c.amount;
                            uint16_t transfer = std::min(toMove, space);
                            curPlayer->cursorItem.count += transfer;
                            slot.count -= transfer;
                            if (slot.count == 0) slot.clear();
                            inv->incrementRevision();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        } else {
                            std::swap(slot, curPlayer->cursorItem);
                            inv->incrementRevision();
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        }
                    }
                }

                // 2. Transacciones entre contenedores (o dentro del mismo contenedor)
                Inventory* fromInv = resolveInventory(c.fromContainer);
                Inventory* toInv = resolveInventory(c.toContainer);
                if (!fromInv || !toInv) return CommandResult::Rejected;

                if (c.fromSlot >= fromInv->getSize() || c.toSlot >= toInv->getSize()) {
                    return CommandResult::InvalidSlot;
                }

                // Verificación optimista de concurrencia
                if (c.expectedRevision != 0) {
                    if ((c.fromContainer == InventoryContainerType::ChestContainer ||
                         c.fromContainer == InventoryContainerType::FurnaceContainer) &&
                        fromInv->getRevision() != c.expectedRevision) {
                        return CommandResult::RevisionConflict;
                    }
                    if ((c.toContainer == InventoryContainerType::ChestContainer ||
                         c.toContainer == InventoryContainerType::FurnaceContainer) &&
                        toInv->getRevision() != c.expectedRevision) {
                        return CommandResult::RevisionConflict;
                    }
                }

                if (fromInv == toInv) {
                    if (c.action == InventoryAction::Swap) {
                        bool ok = fromInv->swapSlots(c.fromSlot, c.toSlot);
                        if (ok) {
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        }
                        return CommandResult::Rejected;
                    } else {
                        bool ok = fromInv->moveItem(c.fromSlot, c.toSlot, c.amount);
                        if (ok) {
                            curPlayer->syncHotbar();
                            return CommandResult::Success;
                        }
                        return CommandResult::Rejected;
                    }
                }

                // Entre contenedores distintos
                auto& fromStack = fromInv->getSlot(c.fromSlot);
                if (fromStack.isEmpty()) return CommandResult::Rejected;

                if (c.action == InventoryAction::QuickTransfer) {
                    uint16_t moved = toInv->addItem(fromStack);
                    if (moved > 0) {
                        fromInv->incrementRevision();
                        curPlayer->syncHotbar();
                        return CommandResult::Success;
                    }
                    return CommandResult::InventoryFull;
                } else {
                    auto& toStack = toInv->getSlot(c.toSlot);
                    uint16_t toMove = (c.amount == 0 || c.amount >= fromStack.count) ? fromStack.count : c.amount;

                    if (toStack.isEmpty()) {
                        toStack = fromStack.split(toMove);
                        fromInv->incrementRevision();
                        toInv->incrementRevision();
                        curPlayer->syncHotbar();
                        return CommandResult::Success;
                    } else if (toStack.canStackWith(fromStack)) {
                        uint16_t space = toStack.getMaxStack() - toStack.count;
                        if (space == 0) return CommandResult::InventoryFull;
                        uint16_t transfer = std::min(toMove, space);
                        toStack.count += transfer;
                        fromStack.count -= transfer;
                        if (fromStack.count == 0) fromStack.clear();
                        fromInv->incrementRevision();
                        toInv->incrementRevision();
                        curPlayer->syncHotbar();
                        return CommandResult::Success;
                    } else {
                        std::swap(fromStack, toStack);
                        fromInv->incrementRevision();
                        toInv->incrementRevision();
                        curPlayer->syncHotbar();
                        return CommandResult::Success;
                    }
                }
            } else if constexpr (std::is_same_v<T, DropItemCommand>) {
                float yawRad = Math::radians(m_player.yaw);
                Math::Vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
                forward = forward.normalized();

                Math::Vec3 dropPos = m_player.getEyePosition();
                Math::Vec3 dropVel = forward * 4.0f + Math::Vec3(0.0f, 2.0f, 0.0f);

                if (c.fromCursor) {
                    if (!m_player.cursorItem.isEmpty()) {
                        uint16_t toDrop = (c.count == 0 || c.count >= m_player.cursorItem.count)
                                              ? m_player.cursorItem.count
                                              : c.count;
                        ItemStack dropped = m_player.cursorItem.split(toDrop);
                        spawnDroppedItem(std::move(dropped), dropPos, dropVel, 20);
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    }
                    return CommandResult::Rejected;
                } else {
                    size_t slotIdx = (c.slot < m_player.inventory.getSize()) ? c.slot : m_player.selectedSlot;
                    auto& slot = m_player.inventory.getSlot(slotIdx);
                    if (!slot.isEmpty()) {
                        uint16_t toDrop = (c.count == 0 || c.count >= slot.count) ? slot.count : c.count;
                        ItemStack dropped = slot.split(toDrop);
                        spawnDroppedItem(std::move(dropped), dropPos, dropVel, 20);
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    }
                    return CommandResult::Rejected;
                }
            } else if constexpr (std::is_same_v<T, ToggleFlightCommand>) {
                if (execPlayer->gameMode != 1) {
                    return CommandResult::Rejected; // en espectador se vuela siempre
                }
                execPlayer->isFlying = !execPlayer->isFlying;
                execPlayer->fallDistance = 0.0f;
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, CreativePickCommand>) {
                // Paleta del modo creativo: el servidor solo la acepta para jugadores en creativo
                if (execPlayer->gameMode != 1) {
                    return CommandResult::Rejected;
                }
                if (c.itemId == EMPTY_ITEM_ID || c.count == 0) {
                    execPlayer->cursorItem.clear();
                    return CommandResult::Success;
                }
                const auto& def = ItemRegistry::global().get(c.itemId);
                if (def.id == EMPTY_ITEM_ID) {
                    return CommandResult::Rejected;
                }
                uint16_t count = std::min<uint16_t>(c.count, def.maxStackSize);
                if (c.toInventory) {
                    ItemStack picked(c.itemId, count);
                    execPlayer->inventory.addItem(picked);
                    execPlayer->syncHotbar();
                } else {
                    execPlayer->cursorItem = ItemStack(c.itemId, count);
                }
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, GiveItemCommand>) {
                ItemStack stack(c.itemId, c.count, c.durability);
                m_player.inventory.addItem(stack);
                m_player.syncHotbar();
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, InteractBlockCommand>) {
                if (m_player.isSpectator()) {
                    return CommandResult::Rejected;
                }

                Math::Vec3 eyePos = m_player.getEyePosition();
                Math::Vec3 blockCenter(static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.5f,
                                       static_cast<float>(c.z) + 0.5f);
                float distSq = (eyePos - blockCenter).dot(eyePos - blockCenter);
                if (distSq > (MAX_REACH_DISTANCE + 0.5f) * (MAX_REACH_DISTANCE + 0.5f)) {
                    return CommandResult::OutOfReach;
                }

                BlockStateId st = m_world.getBlockState(c.x, c.y, c.z);
                const auto& props = BlockRegistry::global().get(st);
                // 1. Interacción con cubetas (vacía, agua, lava)
                auto& heldStack = (m_player.selectedSlot < m_player.inventory.getSize())
                                      ? m_player.inventory.getSlot(m_player.selectedSlot)
                                      : m_player.cursorItem;
                ItemId heldItemId = heldStack.isEmpty() ? EMPTY_ITEM_ID : heldStack.itemId;
                const auto& heldDef = ItemRegistry::global().get(heldItemId);

                // Portales (Fase 12): mechero sobre un marco de obsidiana y Ojo del Confín sobre un marco del Confín
                if (heldDef.namespacedId == "opencraft:flint_and_steel" && props.legacyId == BlockID::Obsidian) {
                    if (!PortalSystem::detectAndActivateAbyssPortal(m_world, BlockPos{c.x, c.y + 1, c.z})) {
                        return CommandResult::Rejected;
                    }
                    if (!m_player.isCreative()) {
                        heldStack.damage(1);
                    }
                    m_player.syncHotbar();
                    return CommandResult::Success;
                }
                if (heldDef.namespacedId == "opencraft:rim_eye" && props.legacyId == BlockID::RimPortalFrame) {
                    if (!PortalSystem::insertRimEye(m_world, BlockPos{c.x, c.y, c.z})) {
                        return CommandResult::Rejected;
                    }
                    if (!m_player.isCreative()) {
                        heldStack.consume(1);
                    }
                    m_player.syncHotbar();
                    return CommandResult::Success;
                }

                if (heldDef.namespacedId == "opencraft:bucket") {
                    if (props.legacyId == BlockID::Water || props.namespacedId == "opencraft:water") {
                        m_world.setBlock(c.x, c.y, c.z, BlockID::Air);
                        heldStack.consume(1);
                        ItemId waterBucketId = ItemRegistry::global().getId("opencraft:water_bucket");
                        if (heldStack.isEmpty()) {
                            heldStack = ItemStack(waterBucketId, 1);
                        } else {
                            ItemStack waterBucket(waterBucketId, 1);
                            m_player.inventory.addItem(waterBucket);
                        }
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    } else if (props.legacyId == BlockID::Lava || props.namespacedId == "opencraft:lava") {
                        m_world.setBlock(c.x, c.y, c.z, BlockID::Air);
                        heldStack.consume(1);
                        ItemId lavaBucketId = ItemRegistry::global().getId("opencraft:lava_bucket");
                        if (heldStack.isEmpty()) {
                            heldStack = ItemStack(lavaBucketId, 1);
                        } else {
                            ItemStack lavaBucket(lavaBucketId, 1);
                            m_player.inventory.addItem(lavaBucket);
                        }
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    }
                } else if (heldDef.namespacedId == "opencraft:water_bucket") {
                    int targetX = c.x;
                    int targetY = c.y + (props.isSolid ? 1 : 0);
                    int targetZ = c.z;
                    m_world.setBlock(targetX, targetY, targetZ, BlockID::Water);
                    m_fluidSimulator.onFluidPlaced(m_world, m_scheduledUpdates, {targetX, targetY, targetZ},
                                                   BlockID::Water, m_clock.getTickCount());
                    heldStack.consume(1);
                    ItemId emptyBucketId = ItemRegistry::global().getId("opencraft:bucket");
                    if (heldStack.isEmpty()) {
                        heldStack = ItemStack(emptyBucketId, 1);
                    } else {
                        ItemStack emptyBucket(emptyBucketId, 1);
                        m_player.inventory.addItem(emptyBucket);
                    }
                    m_player.syncHotbar();
                    return CommandResult::Success;
                } else if (heldDef.namespacedId == "opencraft:lava_bucket") {
                    int targetX = c.x;
                    int targetY = c.y + (props.isSolid ? 1 : 0);
                    int targetZ = c.z;
                    m_world.setBlock(targetX, targetY, targetZ, BlockID::Lava);
                    m_fluidSimulator.onFluidPlaced(m_world, m_scheduledUpdates, {targetX, targetY, targetZ},
                                                   BlockID::Lava, m_clock.getTickCount());
                    heldStack.consume(1);
                    ItemId emptyBucketId = ItemRegistry::global().getId("opencraft:bucket");
                    if (heldStack.isEmpty()) {
                        heldStack = ItemStack(emptyBucketId, 1);
                    } else {
                        ItemStack emptyBucket(emptyBucketId, 1);
                        m_player.inventory.addItem(emptyBucket);
                    }
                    m_player.syncHotbar();
                    return CommandResult::Success;
                }

                if (props.legacyId == BlockID::Chest || props.namespacedId == "opencraft:chest") {
                    auto chest = m_world.getChestBlockEntity(c.x, c.y, c.z);
                    if (chest) {
                        returnCursorItemToInventory();
                        returnCraftingGridToInventory();
                        m_isContainerOpen = true;
                        m_openContainerKind = OpenContainerKind::Chest;
                        m_openContainerPos = BlockPos{c.x, c.y, c.z};
                        m_pendingEvents.push_back(
                            ContainerOpenedEvent{.playerEntityId = m_player.id, .x = c.x, .y = c.y, .z = c.z});
                        return CommandResult::Success;
                    }
                } else if (props.legacyId == BlockID::CraftingTable ||
                           props.namespacedId == "opencraft:crafting_table") {
                    returnCursorItemToInventory();
                    returnCraftingGridToInventory();
                    m_isContainerOpen = true;
                    m_openContainerKind = OpenContainerKind::CraftingTable;
                    m_openContainerPos = BlockPos{c.x, c.y, c.z};
                    m_craftingGrid.assign(9, ItemStack());
                    m_craftingOutput.clear();
                    m_pendingEvents.push_back(
                        ContainerOpenedEvent{.playerEntityId = m_player.id, .x = c.x, .y = c.y, .z = c.z});
                    return CommandResult::Success;
                } else if (props.legacyId == BlockID::Furnace || props.namespacedId == "opencraft:furnace") {
                    auto furnace = m_world.getFurnaceBlockEntity(c.x, c.y, c.z);
                    if (furnace) {
                        returnCursorItemToInventory();
                        returnCraftingGridToInventory();
                        m_isContainerOpen = true;
                        m_openContainerKind = OpenContainerKind::Furnace;
                        m_openContainerPos = BlockPos{c.x, c.y, c.z};
                        m_pendingEvents.push_back(
                            ContainerOpenedEvent{.playerEntityId = m_player.id, .x = c.x, .y = c.y, .z = c.z});
                        return CommandResult::Success;
                    }
                } else if (props.legacyId == BlockID::Bed || props.namespacedId == "opencraft:bed") {
                    m_player.spawnPoint = Math::Vec3(static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 1.0f,
                                                     static_cast<float>(c.z) + 0.5f);
                    m_player.hasCustomSpawnPoint = true;

                    uint64_t curTick = m_clock.getTickCount();
                    uint64_t timeOfDay = curTick % 24000;
                    bool isNight =
                        (timeOfDay >= 12000 && timeOfDay <= 23500) || (m_weather.state != WeatherState::Clear);
                    if (isNight) {
                        uint64_t day = curTick / 24000;
                        m_clock.setTickCount((day + 1) * 24000);
                        setWeather(WeatherState::Clear, 24000);
                        m_pendingEvents.push_back(BedInteractedEvent{.playerEntityId = m_player.id,
                                                                     .x = c.x,
                                                                     .y = c.y,
                                                                     .z = c.z,
                                                                     .timeSkipped = true,
                                                                     .spawnSet = true});
                    } else {
                        m_pendingEvents.push_back(BedInteractedEvent{.playerEntityId = m_player.id,
                                                                     .x = c.x,
                                                                     .y = c.y,
                                                                     .z = c.z,
                                                                     .timeSkipped = false,
                                                                     .spawnSet = true});
                    }
                    return CommandResult::Success;
                } else if (props.legacyId == BlockID::Lever || props.legacyId == BlockID::StoneButton ||
                           props.legacyId == BlockID::OakButton || props.legacyId == BlockID::Repeater ||
                           props.legacyId == BlockID::Comparator) {
                    m_cinnabarEngine.onBlockInteracted(m_world, {c.x, c.y, c.z}, props.legacyId,
                                                       m_clock.getTickCount());
                    return CommandResult::Success;
                } else if (RailSystem::isRail(props.legacyId)) {
                    if (heldDef.namespacedId == "opencraft:minecart") {
                        spawnMinecart({static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.1f,
                                       static_cast<float>(c.z) + 0.5f},
                                      MinecartType::Standard);
                        heldStack.consume(1);
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    } else if (heldDef.namespacedId == "opencraft:chest_minecart") {
                        spawnMinecart({static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.1f,
                                       static_cast<float>(c.z) + 0.5f},
                                      MinecartType::Chest);
                        heldStack.consume(1);
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    } else if (heldDef.namespacedId == "opencraft:hopper_minecart") {
                        spawnMinecart({static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.1f,
                                       static_cast<float>(c.z) + 0.5f},
                                      MinecartType::Hopper);
                        heldStack.consume(1);
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    }
                } else {
                    auto entity = m_world.getBlockEntity(c.x, c.y, c.z);
                    if (entity) {
                        if (entity->getType() == BlockEntityType::Hopper) {
                            returnCursorItemToInventory();
                            returnCraftingGridToInventory();
                            m_isContainerOpen = true;
                            m_openContainerKind = OpenContainerKind::Hopper;
                            m_openContainerPos = BlockPos{c.x, c.y, c.z};
                            return CommandResult::Success;
                        } else if (entity->getType() == BlockEntityType::Dropper) {
                            returnCursorItemToInventory();
                            returnCraftingGridToInventory();
                            m_isContainerOpen = true;
                            m_openContainerKind = OpenContainerKind::Dropper;
                            m_openContainerPos = BlockPos{c.x, c.y, c.z};
                            return CommandResult::Success;
                        } else if (entity->getType() == BlockEntityType::Dispenser) {
                            returnCursorItemToInventory();
                            returnCraftingGridToInventory();
                            m_isContainerOpen = true;
                            m_openContainerKind = OpenContainerKind::Dispenser;
                            m_openContainerPos = BlockPos{c.x, c.y, c.z};
                            return CommandResult::Success;
                        } else if (entity->getType() == BlockEntityType::Crafter) {
                            returnCursorItemToInventory();
                            returnCraftingGridToInventory();
                            m_isContainerOpen = true;
                            m_openContainerKind = OpenContainerKind::Crafter;
                            m_openContainerPos = BlockPos{c.x, c.y, c.z};
                            return CommandResult::Success;
                        } else if (entity->getType() == BlockEntityType::Vault) {
                            auto vault = std::static_pointer_cast<VaultBlockEntity>(entity);
                            auto& heldItem = (execPlayer->selectedSlot < execPlayer->inventory.getSize())
                                                 ? execPlayer->inventory.getSlot(execPlayer->selectedSlot)
                                                 : execPlayer->cursorItem;
                            if (!heldItem.isEmpty()) {
                                const auto& def = ItemRegistry::global().get(heldItem.itemId);
                                std::vector<ItemStack> loot;
                                if (vault->tryUnlock(execPlayer->id, def.namespacedId, loot)) {
                                    heldItem.consume(1);
                                    execPlayer->syncHotbar();
                                    for (auto& st : loot) {
                                        spawnDroppedItem(st, Math::Vec3(c.x + 0.5f, c.y + 1.2f, c.z + 0.5f),
                                                         Math::Vec3(0.0f, 1.5f, 0.0f), 10);
                                    }
                                    return CommandResult::Success;
                                }
                            }
                            return CommandResult::Rejected;
                        }
                    }
                }
                return CommandResult::Rejected;
            } else if constexpr (std::is_same_v<T, ToggleCrafterSlotCommand>) {
                auto entity = m_world.getBlockEntity(c.x, c.y, c.z);
                if (entity && entity->getType() == BlockEntityType::Crafter) {
                    auto crafter = std::static_pointer_cast<CrafterBlockEntity>(entity);
                    crafter->toggleSlotDisabled(c.slot);
                    return CommandResult::Success;
                }
                return CommandResult::Rejected;
            } else if constexpr (std::is_same_v<T, CloseContainerCommand>) {
                returnCursorItemToInventory();
                returnCraftingGridToInventory();
                m_isContainerOpen = false;
                m_openContainerKind = OpenContainerKind::None;
                m_pendingEvents.push_back(ContainerClosedEvent{.playerEntityId = m_player.id});
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, CraftCommand>) {
                if (m_craftingOutput.isEmpty()) {
                    return CommandResult::Rejected;
                }
                int gridW = (m_craftingGrid.size() == 9) ? 3 : 2;
                if (c.craftAll) {
                    uint32_t availableSpace = m_player.inventory.getAvailableSpace(m_craftingOutput);
                    if (availableSpace < m_craftingOutput.count) {
                        return CommandResult::InventoryFull;
                    }
                    ItemStack batchResult;
                    bool ok = RecipeRegistry::global().craft(m_craftingGrid, gridW, gridW, batchResult, true);
                    if (ok) {
                        m_player.inventory.addItem(batchResult);
                        updateCraftingOutput();
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    }
                } else {
                    if (m_player.inventory.getAvailableSpace(m_craftingOutput) < m_craftingOutput.count) {
                        return CommandResult::InventoryFull;
                    }
                    ItemStack singleResult;
                    bool ok = RecipeRegistry::global().craft(m_craftingGrid, gridW, gridW, singleResult, false);
                    if (ok) {
                        m_player.inventory.addItem(singleResult);
                        updateCraftingOutput();
                        m_player.syncHotbar();
                        return CommandResult::Success;
                    }
                }
                return CommandResult::Rejected;
            } else if constexpr (std::is_same_v<T, DigBlockCommand>) {
                if (c.action == DigAction::Start) {
                    if (c.y < Chunk::MIN_Y || c.y >= Chunk::MAX_Y) return CommandResult::InvalidPosition;
                    Math::Vec3 eyePos = m_player.getEyePosition();
                    Math::Vec3 blockCenter(static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.5f,
                                           static_cast<float>(c.z) + 0.5f);
                    float distSq = (eyePos - blockCenter).dot(eyePos - blockCenter);
                    if (distSq > (MAX_REACH_DISTANCE + 0.5f) * (MAX_REACH_DISTANCE + 0.5f)) {
                        return CommandResult::OutOfReach;
                    }
                    BlockStateId current = m_world.getBlockState(c.x, c.y, c.z);
                    if (current == AIR_BLOCK_STATE) return CommandResult::InvalidBlock;
                    const auto& props = BlockRegistry::global().get(current);
                    if (props.hardness < 0.0f) return CommandResult::Unbreakable;

                    m_miningPos = BlockPos{c.x, c.y, c.z};
                    m_miningProgress = 0.0f;
                    m_isMining = true;
                    return CommandResult::Success;
                } else if (c.action == DigAction::Cancel) {
                    m_isMining = false;
                    m_miningProgress = 0.0f;
                    return CommandResult::Success;
                } else if (c.action == DigAction::Progress) {
                    if (!m_isMining || m_miningPos != BlockPos{c.x, c.y, c.z}) {
                        m_isMining = false;
                        m_miningProgress = 0.0f;
                        return CommandResult::Rejected;
                    }
                    Math::Vec3 eyePos = m_player.getEyePosition();
                    Math::Vec3 blockCenter(static_cast<float>(c.x) + 0.5f, static_cast<float>(c.y) + 0.5f,
                                           static_cast<float>(c.z) + 0.5f);
                    float distSq = (eyePos - blockCenter).dot(eyePos - blockCenter);
                    if (distSq > (MAX_REACH_DISTANCE + 0.5f) * (MAX_REACH_DISTANCE + 0.5f)) {
                        m_isMining = false;
                        m_miningProgress = 0.0f;
                        return CommandResult::OutOfReach;
                    }
                    BlockStateId current = m_world.getBlockState(c.x, c.y, c.z);
                    const auto& props = BlockRegistry::global().get(current);

                    auto& heldItem = (m_player.selectedSlot < m_player.inventory.getSize())
                                         ? m_player.inventory.getSlot(m_player.selectedSlot)
                                         : m_player.cursorItem;
                    float speed = 1.0f;
                    bool canHarvest = (props.minHarvestTier == ToolTier::None);
                    if (!heldItem.isEmpty()) {
                        const auto& itemDef = ItemRegistry::global().get(heldItem.itemId);
                        if (itemDef.toolType == props.requiredTool) {
                            speed = itemDef.miningEfficiency;
                        }
                        if (props.minHarvestTier != ToolTier::None && itemDef.toolType == props.requiredTool &&
                            static_cast<int>(itemDef.toolTier) >= static_cast<int>(props.minHarvestTier)) {
                            canHarvest = true;
                        }
                    }
                    float damagePerTick =
                        (props.hardness <= 0.0f) ? 1.0f : (speed / (props.hardness * (canHarvest ? 30.0f : 100.0f)));
                    m_miningProgress += damagePerTick;

                    if (m_miningProgress >= 1.0f) {
                        m_isMining = false;
                        m_miningProgress = 0.0f;
                        return executeCommand(BreakBlockCommand{c.entityId, c.x, c.y, c.z});
                    }
                    return CommandResult::Success;
                } else if (c.action == DigAction::Complete) {
                    m_isMining = false;
                    m_miningProgress = 0.0f;
                    return executeCommand(BreakBlockCommand{c.entityId, c.x, c.y, c.z});
                }
                return CommandResult::Rejected;
            } else if constexpr (std::is_same_v<T, EatFoodCommand>) {
                if (!m_player.isAlive) return CommandResult::Rejected;

                // En supervivencia no se puede comer si el hambre está al máximo (20)
                if (m_player.gameMode == 0 && m_player.hunger >= 20.0f) {
                    return CommandResult::Rejected;
                }

                auto& stack = (m_player.selectedSlot < m_player.inventory.getSize())
                                  ? m_player.inventory.getSlot(m_player.selectedSlot)
                                  : m_player.cursorItem;
                if (stack.isEmpty()) return CommandResult::InvalidSlot;

                const auto& def = ItemRegistry::global().get(stack.itemId);
                if (!def.isFood) return CommandResult::Rejected;

                uint16_t foodGained = def.foodRestore;
                float satGained = def.saturationRestore;

                m_player.hunger = std::min(20.0f, m_player.hunger + static_cast<float>(foodGained));
                m_player.saturation = std::min(m_player.hunger, m_player.saturation + satGained);

                // Consumir 1 unidad del stack
                stack.count--;
                if (stack.count == 0) {
                    stack.clear();
                }
                m_player.syncHotbar();

                m_pendingEvents.push_back(PlayerAteEvent{.playerEntityId = m_player.id,
                                                         .itemId = def.id,
                                                         .foodGained = foodGained,
                                                         .saturationGained = satGained});

                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, RespawnCommand>) {
                if (m_player.isAlive) return CommandResult::Success;
                respawnPlayer();
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, SetGameModeCommand>) {
                if (m_isHardcore && m_player.isSpectator() && c.gameMode != 3) {
                    return CommandResult::Rejected;
                }
                m_player.gameMode = c.gameMode;
                if (m_player.gameMode == 1) { // Creative
                    m_player.hunger = 20.0f;
                    m_player.saturation = 5.0f;
                } else if (m_player.gameMode == 3) { // Spectator
                    m_player.isFlying = true;
                    m_player.hunger = 20.0f;
                } else {
                    m_player.isFlying = false;
                }
                m_pendingEvents.push_back(
                    GameModeChangedEvent{.playerEntityId = m_player.id, .newGameMode = m_player.gameMode});
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, SleepInBedCommand>) {
                return executeCommand(InteractBlockCommand{c.entityId, c.x, c.y, c.z});
            } else if constexpr (std::is_same_v<T, AttackEntityCommand>) {
                if (!execPlayer->isAlive || execPlayer->isSpectator()) return CommandResult::Rejected;

                MobEntity* targetMob = nullptr;
                for (auto& mob : m_mobs) {
                    if (mob.id == c.targetEntityId && mob.isAlive && !mob.isDead) {
                        targetMob = &mob;
                        break;
                    }
                }

                PlayerEntity* targetPlayer = nullptr;
                if (!targetMob) {
                    targetPlayer = getPlayerById(c.targetEntityId);
                }

                if (!targetMob && (!targetPlayer || !targetPlayer->isAlive)) return CommandResult::UnknownEntity;

                Math::Vec3 targetPos = targetMob ? targetMob->position : targetPlayer->position;
                Math::Vec3 diff = targetPos - execPlayer->position;
                if (diff.dot(diff) > (MAX_REACH_DISTANCE * MAX_REACH_DISTANCE)) {
                    return CommandResult::OutOfReach;
                }

                // Calcular daño según arma en mano
                auto& heldItem = (execPlayer->selectedSlot < execPlayer->inventory.getSize())
                                     ? execPlayer->inventory.getSlot(execPlayer->selectedSlot)
                                     : execPlayer->cursorItem;
                float damage = 1.0f;
                uint8_t breachLvl = 0;
                if (!heldItem.isEmpty()) {
                    const auto& def = ItemRegistry::global().get(heldItem.itemId);
                    if (def.attackDamage > 0.0f) {
                        damage = def.attackDamage;
                    }

                    // Mecánica de Maza 1.21: Smash Attack descendente
                    if (def.namespacedId == "opencraft:mace" &&
                        (execPlayer->velocity.y < -0.1f || execPlayer->fallDistance > 1.5f)) {
                        float fallBlocks = std::max(execPlayer->fallDistance, 1.5f);
                        execPlayer->fallDistance = 0.0f; // Anula totalmente el daño de caída del atacante

                        uint8_t densityLvl = EnchantmentSystem::getEnchantmentLevel(heldItem, EnchantmentId::Density);
                        breachLvl = EnchantmentSystem::getEnchantmentLevel(heldItem, EnchantmentId::Breach);
                        uint8_t windBurstLvl =
                            EnchantmentSystem::getEnchantmentLevel(heldItem, EnchantmentId::WindBurst);

                        float extraPerBlock = 3.0f + (static_cast<float>(densityLvl) * 0.5f);
                        damage += fallBlocks * extraPerBlock;

                        // Si posee Wind Burst, impulsa verticalmente al jugador atacante
                        if (windBurstLvl > 0) {
                            execPlayer->velocity.y = 8.0f * (0.8f + 0.3f * static_cast<float>(windBurstLvl));
                        }
                    }

                    if (def.maxDurability > 0) {
                        heldItem.damage(1);
                        execPlayer->syncHotbar();
                    }
                }

                if (targetMob) {
                    bool killed = targetMob->takeDamage(damage, &execPlayer->position, execPlayer->id);
                    m_pendingEvents.push_back(MobHurtEvent{.entityId = targetMob->id,
                                                           .attackerId = execPlayer->id,
                                                           .damage = damage,
                                                           .newHealth = targetMob->health});

                    if (killed) {
                        targetMob->isLootDropped = true;
                        m_pendingEvents.push_back(MobDiedEvent{.entityId = targetMob->id,
                                                               .killerId = execPlayer->id,
                                                               .deathPosition = targetMob->position});
                        auto loot = targetMob->generateLoot();
                        for (auto& st : loot) {
                            spawnDroppedItem(st, targetMob->position, Math::Vec3(0.0f, 1.0f, 0.0f), 10);
                        }
                        spawnExperienceOrb(targetMob->position, targetMob->getExperienceDrop());

                        // Notificar a Trial Spawners cercanos
                        int mbx = static_cast<int>(std::floor(targetMob->position.x));
                        int mby = static_cast<int>(std::floor(targetMob->position.y));
                        int mbz = static_cast<int>(std::floor(targetMob->position.z));
                        for (int dx = -14; dx <= 14; ++dx) {
                            for (int dz = -14; dz <= 14; ++dz) {
                                for (int dy = -6; dy <= 6; ++dy) {
                                    auto be = m_world.getBlockEntity(mbx + dx, mby + dy, mbz + dz);
                                    if (be && be->getType() == BlockEntityType::TrialSpawner) {
                                        auto ts = std::static_pointer_cast<TrialSpawnerBlockEntity>(be);
                                        ts->onMobDefeated(targetMob->id);
                                    }
                                }
                            }
                        }
                    }
                } else if (targetPlayer) {
                    float effectiveDamage = damage;
                    float armorPoints = 0.0f;
                    for (const auto& arm : targetPlayer->armorSlots) {
                        if (!arm.isEmpty()) {
                            const auto& def = ItemRegistry::global().get(arm.itemId);
                            armorPoints += def.armorPoints;
                        }
                    }
                    if (breachLvl > 0) {
                        armorPoints = std::max(0.0f, armorPoints * (1.0f - 0.15f * static_cast<float>(breachLvl)));
                    }
                    effectiveDamage = std::max(0.5f, damage * (1.0f - armorPoints * 0.04f));
                    if (targetPlayer->isBlocking) {
                        effectiveDamage = 0.0f;
                    }
                    targetPlayer->health -= effectiveDamage;
                    targetPlayer->hurtTime = 10;
                    if (targetPlayer->health <= 0.0f) {
                        targetPlayer->health = 0.0f;
                        targetPlayer->isAlive = false;
                    }
                }

                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, UseShieldCommand>) {
                m_player.isBlocking = c.isBlocking;
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, ShootBowCommand>) {
                if (!m_player.isAlive) return CommandResult::Rejected;

                // Verificar si el jugador tiene flechas si está en supervivencia
                auto& itemReg = ItemRegistry::global();
                ItemId arrowId = itemReg.getId("opencraft:arrow");

                if (m_player.gameMode == 0) {
                    if (!m_player.inventory.hasItem(arrowId, 1)) {
                        return CommandResult::Rejected; // Sin munición
                    }
                    for (size_t s = 0; s < m_player.inventory.getSize(); ++s) {
                        if (m_player.inventory.getSlot(s).itemId == arrowId) {
                            m_player.inventory.removeItem(s, 1);
                            break;
                        }
                    }
                    m_player.syncHotbar();
                }

                float yawRad = Math::radians(m_player.yaw);
                float pitchRad = Math::radians(m_player.pitch);
                Math::Vec3 dir(std::cos(yawRad) * std::cos(pitchRad), std::sin(pitchRad),
                               std::sin(yawRad) * std::cos(pitchRad));
                if (dir.length() > 0.01f) {
                    dir = dir.normalized();
                }

                float power = std::clamp(c.charge, 0.1f, 1.0f);
                spawnProjectile(m_player.id, m_player.getEyePosition(), dir * (power * 24.0f), power * 6.0f);
                return CommandResult::Success;
            } else if constexpr (std::is_same_v<T, TradeWithVillagerCommand>) {
                for (auto& mob : m_mobs) {
                    if (mob.id == c.villagerEntityId && mob.mobType == MobType::Villager && mob.isAlive) {
                        TradeResult res = mob.villagerData.executeTrade(*execPlayer, c.offerIndex);
                        if (res == TradeResult::Success) {
                            return CommandResult::Success;
                        } else if (res == TradeResult::OutOfStock) {
                            return CommandResult::TradeOutOfStock;
                        } else if (res == TradeResult::InsufficientItems) {
                            return CommandResult::InsufficientPayment;
                        }
                        return CommandResult::Rejected;
                    }
                }
                return CommandResult::UnknownEntity;
            } else if constexpr (std::is_same_v<T, InteractMobCommand>) {
                for (auto& mob : m_mobs) {
                    if (mob.id == c.mobEntityId && mob.isAlive) {
                        bool ok = AnimalBreeding::feedAnimal(mob, c.heldItem, execPlayer->id, m_clock.getTickCount());
                        if (ok) {
                            return CommandResult::Success;
                        }
                        return CommandResult::Rejected;
                    }
                }
                return CommandResult::UnknownEntity;
            } else if constexpr (std::is_same_v<T, FishCommand>) {
                BlockPos wPos{c.targetX, c.targetY, c.targetZ};
                auto bite = Agriculture::evaluateFishing(m_world, wPos, m_clock.getTickCount());
                if (bite.validWater) {
                    if (!bite.caughtItem.isEmpty()) {
                        execPlayer->inventory.addItem(bite.caughtItem);
                    }
                    if (bite.xp > 0) {
                        execPlayer->addExperience(bite.xp);
                    }
                    return CommandResult::Success;
                }
                return CommandResult::InvalidPosition;
            }
            return CommandResult::Rejected;
        },
        cmd);
}

void WorldSession::resolveCollisions(Math::Vec3& vel, float dt) {
    if (m_player.isSpectator()) {
        m_player.position += vel * dt;
        m_player.isGrounded = false;
        return;
    }

    const float halfW = m_player.width * 0.5f;
    const float height = m_player.isSneaking ? 1.5f : m_player.height;
    // Margen para que tocar una cara (p. ej. apoyarse en una pared) no cuente como solaparse con el
    // bloque contiguo: sin él, la pared se tomaba como suelo/techo y el salto hundía al jugador
    constexpr float EPS = 1e-4f;

    const auto& reg = BlockRegistry::global();
    auto isSolidAt = [&](int x, int y, int z) -> bool {
        if (y < Chunk::MIN_Y || y >= Chunk::MAX_Y) return false;
        if (!m_world.isChunkLoaded(floorDiv16(x), floorDiv16(z))) {
            return true; // terreno aún no cargado: frontera infranqueable hasta que llegue
        }
        return reg.get(m_world.getBlockState(x, y, z)).isSolid;
    };

    struct Box {
        float minX, minY, minZ, maxX, maxY, maxZ;
    };
    auto boxAt = [&](const Math::Vec3& p) {
        return Box{p.x - halfW, p.y, p.z - halfW, p.x + halfW, p.y + height, p.z + halfW};
    };
    // Recorre los bloques que el volumen ocupa realmente (caras compartidas excluidas)
    auto forEachOverlap = [&](const Box& b, auto&& fn) {
        int x0 = static_cast<int>(std::floor(b.minX + EPS)), x1 = static_cast<int>(std::floor(b.maxX - EPS));
        int y0 = static_cast<int>(std::floor(b.minY + EPS)), y1 = static_cast<int>(std::floor(b.maxY - EPS));
        int z0 = static_cast<int>(std::floor(b.minZ + EPS)), z1 = static_cast<int>(std::floor(b.maxZ - EPS));
        for (int y = y0; y <= y1; ++y)
            for (int z = z0; z <= z1; ++z)
                for (int x = x0; x <= x1; ++x)
                    if (isSolidAt(x, y, z)) fn(x, y, z);
    };

    // Bloques que ya se solapaban antes de mover: no bloquean (permite salir de un bloque en el
    // que se quedó atrapado sin que la resolución lo teletransporte a través del terreno)
    auto overlapsBefore = [&](const Math::Vec3& before, int x, int y, int z) {
        Box b0 = boxAt(before);
        return static_cast<float>(x + 1) > b0.minX + EPS &&
               static_cast<float>(x)<b0.maxX - EPS&& static_cast<float>(y + 1)> b0.minY + EPS &&
               static_cast<float>(y)<b0.maxY - EPS&& static_cast<float>(z + 1)> b0.minZ + EPS &&
               static_cast<float>(z) < b0.maxZ - EPS;
    };

    // Subpasos para no atravesar bloques con velocidades altas (caídas largas, vuelo)
    const float maxSubDt = 0.025f;
    int numSteps = std::max(1, static_cast<int>(std::ceil(dt / maxSubDt)));
    float subDt = dt / static_cast<float>(numSteps);

    bool grounded = false;
    for (int step = 0; step < numSteps; ++step) {
        // Protección de bordes al agacharse: no avanzar hacia un vacío
        if (m_player.isSneaking && m_player.isGrounded) {
            auto hasFloor = [&](float px, float pz) {
                Box b = boxAt(Math::Vec3(px, m_player.position.y - 0.6f, pz));
                b.maxY = m_player.position.y - 0.01f;
                bool any = false;
                forEachOverlap(b, [&](int, int, int) { any = true; });
                return any;
            };
            if (!hasFloor(m_player.position.x + vel.x * subDt, m_player.position.z)) vel.x = 0.0f;
            if (!hasFloor(m_player.position.x, m_player.position.z + vel.z * subDt)) vel.z = 0.0f;
        }

        // Eje Y
        if (vel.y != 0.0f) {
            Math::Vec3 before = m_player.position;
            m_player.position.y += vel.y * subDt;
            Box b = boxAt(m_player.position);
            bool hit = false;
            float limit = vel.y < 0.0f ? -1e30f : 1e30f;
            forEachOverlap(b, [&](int x, int y, int z) {
                if (overlapsBefore(before, x, y, z)) return;
                hit = true;
                if (vel.y < 0.0f)
                    limit = std::max(limit, static_cast<float>(y + 1));
                else
                    limit = std::min(limit, static_cast<float>(y));
            });
            if (hit) {
                if (vel.y < 0.0f) {
                    m_player.position.y = limit;
                    grounded = true;
                } else {
                    m_player.position.y = limit - height;
                }
                vel.y = 0.0f;
            }
        }

        // Ejes horizontales
        auto moveAxis = [&](int axis, float& v) {
            if (v == 0.0f) return;
            Math::Vec3 before = m_player.position;
            float& coord = (axis == 0) ? m_player.position.x : m_player.position.z;
            coord += v * subDt;
            Box b = boxAt(m_player.position);
            bool hit = false;
            float limit = v < 0.0f ? -1e30f : 1e30f;
            forEachOverlap(b, [&](int x, int y, int z) {
                if (overlapsBefore(before, x, y, z)) return;
                int c = (axis == 0) ? x : z;
                hit = true;
                if (v < 0.0f)
                    limit = std::max(limit, static_cast<float>(c + 1));
                else
                    limit = std::min(limit, static_cast<float>(c));
            });
            if (hit) {
                coord = (v < 0.0f) ? limit + halfW : limit - halfW;
                v = 0.0f;
            }
        };
        moveAxis(0, vel.x);
        moveAxis(2, vel.z);
    }

    // Sin velocidad vertical (p. ej. de pie) se comprueba el apoyo directamente
    if (!grounded) {
        Box below = boxAt(m_player.position);
        below.minY -= 0.02f;
        below.maxY = m_player.position.y;
        forEachOverlap(below, [&](int, int, int) { grounded = true; });
        if (vel.y > 0.0f) grounded = false;
    }
    m_player.isGrounded = grounded;
}

void WorldSession::tick() {
    m_functionRunner.onTickStart();

    // 1. Guardar estado previo para interpolación
    m_player.prevPosition = m_player.position;
    m_player.prevYaw = m_player.yaw;
    m_player.prevPitch = m_player.pitch;

    // 2. Procesar cola de comandos
    for (const auto& cmd : m_commandQueue) {
        executeCommand(cmd);
    }
    m_commandQueue.clear();

    float tickDt = static_cast<float>(SimulationClock::TICK_DURATION);

    if (!m_player.isAlive) {
        m_player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
        m_player.deathTime++;
    } else {
        // Normalización de entrada diagonal
        Math::Vec3 moveInput = m_pendingMoveInput;
        float inputLenSq = moveInput.x * moveInput.x + moveInput.z * moveInput.z;
        if (inputLenSq > 1.0f) {
            float invLen = 1.0f / std::sqrt(inputLenSq);
            moveInput.x *= invLen;
            moveInput.z *= invLen;
        }

        float yawRad = Math::radians(m_player.yaw);
        Math::Vec3 forward{std::cos(yawRad), 0.0f, std::sin(yawRad)};
        forward = forward.normalized();
        Math::Vec3 right{-std::sin(yawRad), 0.0f, std::cos(yawRad)};
        right = right.normalized();

        m_player.isInWater = isFeetInWater();
        m_player.isInLava = isFeetInLava();

        float moveSpeed = 4.3f;
        if ((m_player.gameMode == 1 || m_player.gameMode == 3) && m_player.isFlying) {
            moveSpeed = 10.0f;
        } else if (m_player.isInLava) {
            moveSpeed = 2.0f; // Muy lento en lava
        } else if (m_player.isInWater) {
            moveSpeed = 3.2f; // Resistencia de agua
        } else if (m_player.isSprinting && m_player.hunger > 6.0f) {
            moveSpeed = 5.6f;
            if (inputLenSq > 0.01f) {
                m_player.exhaustion += 0.028f;
            }
        } else if (m_player.isSneaking) {
            moveSpeed = 1.3f;
        }

        Math::Vec3 desiredVel = (forward * moveInput.z + right * moveInput.x) * moveSpeed;
        m_player.velocity.x = desiredVel.x;
        m_player.velocity.z = desiredVel.z;

        if ((m_player.isFlying || m_player.isSpectator()) && m_player.isSneaking) {
            m_player.velocity.y = -8.0f;
        }

        if (m_pendingJump) {
            if (m_player.isFlying || m_player.isSpectator()) {
                m_player.velocity.y = 8.0f;
            } else if (m_player.isInWater) {
                m_player.velocity.y = 3.5f; // Nado hacia arriba
            } else if (m_player.isInLava) {
                m_player.velocity.y = 2.0f; // Salto en lava
            } else if (m_player.isGrounded) {
                m_player.velocity.y = 8.5f;
                m_player.isGrounded = false;
                m_player.exhaustion += (m_player.isSprinting ? 0.2f : 0.05f);
            }
        }
        m_pendingJump = false;

        // Gravedad y resistencia
        if (m_player.isSpectator() || (m_player.gameMode == 1 && m_player.isFlying)) {
            m_player.velocity.y *= 0.8f;
            m_player.fallDistance = 0.0f;
        } else if (m_player.isInWater) {
            m_player.velocity.y -= 7.0f * tickDt;
            m_player.velocity.y *= 0.85f;
            m_player.fallDistance = 0.0f;
        } else if (m_player.isInLava) {
            m_player.velocity.y -= 5.0f * tickDt;
            m_player.velocity.y *= 0.6f;
            m_player.fallDistance = 0.0f;
        } else {
            m_player.velocity.y -= 26.0f * tickDt;
            if (!m_player.isGrounded && m_player.velocity.y < 0.0f) {
                m_player.fallDistance += -m_player.velocity.y * tickDt;
            }
        }

        // Resolver colisiones con substepping
        resolveCollisions(m_player.velocity, tickDt);

        // Al tocar tierra, comprobar daño por caída
        if (m_player.isGrounded) {
            if (m_player.gameMode == 0 && !m_player.isFlying && m_player.fallDistance > 3.0f) {
                float fallDmg = std::floor(m_player.fallDistance - 3.0f);
                if (fallDmg > 0.0f) {
                    applyDamage(fallDmg, DamageCause::Fall);
                }
            }
            m_player.fallDistance = 0.0f;
        }

        // 4. Daños ambientales y supervivencia
        if (m_player.hurtTime > 0) {
            m_player.hurtTime--;
        }

        if (!m_player.isSpectator()) {
            // Daño por vacío
            if (m_player.position.y < -96.0f) {
                applyDamage(4.0f, DamageCause::Void);
            }

            // Asfixia dentro de bloque sólido
            if (isHeadInSolid()) {
                if (m_clock.getTickCount() > 0 && m_clock.getTickCount() % 20 == 0) {
                    applyDamage(1.0f, DamageCause::Suffocation);
                }
            }

            // Aire / Ahogamiento
            if (isHeadInWater()) {
                if (m_player.air > 0) {
                    m_player.air--;
                } else {
                    m_player.air = 0;
                    if (m_clock.getTickCount() > 0 && m_clock.getTickCount() % 20 == 0) {
                        applyDamage(2.0f, DamageCause::Drowning);
                    }
                }
            } else {
                m_player.air = std::min(m_player.maxAir, m_player.air + 5);
            }

            // Daño por Lava y Fuego
            if (isFeetInLava() || isHeadInLava()) {
                m_player.fireTicks = 300;
                if (m_clock.getTickCount() > 0 && m_clock.getTickCount() % 20 == 0) {
                    applyDamage(4.0f, DamageCause::Lava);
                }
            }

            int px = static_cast<int>(std::floor(m_player.position.x));
            int py = static_cast<int>(std::floor(m_player.position.y));
            int pz = static_cast<int>(std::floor(m_player.position.z));
            BlockID pBlock = m_world.getBlock(px, py, pz);
            if (pBlock == BlockID::Fire) {
                m_player.fireTicks = 160;
                if (m_clock.getTickCount() > 0 && m_clock.getTickCount() % 20 == 0) {
                    applyDamage(1.0f, DamageCause::Fire);
                }
            }

            if (m_player.fireTicks > 0) {
                m_player.fireTicks--;
                if (m_player.isInWater ||
                    (m_weather.state != WeatherState::Clear && m_world.getSkyLight(px, py, pz) == 15)) {
                    m_player.fireTicks = 0;
                } else if (m_clock.getTickCount() > 0 && m_clock.getTickCount() % 20 == 0) {
                    applyDamage(1.0f, DamageCause::Fire);
                }
            }

            // Inanición (Hambre = 0)
            if (m_player.hunger <= 0.0f && m_player.gameMode == 0 && m_difficulty > 0) {
                if (m_clock.getTickCount() > 0 && m_clock.getTickCount() % 80 == 0) {
                    if (m_difficulty == 1 && m_player.health > 10.0f) {
                        applyDamage(1.0f, DamageCause::Starvation);
                    } else if (m_difficulty == 2 && m_player.health > 1.0f) {
                        applyDamage(1.0f, DamageCause::Starvation);
                    } else if (m_difficulty == 3) {
                        applyDamage(1.0f, DamageCause::Starvation);
                    }
                }
            }

            // Metabolismo: conversión de agotamiento (exhaustion) a hambre/saturación
            while (m_player.exhaustion >= 4.0f) {
                m_player.exhaustion -= 4.0f;
                if (m_player.saturation > 0.0f) {
                    m_player.saturation = std::max(0.0f, m_player.saturation - 1.0f);
                } else if (m_player.hunger > 0.0f) {
                    m_player.hunger = std::max(0.0f, m_player.hunger - 1.0f);
                }
            }

            // Regeneración de salud
            if (m_player.isAlive && m_player.health > 0.0f && m_player.health < m_player.maxHealth &&
                m_player.gameMode == 0) {
                if (m_player.saturation > 0.0f && m_player.hunger >= 20.0f) {
                    // Regeneración rápida
                    if (m_clock.getTickCount() > 0 && m_clock.getTickCount() % 10 == 0) {
                        m_player.health = std::min(m_player.maxHealth, m_player.health + 1.0f);
                        m_player.exhaustion += 1.5f;
                    }
                } else if (m_player.hunger >= 18.0f) {
                    // Regeneración estándar
                    if (m_clock.getTickCount() > 0 && m_clock.getTickCount() % 80 == 0) {
                        m_player.health = std::min(m_player.maxHealth, m_player.health + 1.0f);
                        m_player.exhaustion += 1.5f;
                    }
                }
            }
        }
    }

    // 4b. Portales: permanencia en portal del Abismo o entrada a portal del Confín
    tickPortals();

    // 5. Actualizar físicas y recogida de objetos tirados (ItemEntity)
    for (auto& item : m_itemEntities) {
        item.tick(m_world, tickDt);

        if (m_player.isAlive && item.canBePickedUp()) {
            Math::Vec3 itemCenter = item.position + Math::Vec3(0.0f, item.height * 0.5f, 0.0f);
            Math::Vec3 playerCenter = m_player.position + Math::Vec3(0.0f, 0.9f, 0.0f);
            Math::Vec3 diff = itemCenter - playerCenter;
            float distSq = diff.dot(diff);

            // Radio de recogida ampliado para tolerar centros de bloque y desalineación (2.5m, 2.5² = 6.25)
            if (distSq <= 6.25f) {
                uint16_t added = m_player.inventory.addItem(item.stack);
                if (added > 0) {
                    m_pendingEvents.push_back(ItemPickedUpEvent{.playerEntityId = m_player.id,
                                                                .itemEntityId = item.id,
                                                                .itemId = item.stack.itemId,
                                                                .count = added});
                    if (item.stack.isEmpty()) {
                        item.isDead = true;
                    }
                    m_player.syncHotbar();
                }
            }
        }
    }

    // Limpiar entidades muertas o expiradas
    std::erase_if(m_itemEntities, [](const ItemEntity& it) { return it.isDead; });

    // 6. Ticking de bloques-entidad (Hornos, Bloques de Comandos, Generadores de desafío) a 20 Hz fija
    std::vector<std::shared_ptr<TrialSpawnerBlockEntity>> trialSpawners;
    for (const auto& [coord, chunk] : m_world.getChunks()) {
        if (chunk) {
            for (auto& [pos, be] : chunk->getBlockEntities()) {
                if (be && be->getType() == BlockEntityType::TrialSpawner) {
                    trialSpawners.push_back(std::static_pointer_cast<TrialSpawnerBlockEntity>(be));
                } else if (be && be->getType() == BlockEntityType::Furnace) {
                    std::static_pointer_cast<FurnaceBlockEntity>(be)->tick(RecipeRegistry::global());
                } else if (be && be->getType() == BlockEntityType::CommandBlock) {
                    auto cb = std::static_pointer_cast<CommandBlockEntity>(be);
                    bool powered =
                        cb->isAlwaysActive() || (m_cinnabarEngine.getDirectOrIndirectPower(m_world, pos) > 0);
                    if (cb->getMode() == CommandBlockMode::Repeating) {
                        if (powered) {
                            cb->execute(*this);
                        }
                    } else if (cb->getMode() == CommandBlockMode::Impulse) {
                        if (powered && !cb->wasPowered()) {
                            cb->execute(*this);
                        }
                    }
                    cb->setPowered(powered);
                }
            }
        }
    }

    // Los generadores se procesan fuera del recorrido: generan criaturas y objetos
    for (auto& ts : trialSpawners) {
        tickTrialSpawner(*ts);
    }

    // 7. Verificar alcance de minado en progreso
    if (m_isMining) {
        Math::Vec3 eyePos = m_player.getEyePosition();
        Math::Vec3 blockCenter(static_cast<float>(m_miningPos.x) + 0.5f, static_cast<float>(m_miningPos.y) + 0.5f,
                               static_cast<float>(m_miningPos.z) + 0.5f);
        float distSq = (eyePos - blockCenter).dot(eyePos - blockCenter);
        if (distSq > (MAX_REACH_DISTANCE + 0.5f) * (MAX_REACH_DISTANCE + 0.5f)) {
            m_isMining = false;
            m_miningProgress = 0.0f;
        }
    }

    // 8. Actualizar clima (Clear -> Rain -> Clear)
    if (m_weather.ticksRemaining > 0) {
        m_weather.ticksRemaining--;
    } else {
        if (m_weather.state == WeatherState::Clear) {
            setWeather(WeatherState::Rain, 12000);
        } else {
            setWeather(WeatherState::Clear, 24000);
        }
    }

    // 9. Procesar cola de actualizaciones de bloques programadas (Fluidos, Arena, Fuego) bajo presupuesto acotado
    std::vector<ScheduledBlockUpdate> readyUpdates;
    m_scheduledUpdates.pollReady(m_clock.getTickCount(), ScheduledUpdateQueue::DEFAULT_TICK_BUDGET, readyUpdates);
    for (const auto& upd : readyUpdates) {
        int cx = floorDiv16(upd.pos.x);
        int cz = floorDiv16(upd.pos.z);
        if (!m_world.isChunkLoaded(cx, cz)) {
            // Si el chunk está dormido/descargado, conservar en cola para cuando despierte
            m_scheduledUpdates.schedule(upd.pos, upd.blockId, upd.targetTick + 20, upd.priority);
            continue;
        }

        BlockID cur = m_world.getBlock(upd.pos.x, upd.pos.y, upd.pos.z);
        if (FluidSimulator::isFluid(cur)) {
            m_fluidSimulator.processUpdate(m_world, m_scheduledUpdates, upd, m_clock.getTickCount());
        } else if (cur == BlockID::Sand) {
            // Gravedad de arena
            BlockPos belowPos{upd.pos.x, upd.pos.y - 1, upd.pos.z};
            if (belowPos.y >= Chunk::MIN_Y) {
                BlockID belowBlock = m_world.getBlock(belowPos.x, belowPos.y, belowPos.z);
                const auto& bInfo = BlockDatabase::get(belowBlock);
                if (!bInfo.isSolid) {
                    m_world.setBlock(upd.pos.x, upd.pos.y, upd.pos.z, BlockID::Air);
                    m_world.setBlock(belowPos.x, belowPos.y, belowPos.z, BlockID::Sand);
                    m_scheduledUpdates.schedule(belowPos, BlockID::Sand, m_clock.getTickCount() + 2, 0);
                }
            }
        } else if (cur == BlockID::Fire) {
            // Fuego: extinguir si llueve a cielo abierto
            if (m_weather.state != WeatherState::Clear && m_world.getSkyLight(upd.pos.x, upd.pos.y, upd.pos.z) == 15) {
                m_world.setBlock(upd.pos.x, upd.pos.y, upd.pos.z, BlockID::Air);
            } else {
                bool touchingWater = false;
                static constexpr std::array<BlockPos, 6> fOffsets = {
                    {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}}};
                for (const auto& fo : fOffsets) {
                    if (FluidSimulator::isWater(
                            m_world.getBlock(upd.pos.x + fo.x, upd.pos.y + fo.y, upd.pos.z + fo.z))) {
                        touchingWater = true;
                        break;
                    }
                }
                if (touchingWater) {
                    m_world.setBlock(upd.pos.x, upd.pos.y, upd.pos.z, BlockID::Air);
                } else {
                    // Propagación a bloques inflamables vecinos
                    for (const auto& fo : fOffsets) {
                        BlockPos npos{upd.pos.x + fo.x, upd.pos.y + fo.y, upd.pos.z + fo.z};
                        BlockID nb = m_world.getBlock(npos.x, npos.y, npos.z);
                        if (nb == BlockID::Wood || nb == BlockID::Planks || nb == BlockID::Leaves) {
                            m_world.setBlock(npos.x, npos.y, npos.z, BlockID::Fire);
                            m_scheduledUpdates.schedule(npos, BlockID::Fire, m_clock.getTickCount() + 30, 0);
                        }
                    }
                }
            }
        }
    }

    // 9.1 Simulación de Cinabrio, circuitos, lógica y actuadores
    m_cinnabarEngine.tick(m_world, m_scheduledUpdates, m_itemEntities, m_projectiles, m_clock.getTickCount(),
                          m_clock.getSunlightFactor());

    // 10. Actualizar Criaturas (MobEntity)
    float sunFactor = m_clock.getSunlightFactor();
    for (auto& mob : m_mobs) {
        if (!mob.isAlive || mob.isDead) continue;

        mob.tickAI(m_world, m_player, m_projectiles, m_nextEntityId, sunFactor);

        if (mob.hasMeleeHit) {
            applyDamage(mob.pendingMeleeDamage, DamageCause::Generic, &mob.position);
            Math::Vec3 kb = (m_player.position - mob.position);
            kb.y = 0.0f;
            if (kb.length() > 0.001f) {
                kb = kb.normalized();
                m_player.velocity.x += kb.x * 5.0f;
                m_player.velocity.z += kb.z * 5.0f;
                m_player.velocity.y = 3.5f;
                m_player.isGrounded = false;
            }
        }

        if (mob.hasExploded) {
            explode(mob.position, mob.explosionRadius, true);
        }

        mob.tickPhysics(m_world, tickDt);

        if (mob.isDead && !mob.isLootDropped) {
            mob.isLootDropped = true;
            m_pendingEvents.push_back(
                MobDiedEvent{.entityId = mob.id, .killerId = INVALID_ENTITY_ID, .deathPosition = mob.position});
            auto loot = mob.generateLoot();
            for (auto& st : loot) {
                spawnDroppedItem(st, mob.position, Math::Vec3(0.0f, 1.0f, 0.0f), 10);
            }
        }
    }
    std::erase_if(m_mobs, [](const MobEntity& m) { return m.isDead; });

    // Comprobar cría entre animales en celo cercanos (Fase 10)
    for (size_t i = 0; i < m_mobs.size(); ++i) {
        if (!m_mobs[i].inLove || m_mobs[i].isBaby) continue;
        for (size_t j = i + 1; j < m_mobs.size(); ++j) {
            if (!m_mobs[j].inLove || m_mobs[j].isBaby) continue;
            if (m_mobs[i].mobType == m_mobs[j].mobType) {
                if (AnimalBreeding::tryBreeding(m_mobs[i], m_mobs[j], *this, m_clock.getTickCount())) {
                    break;
                }
            }
        }
    }

    // Reposición periódica de aldeanos (dos veces al día)
    if ((m_clock.getTickCount() % 12000) == 0) {
        for (auto& mob : m_mobs) {
            if (mob.mobType == MobType::Villager && mob.isAlive) {
                mob.villagerData.restock();
            }
        }
    }

    // Actualizar física de barcos (Fase 10)
    for (auto& boat : m_boats) {
        boat.tickPhysics(m_world, tickDt);
    }
    std::erase_if(m_boats, [](const BoatEntity& b) { return !b.isAlive; });

    // Actualizar física de vagonetas (Fase 11)
    for (auto& cart : m_minecarts) {
        cart.tickPhysics(m_world, &m_cinnabarEngine, &m_itemEntities, tickDt);
    }
    std::erase_if(m_minecarts, [](const MinecartEntity& c) { return !c.isAlive; });

    // 11. Actualizar Proyectiles (Flechas)
    for (auto& proj : m_projectiles) {
        Math::Vec3 oldPos = proj.position;
        Math::Vec3 expectedNext = oldPos + proj.velocity * tickDt;
        proj.tick(m_world, tickDt);

        if (!proj.isDead) {
            // Comprobación continua (swept AABB) cubriendo toda la trayectoria del tick
            float maxX = std::max({oldPos.x, proj.position.x, expectedNext.x});
            float minX = std::min({oldPos.x, proj.position.x, expectedNext.x});
            float maxY = std::max({oldPos.y, proj.position.y, expectedNext.y});
            float minY = std::min({oldPos.y, proj.position.y, expectedNext.y});
            float maxZ = std::max({oldPos.z, proj.position.z, expectedNext.z});
            float minZ = std::min({oldPos.z, proj.position.z, expectedNext.z});

            Math::AABB sweptBox{.min = {minX - proj.width * 0.5f, minY - proj.height * 0.5f, minZ - proj.width * 0.5f},
                                .max = {maxX + proj.width * 0.5f, maxY + proj.height * 0.5f, maxZ + proj.width * 0.5f}};

            // Colisión contra jugador
            if (proj.shooterId != m_player.id && m_player.isAlive && m_player.getAABB().intersects(sweptBox)) {
                applyDamage(proj.damage, DamageCause::Generic, &proj.position);
                proj.isDead = true;
            }

            // Colisión contra criaturas
            if (!proj.isDead) {
                for (auto& mob : m_mobs) {
                    if (mob.id != proj.shooterId && mob.isAlive && !mob.isDead && mob.getAABB().intersects(sweptBox)) {
                        bool killed = mob.takeDamage(proj.damage, &proj.position, proj.shooterId);
                        proj.isDead = true;
                        m_pendingEvents.push_back(MobHurtEvent{.entityId = mob.id,
                                                               .attackerId = proj.shooterId,
                                                               .damage = proj.damage,
                                                               .newHealth = mob.health});
                        if (killed) {
                            m_pendingEvents.push_back(MobDiedEvent{
                                .entityId = mob.id, .killerId = proj.shooterId, .deathPosition = mob.position});
                            auto loot = mob.generateLoot();
                            for (auto& st : loot) {
                                spawnDroppedItem(st, mob.position, Math::Vec3(0.0f, 1.0f, 0.0f), 10);
                            }
                            spawnExperienceOrb(mob.position, mob.getExperienceDrop());
                        }
                        break;
                    }
                }
            }
        }
    }
    std::erase_if(m_projectiles, [](const ProjectileEntity& p) { return p.isDead; });

    // 12. Actualizar Orbes de Experiencia
    for (auto& orb : m_experienceOrbs) {
        orb.tick(m_world, m_player.position, tickDt);

        if (m_player.isAlive && !orb.isDead) {
            Math::Vec3 diff = (m_player.position + Math::Vec3(0.0f, 0.9f, 0.0f)) - orb.position;
            if (diff.dot(diff) <= 2.25f) { // <= 1.5 metros
                m_player.addExperience(orb.value);
                orb.isDead = true;
                m_pendingEvents.push_back(ExperienceGainedEvent{
                    .playerEntityId = m_player.id, .amount = orb.value, .newLevel = m_player.xpLevel});
            }
        }
    }
    std::erase_if(m_experienceOrbs, [](const ExperienceOrbEntity& o) { return o.isDead; });

    // 13. Generación y Desaparición Periódica de Criaturas (MobSpawner)
    m_mobSpawner.tick(m_world, m_player, m_mobs, m_nextEntityId, m_difficulty, sunFactor,
                      m_commandEngine.getGameRules().doMobSpawning);

    // 14. Despachar eventos confirmados
    dispatchEvents();

    m_clock.advanceTick();
}

void WorldSession::notifyTerrainChange(int x, int y, int z) {
    Math::Vec3 changePos(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, static_cast<float>(z) + 0.5f);
    for (auto& mob : m_mobs) {
        if ((mob.position - changePos).length() < 24.0f) {
            mob.invalidatePath();
        }
    }
}

void WorldSession::explode(const Math::Vec3& pos, float radius, bool causeBlockGriefing) {
    uint32_t blocksDestroyed = 0;

    // 1. Destrucción de bloques en radio esférico
    if (causeBlockGriefing) {
        int minX = static_cast<int>(std::floor(pos.x - radius));
        int maxX = static_cast<int>(std::floor(pos.x + radius));
        int minY = std::max(Chunk::MIN_Y, static_cast<int>(std::floor(pos.y - radius)));
        int maxY = std::min(Chunk::MAX_Y - 1, static_cast<int>(std::floor(pos.y + radius)));
        int minZ = static_cast<int>(std::floor(pos.z - radius));
        int maxZ = static_cast<int>(std::floor(pos.z + radius));

        const auto& reg = BlockRegistry::global();

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                for (int z = minZ; z <= maxZ; ++z) {
                    Math::Vec3 bCenter(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f,
                                       static_cast<float>(z) + 0.5f);
                    float dSq = (bCenter - pos).dot(bCenter - pos);
                    if (dSq <= radius * radius) {
                        int cx = (x >= 0) ? (x / 16) : ((x - 15) / 16);
                        int cz = (z >= 0) ? (z / 16) : ((z - 15) / 16);
                        if (m_world.isChunkLoaded(cx, cz)) {
                            BlockStateId st = m_world.getBlockState(x, y, z);
                            if (st != AIR_BLOCK_STATE) {
                                const auto& props = reg.get(st);
                                if (props.legacyId != BlockID::Bedrock && props.hardness >= 0.0f) {
                                    m_world.setBlockState(x, y, z, AIR_BLOCK_STATE);
                                    blocksDestroyed++;
                                    if (DeterministicRandom::hashBelow(
                                            100,
                                            static_cast<uint32_t>(x) * 73856093u ^ static_cast<uint32_t>(z) * 19349663u,
                                            static_cast<uint64_t>(y), m_clock.getTickCount()) < 30) {
                                        ItemId dropId = ItemRegistry::global().getItemForBlock(st);
                                        if (dropId != EMPTY_ITEM_ID) {
                                            spawnDroppedItem(ItemStack(dropId, 1), bCenter,
                                                             Math::Vec3(0.0f, 1.0f, 0.0f), 10);
                                        }
                                    }
                                    notifyTerrainChange(x, y, z);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 2. Daño y empuje a criaturas
    for (auto& mob : m_mobs) {
        if (!mob.isAlive || mob.isDead) continue;
        Math::Vec3 diff = mob.position - pos;
        float dist = diff.length();
        if (dist <= radius) {
            float damageFactor = 1.0f - (dist / radius);
            float dmg = damageFactor * 24.0f;
            bool killed = mob.takeDamage(dmg, &pos, INVALID_ENTITY_ID);
            m_pendingEvents.push_back(MobHurtEvent{
                .entityId = mob.id, .attackerId = INVALID_ENTITY_ID, .damage = dmg, .newHealth = mob.health});
            if (killed) {
                m_pendingEvents.push_back(
                    MobDiedEvent{.entityId = mob.id, .killerId = INVALID_ENTITY_ID, .deathPosition = mob.position});
                auto loot = mob.generateLoot();
                for (auto& st : loot) spawnDroppedItem(st, mob.position, Math::Vec3(0.0f, 1.0f, 0.0f), 10);
                spawnExperienceOrb(mob.position, mob.getExperienceDrop());
            }
        }
    }

    // 3. Daño y empuje al jugador
    if (m_player.isAlive) {
        Math::Vec3 diff = m_player.position - pos;
        float dist = diff.length();
        if (dist <= radius) {
            float damageFactor = 1.0f - (dist / radius);
            float dmg = damageFactor * 24.0f;
            applyDamage(dmg, DamageCause::Generic, &pos);
            if (dist > 0.001f) {
                Math::Vec3 push = diff * (1.0f / dist);
                m_player.velocity.x += push.x * 8.0f * damageFactor;
                m_player.velocity.z += push.z * 8.0f * damageFactor;
                m_player.velocity.y += 6.0f * damageFactor;
                m_player.isGrounded = false;
            }
        }
    }

    m_pendingEvents.push_back(ExplosionEvent{.position = pos, .radius = radius, .blocksDestroyed = blocksDestroyed});
}

void WorldSession::applyDamage(float amount, DamageCause cause, const Math::Vec3* sourcePos) {
    if (!m_player.isAlive || amount <= 0.0f) return;
    if (m_player.isSpectator()) return;                               // Spectator invulnerable
    if (m_player.gameMode == 1 && cause != DamageCause::Void) return; // Creative invulnerable

    // Reglas de juego (GameRules)
    const auto& rules = m_commandEngine.getGameRules();
    if (cause == DamageCause::Fall && !rules.fallDamage) return;
    if ((cause == DamageCause::Fire || cause == DamageCause::Lava) && !rules.fireDamage) return;
    if (cause == DamageCause::Drowning && !rules.drowningDamage) return;

    if (cause != DamageCause::Void && m_player.hurtTime > 0) return; // Invulnerability window

    // Bloqueo frontal con escudo
    if (m_player.isBlocking && cause != DamageCause::Void && cause != DamageCause::Suffocation &&
        cause != DamageCause::Drowning && cause != DamageCause::Starvation) {
        if (!sourcePos) {
            // Si no se especifica posición de origen, se asume frontal
            return;
        }
        float yawRad = Math::radians(m_player.yaw);
        Math::Vec3 lookDir(std::cos(yawRad), 0.0f, std::sin(yawRad));
        Math::Vec3 toSource = (*sourcePos - m_player.position);
        toSource.y = 0.0f;
        if (toSource.length() > 0.001f) {
            toSource = toSource.normalized();
            if (lookDir.dot(toSource) > 0.0f) {
                // Escudo bloquea completamente el daño frontal
                return;
            }
        } else {
            return;
        }
    }

    // Reducción por armadura
    if (cause != DamageCause::Void && cause != DamageCause::Suffocation && cause != DamageCause::Drowning &&
        cause != DamageCause::Starvation) {
        float armor = m_player.getTotalArmorPoints();
        if (armor > 0.0f) {
            float reduction = std::clamp(armor / 25.0f, 0.0f, 0.8f);
            amount *= (1.0f - reduction);
        }
    }

    m_player.health = std::max(0.0f, m_player.health - amount);
    if (cause != DamageCause::Void) {
        m_player.hurtTime = 10; // 10 ticks = 0.5s
    }

    m_pendingEvents.push_back(
        PlayerHurtEvent{.playerEntityId = m_player.id, .damage = amount, .cause = cause, .newHealth = m_player.health});

    if (m_player.health <= 0.0f) {
        m_player.isAlive = false;
        m_player.deathTime = 0;
        m_player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
        m_statisticsManager.incrementGeneral("deaths");
        m_pendingEvents.push_back(
            PlayerDiedEvent{.playerEntityId = m_player.id, .deathPosition = m_player.position, .cause = cause});
        dropAllInventoryOnDeath();
    }
}

void WorldSession::dropAllInventoryOnDeath() {
    if (m_commandEngine.getGameRules().keepInventory) return;

    returnCursorItemToInventory();
    returnCraftingGridToInventory();

    Math::Vec3 dropCenter = m_player.position + Math::Vec3(0.0f, 0.5f, 0.0f);
    for (size_t i = 0; i < m_player.inventory.getSize(); ++i) {
        auto& stack = m_player.inventory.getSlot(i);
        if (!stack.isEmpty()) {
            float angle = static_cast<float>(i) * 0.35f;
            Math::Vec3 vel(std::cos(angle) * 1.5f, 2.5f, std::sin(angle) * 1.5f);
            spawnDroppedItem(stack, dropCenter, vel, 20);
            stack.clear();
        }
    }

    if (!m_player.cursorItem.isEmpty()) {
        spawnDroppedItem(m_player.cursorItem, dropCenter, Math::Vec3(0.0f, 2.0f, 0.0f), 20);
        m_player.cursorItem.clear();
    }
    if (!m_player.offhandItem.isEmpty()) {
        spawnDroppedItem(m_player.offhandItem, dropCenter, Math::Vec3(0.0f, 2.0f, 0.0f), 20);
        m_player.offhandItem.clear();
    }

    m_player.syncHotbar();

    if (m_isContainerOpen) {
        m_isContainerOpen = false;
        m_openContainerKind = OpenContainerKind::None;
        m_pendingEvents.push_back(ContainerClosedEvent{.playerEntityId = m_player.id});
    }
}

Math::Vec3 WorldSession::findSafeSpawn(const Math::Vec3& target) {
    int startX = static_cast<int>(std::floor(target.x));
    int startY = std::clamp(static_cast<int>(std::floor(target.y)), Chunk::MIN_Y + 1, Chunk::MAX_Y - 3);
    int startZ = static_cast<int>(std::floor(target.z));

    const auto& reg = BlockRegistry::global();

    for (int r = 0; r <= 4; ++r) {
        for (int dx = -r; dx <= r; ++dx) {
            for (int dz = -r; dz <= r; ++dz) {
                int x = startX + dx;
                int z = startZ + dz;
                for (int dy = -4; dy <= 4; ++dy) {
                    int y = startY + dy;
                    if (y <= Chunk::MIN_Y || y >= Chunk::MAX_Y - 2) continue;

                    BlockStateId below = m_world.getBlockState(x, y - 1, z);
                    BlockStateId feet = m_world.getBlockState(x, y, z);
                    BlockStateId head = m_world.getBlockState(x, y + 1, z);

                    bool belowSolid = reg.get(below).isSolid;
                    bool feetSolid = reg.get(feet).isSolid;
                    bool headSolid = reg.get(head).isSolid;

                    if (belowSolid && !feetSolid && !headSolid) {
                        return Math::Vec3(static_cast<float>(x) + 0.5f, static_cast<float>(y),
                                          static_cast<float>(z) + 0.5f);
                    }
                }
            }
        }
    }
    return target;
}

void WorldSession::tickTrialSpawner(TrialSpawnerBlockEntity& ts) {
    ts.tick(); // enfriamiento

    const BlockPos sp = ts.getPos();
    const Math::Vec3 center(static_cast<float>(sp.x) + 0.5f, static_cast<float>(sp.y) + 0.5f,
                            static_cast<float>(sp.z) + 0.5f);
    switch (ts.getState()) {
        case TrialSpawnerState::Inactive:
            if (!m_player.isSpectator() && m_player.isAlive && ts.isPlayerInRange(m_player.position)) {
                ts.activate();
            }
            break;
        case TrialSpawnerState::ActiveSpawning: {
            // Oleada: criaturas de desafío alrededor del generador, en huecos con suelo
            const auto& reg = BlockRegistry::global();
            uint32_t count = ts.isOminous() ? 3u : 2u;
            static constexpr MobType WAVE_TYPES[] = {MobType::Zombie, MobType::Skeleton, MobType::Whirl,
                                                     MobType::SwampSkeleton};
            uint32_t spawned = 0;
            for (int attempt = 0; attempt < 24 && spawned < count; ++attempt) {
                uint64_t h = (static_cast<uint64_t>(m_clock.getTickCount()) * 2654435761ULL) ^
                             (static_cast<uint64_t>(attempt) * 40503ULL) ^
                             (static_cast<uint64_t>(static_cast<uint32_t>(sp.x)) << 20) ^
                             static_cast<uint64_t>(static_cast<uint32_t>(sp.z));
                int x = sp.x + static_cast<int>(h % 7) - 3;
                int z = sp.z + static_cast<int>((h >> 8) % 7) - 3;
                for (int dy = -2; dy <= 2; ++dy) {
                    int y = sp.y + dy;
                    if (reg.get(m_world.getBlockState(x, y - 1, z)).isSolid &&
                        !reg.get(m_world.getBlockState(x, y, z)).isSolid &&
                        !reg.get(m_world.getBlockState(x, y + 1, z)).isSolid) {
                        EntityId id = m_nextEntityId++;
                        MobType type = WAVE_TYPES[(ts.getCurrentWave() + spawned) % 4];
                        m_mobs.emplace_back(id, type,
                                            Math::Vec3(static_cast<float>(x) + 0.5f, static_cast<float>(y),
                                                       static_cast<float>(z) + 0.5f));
                        m_mobs.back().targetEntityId = m_player.id;
                        ts.addActiveMob(static_cast<uint32_t>(id));
                        ++spawned;
                        break;
                    }
                }
            }
            if (spawned > 0) {
                ts.setState(TrialSpawnerState::WaitingForWaveDefeat);
            }
            break;
        }
        case TrialSpawnerState::WaitingForWaveDefeat: {
            // Criaturas desaparecidas por cualquier causa (despawn, caída al vacío) cuentan como derrotadas
            std::vector<uint32_t> gone;
            for (uint32_t id : ts.getActiveMobs()) {
                bool alive = std::any_of(m_mobs.begin(), m_mobs.end(),
                                         [id](const MobEntity& m) { return m.id == id && m.isAlive && !m.isDead; });
                if (!alive) gone.push_back(id);
            }
            for (uint32_t id : gone) ts.onMobDefeated(id);
            break;
        }
        case TrialSpawnerState::EjectingRewards:
            for (auto& reward : ts.generateRewards()) {
                spawnDroppedItem(reward, center + Math::Vec3(0.0f, 0.8f, 0.0f), Math::Vec3(0.0f, 3.0f, 0.0f), 10);
            }
            ts.resetToCooldown();
            break;
        case TrialSpawnerState::Cooldown: break;
    }
}

std::optional<Math::Vec3> WorldSession::findSurfaceSpawn(int x, int z, int searchRadius) const {
    const auto& reg = BlockRegistry::global();
    auto columnSurface = [&](int cx, int cz) -> std::optional<Math::Vec3> {
        if (!m_world.isChunkLoaded(floorDiv16(cx), floorDiv16(cz))) return std::nullopt;
        for (int y = Chunk::MAX_Y - 3; y > Chunk::MIN_Y; --y) {
            const auto& below = reg.get(m_world.getBlockState(cx, y - 1, cz));
            if (below.legacyId == BlockID::Air) continue;
            // Primer bloque no aéreo desde arriba: debe ser suelo firme, no agua, lava ni hojas
            if (!below.isSolid || below.legacyId == BlockID::Leaves) return std::nullopt;
            if (reg.get(m_world.getBlockState(cx, y, cz)).isSolid ||
                reg.get(m_world.getBlockState(cx, y + 1, cz)).isSolid) {
                return std::nullopt;
            }
            return Math::Vec3(static_cast<float>(cx) + 0.5f, static_cast<float>(y), static_cast<float>(cz) + 0.5f);
        }
        return std::nullopt;
    };
    for (int r = 0; r <= searchRadius; ++r) {
        for (int dx = -r; dx <= r; ++dx) {
            for (int dz = -r; dz <= r; ++dz) {
                if (std::max(std::abs(dx), std::abs(dz)) != r) continue;
                if (auto p = columnSurface(x + dx, z + dz)) return p;
            }
        }
    }
    return std::nullopt;
}

bool WorldSession::ensurePlayerAboveGround(bool freshWorld) {
    const auto& reg = BlockRegistry::global();
    int bx = static_cast<int>(std::floor(m_player.position.x));
    int by = static_cast<int>(std::floor(m_player.position.y));
    int bz = static_cast<int>(std::floor(m_player.position.z));
    bool buried =
        reg.get(m_world.getBlockState(bx, by, bz)).isSolid || reg.get(m_world.getBlockState(bx, by + 1, bz)).isSolid;
    if (!freshWorld && !buried) return false;

    auto surface = findSurfaceSpawn(bx, bz);
    if (!surface) return false;
    m_player.position = *surface;
    m_player.prevPosition = *surface;
    m_player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
    m_player.fallDistance = 0.0f;
    if (freshWorld) {
        // Punto de reaparición del mundo (sin cama)
        m_player.spawnPoint = *surface;
        m_player.hasCustomSpawnPoint = false;
    }
    return true;
}

void WorldSession::respawnPlayer() {
    // Punto del mundo fijado al crear la partida; los mundos antiguos usan el origen clásico
    Math::Vec3 targetSpawn = (m_player.spawnPoint.y > static_cast<float>(Chunk::MIN_Y)) ? m_player.spawnPoint
                                                                                        : Math::Vec3(8.0f, 68.0f, 8.0f);
    if (m_player.hasCustomSpawnPoint) {
        int bx = static_cast<int>(std::floor(m_player.spawnPoint.x));
        int by = static_cast<int>(std::floor(m_player.spawnPoint.y - 1.0f));
        int bz = static_cast<int>(std::floor(m_player.spawnPoint.z));
        BlockStateId st = m_world.getBlockState(bx, by, bz);
        const auto& props = BlockRegistry::global().get(st);
        if (props.legacyId == BlockID::Bed || props.namespacedId == "opencraft:bed") {
            targetSpawn = m_player.spawnPoint;
        } else {
            m_player.hasCustomSpawnPoint = false;
        }
    }

    Math::Vec3 safePos = findSafeSpawn(targetSpawn);
    if (safePos.x == targetSpawn.x && safePos.y == targetSpawn.y && safePos.z == targetSpawn.z) {
        // Sin hueco cercano: buscar la superficie de la columna
        if (auto surface = findSurfaceSpawn(static_cast<int>(std::floor(targetSpawn.x)),
                                            static_cast<int>(std::floor(targetSpawn.z)))) {
            safePos = *surface;
        }
    }
    m_player.position = safePos;
    m_player.prevPosition = safePos;
    m_player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
    m_player.health = m_player.maxHealth;
    m_player.hunger = 20.0f;
    m_player.saturation = 5.0f;
    m_player.exhaustion = 0.0f;
    m_player.air = m_player.maxAir;
    m_player.fallDistance = 0.0f;
    m_player.hurtTime = 0;
    m_player.deathTime = 0;
    m_player.isAlive = true;

    if (m_isHardcore) {
        m_player.gameMode = 3; // Spectator permanente
        m_player.isFlying = true;
    }

    m_pendingEvents.push_back(PlayerRespawnedEvent{.playerEntityId = m_player.id, .spawnPosition = safePos});
}

bool WorldSession::isHeadInWater() const {
    Math::Vec3 eye = m_player.getEyePosition();
    int x = static_cast<int>(std::floor(eye.x));
    int y = static_cast<int>(std::floor(eye.y));
    int z = static_cast<int>(std::floor(eye.z));
    BlockStateId st = m_world.getBlockState(x, y, z);
    const auto& props = BlockRegistry::global().get(st);
    return props.legacyId == BlockID::Water || props.legacyId == BlockID::FlowingWater ||
           props.namespacedId == "opencraft:water" || props.namespacedId == "opencraft:flowing_water";
}

bool WorldSession::isFeetInWater() const {
    int x = static_cast<int>(std::floor(m_player.position.x));
    int y = static_cast<int>(std::floor(m_player.position.y));
    int z = static_cast<int>(std::floor(m_player.position.z));
    BlockStateId st = m_world.getBlockState(x, y, z);
    const auto& props = BlockRegistry::global().get(st);
    return props.legacyId == BlockID::Water || props.legacyId == BlockID::FlowingWater ||
           props.namespacedId == "opencraft:water" || props.namespacedId == "opencraft:flowing_water";
}

bool WorldSession::isFeetInLava() const {
    int x = static_cast<int>(std::floor(m_player.position.x));
    int y = static_cast<int>(std::floor(m_player.position.y));
    int z = static_cast<int>(std::floor(m_player.position.z));
    BlockStateId st = m_world.getBlockState(x, y, z);
    const auto& props = BlockRegistry::global().get(st);
    return props.legacyId == BlockID::Lava || props.legacyId == BlockID::FlowingLava ||
           props.namespacedId == "opencraft:lava" || props.namespacedId == "opencraft:flowing_lava";
}

bool WorldSession::isHeadInLava() const {
    Math::Vec3 eye = m_player.getEyePosition();
    int x = static_cast<int>(std::floor(eye.x));
    int y = static_cast<int>(std::floor(eye.y));
    int z = static_cast<int>(std::floor(eye.z));
    BlockStateId st = m_world.getBlockState(x, y, z);
    const auto& props = BlockRegistry::global().get(st);
    return props.legacyId == BlockID::Lava || props.legacyId == BlockID::FlowingLava ||
           props.namespacedId == "opencraft:lava" || props.namespacedId == "opencraft:flowing_lava";
}

void WorldSession::setWeather(WeatherState state, uint32_t ticks) {
    m_weather.state = state;
    m_weather.ticksRemaining = ticks;
    m_weather.rainStrength = (state == WeatherState::Clear) ? 0.0f : 1.0f;
}

bool WorldSession::isHeadInSolid() const {
    Math::Vec3 eye = m_player.getEyePosition();
    int x = static_cast<int>(std::floor(eye.x));
    int y = static_cast<int>(std::floor(eye.y));
    int z = static_cast<int>(std::floor(eye.z));
    BlockStateId st = m_world.getBlockState(x, y, z);
    const auto& props = BlockRegistry::global().get(st);
    return props.isSolid && props.isOpaque;
}

void WorldSession::update(double dt) {
    m_clock.addTime(dt);
    while (m_clock.shouldTick()) {
        tick();
    }
}

WorldSnapshot WorldSession::getSnapshot() const {
    WorldSnapshot snap;
    snap.tick = m_clock.getTickCount();
    snap.player = EntitySnapshot{.id = m_player.id,
                                 .prevPosition = m_player.prevPosition,
                                 .currentPosition = m_player.position,
                                 .prevYaw = m_player.prevYaw,
                                 .currentYaw = m_player.yaw,
                                 .prevPitch = m_player.prevPitch,
                                 .currentPitch = m_player.pitch};
    snap.selectedSlot = m_player.selectedSlot;
    snap.hotbar = m_player.hotbar;
    snap.health = m_player.health;
    snap.hunger = m_player.hunger;
    snap.saturation = m_player.saturation;
    snap.air = m_player.air;
    snap.isAlive = m_player.isAlive;
    snap.isSprinting = m_player.isSprinting;
    snap.isSneaking = m_player.isSneaking;
    snap.gameMode = m_player.gameMode;

    snap.inventorySlots = m_player.inventory.getSlots();
    snap.cursorItem = m_player.cursorItem;
    snap.isContainerOpen = m_isContainerOpen;
    snap.openContainerKind = m_openContainerKind;
    snap.openContainerPos = m_openContainerPos;
    snap.craftingGrid = m_craftingGrid;
    snap.craftingOutput = m_craftingOutput;
    snap.miningProgress = m_miningProgress;
    snap.miningBlockPos = m_miningPos;

    if (m_isContainerOpen) {
        if (m_openContainerKind == OpenContainerKind::Chest) {
            auto chest = m_world.getChestBlockEntity(m_openContainerPos.x, m_openContainerPos.y, m_openContainerPos.z);
            if (chest) {
                snap.containerSlots = chest->getInventory().getSlots();
                snap.containerRevision = chest->getRevision();
            }
        } else if (m_openContainerKind == OpenContainerKind::Furnace) {
            auto furnace =
                m_world.getFurnaceBlockEntity(m_openContainerPos.x, m_openContainerPos.y, m_openContainerPos.z);
            if (furnace) {
                snap.containerSlots = furnace->getInventory().getSlots();
                snap.containerRevision = furnace->getRevision();
                snap.furnaceLitTime = furnace->getLitTimeTicks();
                snap.furnaceLitDuration = furnace->getLitDurationTicks();
                snap.furnaceCookProgress = furnace->getCookProgressTicks();
                snap.furnaceCookTotal = furnace->getCookTotalTicks();
            }
        }
    }

    // Esquema v7: Experiencia, bloqueo y entidades vivas
    snap.xpLevel = m_player.xpLevel;
    snap.xpProgress = m_player.xpProgress;
    snap.isBlocking = m_player.isBlocking;

    snap.armorPoints = m_player.getTotalArmorPoints();
    snap.isFlying = m_player.isFlying;
    snap.isHeadInWater = isHeadInWater();
    snap.isHeadInLava = isHeadInLava();
    snap.dimension = static_cast<uint8_t>(m_currentDimension);
    snap.hurtTime = m_player.hurtTime;
    snap.deathTime = m_player.deathTime;
    snap.isHardcore = m_isHardcore;

    snap.itemEntities.reserve(m_itemEntities.size());
    for (const auto& it : m_itemEntities) {
        if (it.isDead || it.stack.isEmpty()) continue;
        snap.itemEntities.push_back({it.id, it.stack.itemId, it.stack.count, it.position, it.ageTicks});
    }

    snap.mobs.reserve(m_mobs.size());
    for (const auto& mob : m_mobs) {
        if (mob.isAlive && !mob.isDead) {
            WorldSnapshot::MobSnapshot ms;
            ms.id = mob.id;
            ms.mobType = static_cast<uint8_t>(mob.mobType);
            ms.aiState = static_cast<uint8_t>(mob.aiState);
            ms.transform = EntitySnapshot{.id = mob.id,
                                          .prevPosition = mob.prevPosition,
                                          .currentPosition = mob.position,
                                          .prevYaw = mob.prevYaw,
                                          .currentYaw = mob.yaw,
                                          .prevPitch = mob.prevPitch,
                                          .currentPitch = mob.pitch};
            ms.health = mob.health;
            ms.maxHealth = mob.maxHealth;
            ms.isIgnited = mob.isIgnited;
            ms.isDead = mob.isDead;
            snap.mobs.push_back(std::move(ms));
        }
    }

    snap.projectiles.reserve(m_projectiles.size());
    for (const auto& p : m_projectiles) {
        if (!p.isDead) {
            WorldSnapshot::ProjectileSnapshot ps;
            ps.id = p.id;
            ps.transform = EntitySnapshot{.id = p.id,
                                          .prevPosition = p.prevPosition,
                                          .currentPosition = p.position,
                                          .prevYaw = p.yaw,
                                          .currentYaw = p.yaw,
                                          .prevPitch = p.pitch,
                                          .currentPitch = p.pitch};
            ps.inGround = p.inGround;
            snap.projectiles.push_back(std::move(ps));
        }
    }

    snap.remotePlayers.reserve(m_connectedPlayers.size());
    for (const auto& [id, rp] : m_connectedPlayers) {
        if (rp.isAlive) {
            WorldSnapshot::RemotePlayerSnapshot rps;
            rps.id = id;
            auto itName = m_playerNames.find(id);
            rps.name = (itName != m_playerNames.end()) ? itName->second : "Player";
            rps.transform = EntitySnapshot{.id = id,
                                           .prevPosition = rp.prevPosition,
                                           .currentPosition = rp.position,
                                           .prevYaw = rp.prevYaw,
                                           .currentYaw = rp.yaw,
                                           .prevPitch = rp.prevPitch,
                                           .currentPitch = rp.pitch};
            rps.health = rp.health;
            rps.isSprinting = rp.isSprinting;
            rps.isSneaking = rp.isSneaking;
            rps.isBlocking = rp.isBlocking;
            snap.remotePlayers.push_back(std::move(rps));
        }
    }

    return snap;
}

uint64_t WorldSession::computeStateHash() const noexcept {
    uint64_t hash = m_world.computeStateHash();
    constexpr uint64_t fnvPrime = 1099511628211ULL;

    auto hashFloat = [&hash, fnvPrime](float v) {
        uint32_t bits = 0;
        std::memcpy(&bits, &v, sizeof(bits));
        for (int i = 0; i < 4; ++i) {
            hash ^= static_cast<uint64_t>((bits >> (i * 8)) & 0xFF);
            hash *= fnvPrime;
        }
    };

    hashFloat(m_player.position.x);
    hashFloat(m_player.position.y);
    hashFloat(m_player.position.z);
    hashFloat(m_player.velocity.x);
    hashFloat(m_player.velocity.y);
    hashFloat(m_player.velocity.z);
    hash ^= static_cast<uint64_t>(m_player.isGrounded ? 1 : 0);
    hash *= fnvPrime;

    // Hashear inventario del jugador
    for (size_t i = 0; i < m_player.inventory.getSize(); ++i) {
        const auto& stack = m_player.inventory.getSlot(i);
        hash ^= static_cast<uint64_t>(stack.itemId) << 16 | static_cast<uint64_t>(stack.count);
        hash *= fnvPrime;
        hash ^= static_cast<uint64_t>(stack.durability);
        hash *= fnvPrime;
    }

    // Hashear jugadores remotos en orden determinista de ID
    if (!m_connectedPlayers.empty()) {
        std::vector<EntityId> sortedIds;
        sortedIds.reserve(m_connectedPlayers.size());
        for (const auto& [id, _] : m_connectedPlayers) {
            sortedIds.push_back(id);
        }
        std::sort(sortedIds.begin(), sortedIds.end());
        for (EntityId id : sortedIds) {
            const auto& p = m_connectedPlayers.at(id);
            hash ^= static_cast<uint64_t>(id);
            hash *= fnvPrime;
            hashFloat(p.position.x);
            hashFloat(p.position.y);
            hashFloat(p.position.z);
            hashFloat(p.health);
            for (size_t i = 0; i < p.inventory.getSize(); ++i) {
                const auto& stack = p.inventory.getSlot(i);
                hash ^= static_cast<uint64_t>(stack.itemId) << 16 | static_cast<uint64_t>(stack.count);
                hash *= fnvPrime;
            }
        }
    }

    // Hashear entidades de objetos tirados
    hash ^= static_cast<uint64_t>(m_itemEntities.size());
    hash *= fnvPrime;
    for (const auto& item : m_itemEntities) {
        hash ^= static_cast<uint64_t>(item.id);
        hash *= fnvPrime;
        hash ^= static_cast<uint64_t>(item.stack.itemId) << 16 | static_cast<uint64_t>(item.stack.count);
        hash *= fnvPrime;
    }

    // Hashear criaturas activas
    hash ^= static_cast<uint64_t>(m_mobs.size());
    hash *= fnvPrime;
    for (const auto& mob : m_mobs) {
        hash ^= static_cast<uint64_t>(mob.id);
        hash *= fnvPrime;
        hashFloat(mob.position.x);
        hashFloat(mob.position.y);
        hashFloat(mob.position.z);
        hashFloat(mob.health);
        hash ^= static_cast<uint64_t>(mob.aiState);
        hash *= fnvPrime;
    }

    return hash;
}

} // namespace OpenCraft
