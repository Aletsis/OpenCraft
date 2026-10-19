#include "sim/MobEntity.hpp"
#include "world/IdAliases.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"
#include <cmath>
#include <queue>
#include <unordered_map>
#include <algorithm>

namespace OpenCraft {

std::string mobTypeToString(MobType type) {
    switch (type) {
        case MobType::Cow: return "opencraft:cow";
        case MobType::Zombie: return "opencraft:zombie";
        case MobType::Skeleton: return "opencraft:skeleton";
        case MobType::Lurker: return "opencraft:lurker";
        case MobType::Sheep: return "opencraft:sheep";
        case MobType::Pig: return "opencraft:pig";
        case MobType::Chicken: return "opencraft:chicken";
        case MobType::Wolf: return "opencraft:wolf";
        case MobType::Villager: return "opencraft:villager";
        case MobType::Whirl: return "opencraft:whirl";
        case MobType::SwampSkeleton: return "opencraft:swamp_skeleton";
    }
    return "opencraft:zombie";
}

MobType mobTypeFromString(std::string_view rawName) {
    // Los mundos anteriores guardan "opencraft:acechador", "opencraft:torbellino"...: se traducen al nombre vigente
    const std::string canonicalName = IdAliases::canonical(rawName);
    std::string_view name = canonicalName;
    if (name == "opencraft:cow") return MobType::Cow;
    if (name == "opencraft:skeleton") return MobType::Skeleton;
    if (name == "opencraft:lurker") return MobType::Lurker;
    if (name == "opencraft:sheep") return MobType::Sheep;
    if (name == "opencraft:pig") return MobType::Pig;
    if (name == "opencraft:chicken") return MobType::Chicken;
    if (name == "opencraft:wolf") return MobType::Wolf;
    if (name == "opencraft:villager") return MobType::Villager;
    if (name == "opencraft:whirl") return MobType::Whirl;
    if (name == "opencraft:swamp_skeleton") return MobType::SwampSkeleton;
    return MobType::Zombie;
}

MobFamily getMobFamily(MobType type) {
    switch (type) {
        case MobType::Cow:
        case MobType::Sheep:
        case MobType::Pig:
        case MobType::Chicken:
        case MobType::Wolf:
        case MobType::Villager: return MobFamily::Passive;
        default: return MobFamily::Monster;
    }
}

MobEntity::MobEntity() {
    id = INVALID_ENTITY_ID;
    mobType = MobType::Zombie;
    width = 0.6f;
    height = 1.95f;
}

MobEntity::MobEntity(EntityId eId, MobType type, const Math::Vec3& pos) {
    id = eId;
    mobType = type;
    position = pos;
    prevPosition = pos;

    switch (type) {
        case MobType::Cow:
            health = 10.0f;
            maxHealth = 10.0f;
            attackDamage = 0.0f;
            movementSpeed = 2.0f;
            width = 0.9f;
            height = 1.4f;
            break;
        case MobType::Sheep:
            health = 8.0f;
            maxHealth = 8.0f;
            attackDamage = 0.0f;
            movementSpeed = 2.0f;
            width = 0.9f;
            height = 1.3f;
            break;
        case MobType::Pig:
            health = 10.0f;
            maxHealth = 10.0f;
            attackDamage = 0.0f;
            movementSpeed = 2.2f;
            width = 0.9f;
            height = 0.9f;
            break;
        case MobType::Chicken:
            health = 4.0f;
            maxHealth = 4.0f;
            attackDamage = 0.0f;
            movementSpeed = 2.0f;
            width = 0.4f;
            height = 0.7f;
            break;
        case MobType::Wolf:
            health = 8.0f;
            maxHealth = 8.0f;
            attackDamage = 4.0f;
            movementSpeed = 3.0f;
            width = 0.6f;
            height = 0.85f;
            break;
        case MobType::Villager:
            health = 20.0f;
            maxHealth = 20.0f;
            attackDamage = 0.0f;
            movementSpeed = 2.0f;
            width = 0.6f;
            height = 1.95f;
            isPersistent = true;
            break;
        case MobType::Zombie:
            health = 20.0f;
            maxHealth = 20.0f;
            attackDamage = 3.0f;
            movementSpeed = 2.8f;
            width = 0.6f;
            height = 1.95f;
            break;
        case MobType::Skeleton:
            health = 20.0f;
            maxHealth = 20.0f;
            attackDamage = 2.0f;
            movementSpeed = 2.7f;
            width = 0.6f;
            height = 1.99f;
            break;
        case MobType::Lurker:
            health = 20.0f;
            maxHealth = 20.0f;
            attackDamage = 0.0f;
            movementSpeed = 2.8f;
            width = 0.9f; // cuerpo bajo y ancho del acechador
            height = 0.95f;
            break;
        case MobType::Whirl:
            health = 30.0f;
            maxHealth = 30.0f;
            attackDamage = 3.0f;
            movementSpeed = 4.0f;
            width = 0.6f;
            height = 1.77f;
            break;
        case MobType::SwampSkeleton:
            health = 16.0f;
            maxHealth = 16.0f;
            attackDamage = 2.0f;
            movementSpeed = 2.6f;
            width = 0.6f;
            height = 1.99f;
            break;
    }
}

bool MobEntity::takeDamage(float amount, const Math::Vec3* attackerPos, EntityId attackerId) {
    (void)attackerId;
    if (!isAlive || isDead) return false;

    if (hurtTime > 0) {
        // En periodo de invulnerabilidad
        return false;
    }

    hurtTime = 10;
    health -= amount;

    // Aplicar empuje (retroceso / knockback)
    if (attackerPos) {
        Math::Vec3 dir = position - *attackerPos;
        dir.y = 0.0f;
        float len = dir.length();
        if (len > 0.001f) {
            dir = dir * (1.0f / len);
            velocity.x += dir.x * 6.0f;
            velocity.z += dir.z * 6.0f;
            velocity.y = 4.0f;
            isGrounded = false;
        }
    }

    if (health <= 0.0f) {
        health = 0.0f;
        isAlive = false;
        isDead = true;
        return true; // Provocó defunción
    }

    // Comportamiento pasivo ante daño: huir en pánico
    if (mobType == MobType::Cow || mobType == MobType::Sheep || mobType == MobType::Pig ||
        mobType == MobType::Chicken || mobType == MobType::Villager) {
        fleeTicks = 100; // 5 segundos de carrera descontrolada
        aiState = AIState::Flee;
        invalidatePath();
    }

    return false;
}

std::vector<ItemStack> MobEntity::generateLoot() const {
    std::vector<ItemStack> loot;
    const auto& itemReg = ItemRegistry::global();

    switch (mobType) {
        case MobType::Cow: {
            ItemId beefId = itemReg.getId("opencraft:raw_beef");
            ItemId leatherId = itemReg.getId("opencraft:leather");
            if (beefId != EMPTY_ITEM_ID) loot.emplace_back(beefId, 2);
            if (leatherId != EMPTY_ITEM_ID) loot.emplace_back(leatherId, 1);
            break;
        }
        case MobType::Sheep: {
            ItemId beefId = itemReg.getId("opencraft:raw_beef");
            ItemId stringId = itemReg.getId("opencraft:string");
            if (beefId != EMPTY_ITEM_ID) loot.emplace_back(beefId, 1);
            if (stringId != EMPTY_ITEM_ID) loot.emplace_back(stringId, 1);
            break;
        }
        case MobType::Pig: {
            ItemId beefId = itemReg.getId("opencraft:raw_beef");
            if (beefId != EMPTY_ITEM_ID) loot.emplace_back(beefId, 2);
            break;
        }
        case MobType::Chicken: {
            ItemId featherId = itemReg.getId("opencraft:feather");
            if (featherId != EMPTY_ITEM_ID) loot.emplace_back(featherId, 1);
            break;
        }
        case MobType::Wolf: {
            ItemId boneId = itemReg.getId("opencraft:bone");
            if (boneId != EMPTY_ITEM_ID) loot.emplace_back(boneId, 1);
            break;
        }
        case MobType::Villager: {
            // Los aldeanos no sueltan botín al morir
            break;
        }
        case MobType::Zombie: {
            ItemId fleshId = itemReg.getId("opencraft:rotten_flesh");
            if (fleshId != EMPTY_ITEM_ID) loot.emplace_back(fleshId, 1);
            break;
        }
        case MobType::Skeleton: {
            ItemId boneId = itemReg.getId("opencraft:bone");
            ItemId arrowId = itemReg.getId("opencraft:arrow");
            if (boneId != EMPTY_ITEM_ID) loot.emplace_back(boneId, 1);
            if (arrowId != EMPTY_ITEM_ID) loot.emplace_back(arrowId, 2);
            break;
        }
        case MobType::Lurker: {
            ItemId powderId = itemReg.getId("opencraft:gunpowder");
            if (powderId != EMPTY_ITEM_ID) loot.emplace_back(powderId, 1);
            break;
        }
        case MobType::Whirl: {
            ItemId rodId = itemReg.getId("opencraft:whirl_rod");
            if (rodId != EMPTY_ITEM_ID) loot.emplace_back(rodId, 2);
            break;
        }
        case MobType::SwampSkeleton: {
            ItemId boneId = itemReg.getId("opencraft:bone");
            ItemId arrowId = itemReg.getId("opencraft:arrow");
            if (boneId != EMPTY_ITEM_ID) loot.emplace_back(boneId, 1);
            if (arrowId != EMPTY_ITEM_ID) loot.emplace_back(arrowId, 2);
            break;
        }
    }
    return loot;
}

uint32_t MobEntity::getExperienceDrop() const {
    switch (mobType) {
        case MobType::Cow:
        case MobType::Sheep:
        case MobType::Pig:
        case MobType::Chicken: return 2;
        case MobType::Villager: return 0;
        default: return 5;
    }
}

namespace {
struct PathNode {
    BlockPos pos;
    int gCost = 0;
    int hCost = 0;
    int fCost() const { return gCost + hCost; }
    BlockPos parent{0, 0, 0};
    bool hasParent = false;
};

struct CompareNode {
    bool operator()(const PathNode& a, const PathNode& b) const { return a.fCost() > b.fCost(); }
};

inline int heuristic(const BlockPos& a, const BlockPos& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z);
}
} // namespace

bool MobEntity::computePathTo(World& world, const BlockPos& targetPos, int maxNodes) {
    BlockPos startPos{static_cast<int>(std::floor(position.x)), static_cast<int>(std::floor(position.y)),
                      static_cast<int>(std::floor(position.z))};

    if (startPos == targetPos) {
        currentPath = {targetPos};
        pathIndex = 0;
        pathValid = true;
        return true;
    }

    const auto& reg = BlockRegistry::global();
    auto isStandable = [&](int x, int y, int z) -> bool {
        int cx = (x >= 0) ? (x / 16) : ((x - 15) / 16);
        int cz = (z >= 0) ? (z / 16) : ((z - 15) / 16);
        if (!world.isChunkLoaded(cx, cz)) return false;

        // Suelo sólido abajo, y 2 bloques libres para el cuerpo
        bool floorSolid = reg.get(world.getBlockState(x, y - 1, z)).isSolid;
        bool bodyClear = !reg.get(world.getBlockState(x, y, z)).isSolid;
        bool headClear = !reg.get(world.getBlockState(x, y + 1, z)).isSolid;
        return floorSolid && bodyClear && headClear;
    };

    std::priority_queue<PathNode, std::vector<PathNode>, CompareNode> openSet;
    std::unordered_map<BlockPos, PathNode, BlockPosHash> allNodes;

    PathNode startNode;
    startNode.pos = startPos;
    startNode.gCost = 0;
    startNode.hCost = heuristic(startPos, targetPos);
    openSet.push(startNode);
    allNodes[startPos] = startNode;

    std::unordered_map<BlockPos, bool, BlockPosHash> closedSet;
    int nodesEvaluated = 0;
    BlockPos closestPos = startPos;
    int closestDist = startNode.hCost;

    const int dx[] = {1, -1, 0, 0};
    const int dz[] = {0, 0, 1, -1};

    bool targetReached = false;
    while (!openSet.empty() && nodesEvaluated < maxNodes) {
        PathNode current = openSet.top();
        openSet.pop();

        if (closedSet[current.pos]) continue;
        closedSet[current.pos] = true;
        nodesEvaluated++;

        if (current.hCost < closestDist) {
            closestDist = current.hCost;
            closestPos = current.pos;
        }

        // Meta alcanzada o adyacente a la meta
        if (current.pos == targetPos || heuristic(current.pos, targetPos) <= 1) {
            closestPos = current.pos;
            targetReached = true;
            break;
        }

        for (int i = 0; i < 4; ++i) {
            int nx = current.pos.x + dx[i];
            int nz = current.pos.z + dz[i];

            // Probar nivel actual, salto hacia arriba (+1) y caída hacia abajo (-1..-2)
            const int dyOptions[] = {0, 1, -1, -2};
            for (int dy : dyOptions) {
                int ny = current.pos.y + dy;
                BlockPos nextPos{nx, ny, nz};

                if (closedSet[nextPos]) continue;

                if (isStandable(nx, ny, nz)) {
                    int newG = current.gCost + 1 + std::abs(dy);
                    auto it = allNodes.find(nextPos);
                    if (it == allNodes.end() || newG < it->second.gCost) {
                        PathNode neighbor;
                        neighbor.pos = nextPos;
                        neighbor.gCost = newG;
                        neighbor.hCost = heuristic(nextPos, targetPos);
                        neighbor.parent = current.pos;
                        neighbor.hasParent = true;
                        allNodes[nextPos] = neighbor;
                        openSet.push(neighbor);
                    }
                    break; // Solo una opción vertical válida por coordenada cardinal
                }
            }
        }
    }

    if (!targetReached) {
        currentPath.clear();
        pathIndex = 0;
        pathValid = false;
        return false;
    }

    // Reconstruir la ruta
    std::vector<BlockPos> path;
    BlockPos curr = closestPos;
    while (allNodes.find(curr) != allNodes.end() && allNodes[curr].hasParent) {
        path.push_back(curr);
        curr = allNodes[curr].parent;
    }
    std::reverse(path.begin(), path.end());

    currentPath = std::move(path);
    pathIndex = 0;
    pathValid = !currentPath.empty();
    return pathValid;
}

void MobEntity::tickAI(World& world, const PlayerEntity& player, std::vector<ProjectileEntity>& outProjectiles,
                       EntityId& nextEntityId, float sunlightFactor) {
    if (!isAlive || isDead) return;

    aiTicks++;
    hasMeleeHit = false;
    pendingMeleeDamage = 0.0f;
    hasExploded = false;

    if (hurtTime > 0) hurtTime--;
    if (attackCooldown > 0) attackCooldown--;

    int bx = static_cast<int>(std::floor(position.x));
    int by = static_cast<int>(std::floor(position.y));
    int bz = static_cast<int>(std::floor(position.z));

    // 1. Quemadura solar para hostiles no-muertos (Zombi y Esqueleto)
    if (mobType == MobType::Zombie || mobType == MobType::Skeleton) {
        if (sunlightFactor > 0.5f) {
            int cx = (bx >= 0) ? (bx / 16) : ((bx - 15) / 16);
            int cz = (bz >= 0) ? (bz / 16) : ((bz - 15) / 16);
            if (world.isChunkLoaded(cx, cz)) {
                uint8_t skyL = world.getSkyLight(bx, by + 2, bz);
                bool hasSolidRoof = false;
                for (int yCheck = by + 2; yCheck < Chunk::MAX_Y; ++yCheck) {
                    if (BlockRegistry::global().get(world.getBlockState(bx, yCheck, bz)).isSolid) {
                        hasSolidRoof = true;
                        break;
                    }
                }
                if (skyL >= 14 && !hasSolidRoof) {
                    fireTicks = 80; // Arde durante 4 segundos
                }
            }
        }
    }

    if (fireTicks > 0) {
        fireTicks--;
        if (aiTicks % 20 == 0) {
            takeDamage(1.0f);
            if (isDead) return;
        }
    }

    // 2. Comportamiento según arquetipo
    if (isSitting) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        aiState = AIState::Sitting;
        return;
    }

    if (isBaby) {
        babyAge++;
        if (babyAge >= 24000) {
            isBaby = false;
        }
    }

    if (loveTicks > 0) {
        loveTicks--;
        if (loveTicks == 0) {
            inLove = false;
        }
    }

    if (breedingCooldown > 0) {
        breedingCooldown--;
    }

    Math::Vec3 playerPos = player.position;
    Math::Vec3 diff = playerPos - position;
    float distSq = diff.dot(diff);
    float dist = std::sqrt(distSq);

    bool targetPlayer = player.isAlive && (player.gameMode == 0 || player.gameMode == 2) && (dist <= 16.0f);

    switch (mobType) {
        case MobType::Wolf: {
            if (isTamed && ownerPlayerId != INVALID_ENTITY_ID && !isSitting) {
                if (dist > 3.0f) {
                    aiState = AIState::Chase;
                    Math::Vec3 moveDir = (playerPos - position);
                    moveDir.y = 0.0f;
                    if (moveDir.length() > 0.01f) {
                        moveDir = moveDir.normalized();
                        velocity.x = moveDir.x * movementSpeed;
                        velocity.z = moveDir.z * movementSpeed;
                        yaw = Math::degrees(std::atan2(moveDir.x, moveDir.z));
                    }
                } else {
                    aiState = AIState::Idle;
                    velocity.x = 0.0f;
                    velocity.z = 0.0f;
                }
                break;
            }
            [[fallthrough]];
        }

        case MobType::Cow:
        case MobType::Sheep:
        case MobType::Pig:
        case MobType::Chicken:
        case MobType::Villager: {
            if (fleeTicks > 0) {
                fleeTicks--;
                aiState = AIState::Flee;
                // Correr alejándose del jugador o en dirección opuesta
                Math::Vec3 fleeDir = (distSq > 0.001f) ? (position - playerPos) : Math::Vec3(1.0f, 0.0f, 0.0f);
                fleeDir.y = 0.0f;
                fleeDir = fleeDir.normalized();
                velocity.x = fleeDir.x * 4.5f;
                velocity.z = fleeDir.z * 4.5f;
                yaw = Math::degrees(std::atan2(fleeDir.x, fleeDir.z));
            } else {
                // Deambular pacíficamente
                wander();
            }
            break;
        }

        case MobType::Zombie: {
            if (targetPlayer) {
                targetEntityId = player.id;
                aiState = AIState::Chase;

                // Si está en rango de ataque cuerpo a cuerpo (<= 1.5m)
                if (dist <= 1.5f) {
                    aiState = AIState::Attack;
                    velocity.x = 0.0f;
                    velocity.z = 0.0f;
                    if (attackCooldown == 0) {
                        attackCooldown = 20; // 1 ataque por segundo
                        hasMeleeHit = true;
                        pendingMeleeDamage = attackDamage;
                    }
                } else {
                    // Persecución mediante A* si es necesario o directa
                    if (pathCooldown > 0) pathCooldown--;
                    if (!pathValid || pathCooldown == 0) {
                        computePathTo(world,
                                      BlockPos{static_cast<int>(std::floor(playerPos.x)),
                                               static_cast<int>(std::floor(playerPos.y)),
                                               static_cast<int>(std::floor(playerPos.z))},
                                      128);
                        pathCooldown = 20; // Recalcular cada segundo
                    }

                    Math::Vec3 targetStep = playerPos;
                    if (pathValid && pathIndex < currentPath.size()) {
                        targetStep = Math::Vec3(static_cast<float>(currentPath[pathIndex].x) + 0.5f,
                                                static_cast<float>(currentPath[pathIndex].y),
                                                static_cast<float>(currentPath[pathIndex].z) + 0.5f);
                        Math::Vec3 flat = targetStep - position;
                        flat.y = 0.0f; // el nodo puede estar un bloque más arriba (escalón)
                        if (flat.length() < 0.6f) {
                            pathIndex++;
                        }
                    }

                    Math::Vec3 moveDir = targetStep - position;
                    moveDir.y = 0.0f;
                    if (moveDir.length() > 0.01f) {
                        moveDir = moveDir.normalized();
                        velocity.x = moveDir.x * movementSpeed;
                        velocity.z = moveDir.z * movementSpeed;
                        yaw = Math::degrees(std::atan2(moveDir.x, moveDir.z));
                    }
                }
            } else {
                wander(0.6f); // sin objetivo deambulan, como en el juego original
            }
            break;
        }

        // Las criaturas de las cámaras de desafío atacan a distancia como el esqueleto
        case MobType::SwampSkeleton:
        case MobType::Whirl:
        case MobType::Skeleton: {
            if (targetPlayer) {
                targetEntityId = player.id;
                aiState = AIState::Target;

                // Mirar al jugador
                yaw = Math::degrees(std::atan2(diff.x, diff.z));

                // Mantener distancia óptima (8 - 14 bloques)
                if (dist < 8.0f) {
                    // Retroceder
                    Math::Vec3 retreatDir = (position - playerPos);
                    retreatDir.y = 0.0f;
                    if (retreatDir.length() > 0.01f) {
                        retreatDir = retreatDir.normalized();
                        velocity.x = retreatDir.x * movementSpeed;
                        velocity.z = retreatDir.z * movementSpeed;
                    }
                } else if (dist > 14.0f) {
                    // Acercarse
                    Math::Vec3 advDir = (playerPos - position);
                    advDir.y = 0.0f;
                    if (advDir.length() > 0.01f) {
                        advDir = advDir.normalized();
                        velocity.x = advDir.x * movementSpeed;
                        velocity.z = advDir.z * movementSpeed;
                    }
                } else {
                    velocity.x = 0.0f;
                    velocity.z = 0.0f;
                }

                // Disparar proyectil (flecha)
                if (attackCooldown == 0 && dist <= 16.0f) {
                    attackCooldown = 40; // Disparo cada 2 segundos
                    aiState = AIState::Attack;

                    Math::Vec3 spawnPos = position + Math::Vec3(0.0f, height * 0.8f, 0.0f);
                    Math::Vec3 aimDir = (player.getEyePosition() - spawnPos);
                    float aimLen = aimDir.length();
                    if (aimLen > 0.01f) {
                        aimDir = aimDir * (1.0f / aimLen);
                        // Ligera compensación de parábola hacia arriba
                        aimDir.y += 0.08f;
                        aimDir = aimDir.normalized();
                        outProjectiles.emplace_back(nextEntityId++, id, spawnPos, aimDir * 18.0f, 4.0f);
                    }
                }
            } else {
                wander(0.6f);
            }
            break;
        }

        case MobType::Lurker: {
            if (targetPlayer) {
                targetEntityId = player.id;
                // Si está a <= 3.0 metros, comenzar a sisear/inflarse
                if (dist <= 3.0f) {
                    aiState = AIState::Ignited;
                    isIgnited = true;
                    velocity.x = 0.0f;
                    velocity.z = 0.0f;
                    fuseTicks++;

                    if (fuseTicks >= 30) { // 1.5 segundos
                        // Detonar explosión
                        isDead = true;
                        isAlive = false;
                        hasExploded = true;
                        explosionRadius = 3.0f;
                    }
                } else {
                    // Si el jugador se aleja a > 5.0 metros, cancelar fusible
                    if (isIgnited && dist > 5.0f) {
                        isIgnited = false;
                        fuseTicks = 0;
                    }

                    aiState = AIState::Chase;
                    Math::Vec3 moveDir = playerPos - position;
                    moveDir.y = 0.0f;
                    if (moveDir.length() > 0.01f) {
                        moveDir = moveDir.normalized();
                        velocity.x = moveDir.x * movementSpeed;
                        velocity.z = moveDir.z * movementSpeed;
                        yaw = Math::degrees(std::atan2(moveDir.x, moveDir.z));
                    }
                }
            } else {
                isIgnited = false;
                fuseTicks = 0;
                wander(0.6f);
            }
            break;
        }
    }
}

void MobEntity::wander(float speedScale) {
    // Alterna pausas y paseos hacia un punto aleatorio cercano
    if (wanderWaitTicks > 0) {
        wanderWaitTicks--;
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        aiState = AIState::Idle;
        return;
    }
    aiState = AIState::Wander;
    Math::Vec3 toTarget = wanderTarget - position;
    toTarget.y = 0.0f;
    bool stuck = aiTicks % 20 == 0 && std::abs(velocity.x) + std::abs(velocity.z) < 0.05f && aiTicks > 20;
    if (toTarget.length() < 1.0f || aiTicks % 100 == 0 || stuck || (wanderTarget.x == 0.0f && wanderTarget.z == 0.0f)) {
        if (toTarget.length() < 1.0f || stuck) {
            wanderWaitTicks = 20 + static_cast<uint32_t>(rand() % 60); // pausa al llegar o atascarse
        }
        float rad = Math::radians(static_cast<float>(rand() % 360));
        float dist = 3.0f + static_cast<float>(rand() % 6);
        wanderTarget = position + Math::Vec3(std::cos(rad) * dist, 0.0f, std::sin(rad) * dist);
        toTarget = wanderTarget - position;
        toTarget.y = 0.0f;
    }
    if (toTarget.length() > 0.1f) {
        Math::Vec3 dir = toTarget.normalized();
        velocity.x = dir.x * movementSpeed * speedScale;
        velocity.z = dir.z * movementSpeed * speedScale;
        yaw = Math::degrees(std::atan2(dir.x, dir.z));
    }
}

void MobEntity::tickPhysics(World& world, float dt) {
    if (isDead) return;

    prevPosition = position;
    prevYaw = yaw;
    prevPitch = pitch;

    // Gravedad
    velocity.y -= 26.0f * dt;

    // Resolver colisiones con el mundo
    resolveCollisions(world, velocity, dt);
}

void MobEntity::resolveCollisions(World& world, Math::Vec3& vel, float dt) {
    const float halfW = width * 0.5f;
    constexpr float EPS = 1e-4f;
    const auto& reg = BlockRegistry::global();
    auto isSolidAt = [&](int x, int y, int z) -> bool {
        int cx = (x >= 0) ? (x / 16) : ((x - 15) / 16);
        int cz = (z >= 0) ? (z / 16) : ((z - 15) / 16);
        if (!world.isChunkLoaded(cx, cz)) return true;
        return reg.get(world.getBlockState(x, y, z)).isSolid;
    };
    // Bloques que la caja ocupa realmente (las caras compartidas no cuentan como choque)
    auto overlaps = [&](const Math::Vec3& p, auto&& fn) {
        int x0 = static_cast<int>(std::floor(p.x - halfW + EPS)), x1 = static_cast<int>(std::floor(p.x + halfW - EPS));
        int y0 = static_cast<int>(std::floor(p.y + EPS)), y1 = static_cast<int>(std::floor(p.y + height - EPS));
        int z0 = static_cast<int>(std::floor(p.z - halfW + EPS)), z1 = static_cast<int>(std::floor(p.z + halfW - EPS));
        for (int y = y0; y <= y1; ++y)
            for (int z = z0; z <= z1; ++z)
                for (int x = x0; x <= x1; ++x)
                    if (isSolidAt(x, y, z)) fn(x, y, z);
    };
    auto wasInside = [&](const Math::Vec3& p, int x, int y, int z) {
        return static_cast<float>(x + 1) > p.x - halfW + EPS &&
               static_cast<float>(x)<p.x + halfW - EPS&& static_cast<float>(y + 1)> p.y + EPS &&
               static_cast<float>(y)<p.y + height - EPS&& static_cast<float>(z + 1)> p.z - halfW + EPS &&
               static_cast<float>(z) < p.z + halfW - EPS;
    };

    const float maxSubDt = 0.025f;
    int numSteps = std::max(1, static_cast<int>(std::ceil(dt / maxSubDt)));
    float subDt = dt / static_cast<float>(numSteps);
    bool grounded = false;
    bool blockedHorizontally = false;

    for (int step = 0; step < numSteps; ++step) {
        if (vel.y != 0.0f) {
            Math::Vec3 before = position;
            position.y += vel.y * subDt;
            bool hit = false;
            float limit = vel.y < 0.0f ? -1e30f : 1e30f;
            overlaps(position, [&](int x, int y, int z) {
                if (wasInside(before, x, y, z)) return;
                hit = true;
                limit =
                    vel.y < 0.0f ? std::max(limit, static_cast<float>(y + 1)) : std::min(limit, static_cast<float>(y));
            });
            if (hit) {
                if (vel.y < 0.0f) {
                    position.y = limit;
                    grounded = true;
                } else {
                    position.y = limit - height;
                }
                vel.y = 0.0f;
            }
        }
        auto moveAxis = [&](int axis, float& v) {
            if (v == 0.0f) return;
            Math::Vec3 before = position;
            float& coord = axis == 0 ? position.x : position.z;
            coord += v * subDt;
            bool hit = false;
            float limit = v < 0.0f ? -1e30f : 1e30f;
            overlaps(position, [&](int x, int y, int z) {
                if (wasInside(before, x, y, z)) return;
                int c = axis == 0 ? x : z;
                hit = true;
                limit = v < 0.0f ? std::max(limit, static_cast<float>(c + 1)) : std::min(limit, static_cast<float>(c));
            });
            if (hit) {
                coord = v < 0.0f ? limit + halfW : limit - halfW;
                blockedHorizontally = true;
                // La velocidad horizontal se conserva para que el salto lo supere en el siguiente tick
            }
        };
        moveAxis(0, vel.x);
        moveAxis(2, vel.z);
    }

    if (!grounded) {
        Math::Vec3 probe = position;
        probe.y -= 0.02f;
        overlaps(probe, [&](int, int y, int) {
            if (static_cast<float>(y + 1) <= position.y + EPS) grounded = true;
        });
    }
    isGrounded = grounded;

    // Salto para superar escalones de un bloque cuando algo le corta el paso estando en el suelo
    if (isGrounded && blockedHorizontally && (std::abs(vel.x) > 0.1f || std::abs(vel.z) > 0.1f)) {
        Math::Vec3 dir(vel.x, 0.0f, vel.z);
        dir = dir.normalized();
        int ax = static_cast<int>(std::floor(position.x + dir.x * (halfW + 0.3f)));
        int az = static_cast<int>(std::floor(position.z + dir.z * (halfW + 0.3f)));
        int fy = static_cast<int>(std::floor(position.y + EPS));
        int clearance = static_cast<int>(std::ceil(height));
        bool spaceAbove = true;
        for (int dy = 1; dy <= clearance; ++dy) {
            if (isSolidAt(ax, fy + dy, az)) spaceAbove = false;
        }
        if (isSolidAt(ax, fy, az) && spaceAbove) {
            // Con la gravedad del siguiente tick, 9 m/s eleva ~1,15 bloques: supera el escalón con margen
            vel.y = 9.0f;
            isGrounded = false;
        }
    }
}

} // namespace OpenCraft
