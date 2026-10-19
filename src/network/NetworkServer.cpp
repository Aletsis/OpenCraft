#include "network/NetworkServer.hpp"
#include "network/Socket.hpp"
#include <cmath>
#include "network/PacketValidator.hpp"
#include "core/Log.hpp"

#include <cstring>
#include <sstream>

namespace OpenCraft::Net {

NetworkServer::NetworkServer() = default;

NetworkServer::~NetworkServer() { stop(); }

bool NetworkServer::start(uint16_t port, const std::string& serverName, uint32_t maxPlayers) {
    stop();

    m_socket = openUdpSocket();
    if (!isValidSocket(m_socket)) {
        Log::error("NetworkServer: Fallo al crear socket UDP");
        return false;
    }

    setSocketOption(m_socket, SOL_SOCKET, SO_REUSEADDR, 1);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (!bindSocket(m_socket, addr)) {
        Log::error("NetworkServer: Fallo al vincular puerto {}", port);
        closeSocket(m_socket);
        m_socket = INVALID_SOCKET_HANDLE;
        return false;
    }

    m_port = port;
    m_serverName = serverName;
    m_maxPlayers = maxPlayers;
    m_running = true;
    m_nextSessionId = 1;
    m_nextPlayerEntityId = 10;
    m_clients.clear();

    Log::info("NetworkServer iniciado en puerto {} ('{}', máx: {} jugadores)", port, serverName, maxPlayers);
    return true;
}

void NetworkServer::stop() {
    if (!m_running) return;

    // Notificar desconexión a todos los clientes
    ByteWriter w;
    serializeHeader(w, PacketType::PlayerDisconnect, ++m_sequence);
    w.writeString("Servidor apagándose");
    for (const auto& [id, client] : m_clients) {
        sendRaw(w.data(), w.size(), client.address);
    }

    if (isValidSocket(m_socket)) {
        closeSocket(m_socket);
        m_socket = INVALID_SOCKET_HANDLE;
    }

    m_clients.clear();
    m_running = false;
    Log::info("NetworkServer detenido.");
}

bool NetworkServer::isWhitelisted(const std::string& name) const {
    if (!m_whitelistEnabled) return true;
    return m_whitelist.find(name) != m_whitelist.end();
}

ServerClientSession* NetworkServer::findClientByAddr(const sockaddr_in& addr) {
    for (auto& [id, client] : m_clients) {
        if (client.address.sin_addr.s_addr == addr.sin_addr.s_addr && client.address.sin_port == addr.sin_port) {
            return &client;
        }
    }
    return nullptr;
}

ServerClientSession* NetworkServer::findClientByName(const std::string& name) {
    for (auto& [id, client] : m_clients) {
        if (client.name == name) {
            return &client;
        }
    }
    return nullptr;
}

void NetworkServer::sendRaw(const void* data, size_t size, const sockaddr_in& dest) {
    if (!isValidSocket(m_socket) || !data || size == 0) return;
    sendTo(m_socket, data, size, dest);
}

bool NetworkServer::kickPlayer(const std::string& name, const std::string& reason) {
    ServerClientSession* client = findClientByName(name);
    if (!client) return false;

    ByteWriter w;
    serializeHeader(w, PacketType::PlayerDisconnect, ++m_sequence);
    w.writeString(reason);
    sendRaw(w.data(), w.size(), client->address);

    Log::info("Jugador '{}' expulsado: {}", name, reason);
    broadcastChat("Servidor", "Jugador " + name + " fue expulsado (" + reason + ")", true);
    m_clients.erase(client->sessionId);
    return true;
}

void NetworkServer::broadcastChat(const std::string& senderName, const std::string& text, bool isSystem) {
    ChatMessageData chat;
    chat.senderId = 0;
    chat.senderName = senderName;
    chat.text = text;
    chat.isSystem = isSystem;

    ByteWriter w;
    serializeChatMessage(w, ++m_sequence, chat);

    for (auto& [id, client] : m_clients) {
        sendRaw(w.data(), w.size(), client.address);
    }
}

void NetworkServer::broadcastBlockChange(int x, int y, int z, BlockStateId state, EntityId causeId) {
    (void)causeId;
    const auto& props = BlockRegistry::global().get(state);

    PacketBlockModify bm{};
    bm.header.type = PacketType::BlockModify;
    bm.header.sequence = ++m_sequence;
    bm.x = x;
    bm.y = y;
    bm.z = z;
    bm.blockID = static_cast<uint8_t>(props.legacyId);
    bm.action = (state == AIR_BLOCK_STATE) ? 0 : 1;

    for (auto& [id, client] : m_clients) {
        sendRaw(&bm, sizeof(bm), client.address);
    }
}

void NetworkServer::broadcastEntitySpawn(const EntitySpawnData& sp) {
    ByteWriter w;
    serializeEntitySpawn(w, ++m_sequence, sp);
    for (auto& [id, client] : m_clients) {
        sendRaw(w.data(), w.size(), client.address);
    }
}

void NetworkServer::broadcastEntityDestroy(uint32_t entityId) {
    ByteWriter w;
    serializeHeader(w, PacketType::EntityDestroy, ++m_sequence);
    w.writeUint32(entityId);
    for (auto& [id, client] : m_clients) {
        sendRaw(w.data(), w.size(), client.address);
    }
}

void NetworkServer::executeAdminCommand(ServerClientSession* client, const std::string& cmd, WorldSession& session) {
    std::istringstream iss(cmd);
    std::string command;
    iss >> command;

    if (command == "/help") {
        ChatMessageData chat{
            0, "Sistema", "Comandos disponibles: /help, /kick <jugador>, /op <jugador>, /whitelist <on|off|add|remove>",
            true};
        ByteWriter w;
        serializeChatMessage(w, ++m_sequence, chat);
        sendRaw(w.data(), w.size(), client->address);
    } else if (command == "/kick") {
        if (!client->isOperator) {
            ChatMessageData chat{0, "Sistema", "Permisos insuficientes para ejecutar /kick", true};
            ByteWriter w;
            serializeChatMessage(w, ++m_sequence, chat);
            sendRaw(w.data(), w.size(), client->address);
            return;
        }
        std::string target, reason;
        iss >> target;
        std::getline(iss, reason);
        if (reason.empty()) reason = "Expulsado por operador";
        kickPlayer(target, reason);
    } else if (command == "/op") {
        if (!client->isOperator) return;
        std::string target;
        iss >> target;
        addOp(target);
        ServerClientSession* targetClient = findClientByName(target);
        if (targetClient) targetClient->isOperator = true;
        broadcastChat("Sistema", target + " ahora es operador", true);
    } else if (command == "/whitelist") {
        if (!client->isOperator) return;
        std::string sub, target;
        iss >> sub;
        if (sub == "on") {
            setWhitelistEnabled(true);
            broadcastChat("Sistema", "Lista blanca activada", true);
        } else if (sub == "off") {
            setWhitelistEnabled(false);
            broadcastChat("Sistema", "Lista blanca desactivada", true);
        } else if (sub == "add") {
            iss >> target;
            addWhitelist(target);
            broadcastChat("Sistema", target + " añadido a la lista blanca", true);
        } else if (sub == "remove") {
            iss >> target;
            removeWhitelist(target);
            broadcastChat("Sistema", target + " eliminado de la lista blanca", true);
        }
    } else if (command == "/gamemode") {
        uint32_t gm = 0;
        iss >> gm;
        session.executeCommand(SetGameModeCommand{client->entityId, gm});
        broadcastChat("Sistema", "Modo de juego actualizado", true);
    }
}

void NetworkServer::handleIncomingPacket(const uint8_t* buffer, size_t size, const sockaddr_in& senderAddr,
                                         WorldSession& session, uint64_t nowMs) {
    auto vres = PacketValidator::validate(buffer, size);
    if (vres != ValidationResult::Valid) {
        Log::warn("NetworkServer: Paquete inválido rechazado (código: {})", static_cast<int>(vres));
        return;
    }

    // Identificar formato OCNT (Fase 09)
    if (size >= 9) {
        uint32_t magic = 0;
        std::memcpy(&magic, buffer, sizeof(magic));
        if (magic == NETWORK_MAGIC) {
            ByteReader r(buffer, size);
            PacketType type = PacketType::Handshake;
            uint32_t seq = 0;
            if (!deserializeHeader(r, type, seq)) return;

            if (type == PacketType::Handshake) {
                std::string playerName;
                uint32_t clientVersion = 0;
                if (!deserializeHandshake(r, playerName, clientVersion)) return;

                char cleanName[32];
                if (!PacketValidator::sanitizePlayerName(playerName.c_str(), playerName.size(), cleanName,
                                                         sizeof(cleanName))) {
                    ByteWriter w;
                    serializeHandshakeReject(w, ++m_sequence, {RejectReason::InvalidName, "Nombre no válido"});
                    sendRaw(w.data(), w.size(), senderAddr);
                    return;
                }

                if (clientVersion != PROTOCOL_VERSION) {
                    ByteWriter w;
                    serializeHandshakeReject(
                        w, ++m_sequence,
                        {RejectReason::IncompatibleVersion,
                         "Versión incompatible de protocolo (servidor v" + std::to_string(PROTOCOL_VERSION) + ")"});
                    sendRaw(w.data(), w.size(), senderAddr);
                    Log::warn("Conexión rechazada: versión de cliente v{} incompatible con servidor v{}", clientVersion,
                              PROTOCOL_VERSION);
                    return;
                }

                if (m_clients.size() >= m_maxPlayers) {
                    ByteWriter w;
                    serializeHandshakeReject(w, ++m_sequence, {RejectReason::ServerFull, "Servidor lleno"});
                    sendRaw(w.data(), w.size(), senderAddr);
                    return;
                }

                if (!isWhitelisted(cleanName)) {
                    ByteWriter w;
                    serializeHandshakeReject(w, ++m_sequence,
                                             {RejectReason::BannedOrNotWhitelisted, "No estás en la lista blanca"});
                    sendRaw(w.data(), w.size(), senderAddr);
                    return;
                }

                uint32_t sid = m_nextSessionId++;

                // Asignar o recuperar EntityId estable por nombre de jugador
                uint32_t eid = 0;
                auto nameIt = m_playerNameToEntityId.find(cleanName);
                if (nameIt != m_playerNameToEntityId.end()) {
                    eid = nameIt->second;
                } else {
                    EntityId sId = session.getPlayerIdByName(cleanName);
                    if (sId != INVALID_ENTITY_ID) {
                        eid = sId;
                    } else {
                        eid = m_nextPlayerEntityId++;
                    }
                    m_playerNameToEntityId[cleanName] = eid;
                }
                if (eid >= m_nextPlayerEntityId) {
                    m_nextPlayerEntityId = eid + 1;
                }

                ServerClientSession newClient;
                newClient.sessionId = sid;
                newClient.entityId = eid;
                newClient.name = cleanName;
                newClient.address = senderAddr;
                newClient.lastHeartbeatMs = nowMs;
                newClient.isConnected = true;
                newClient.isOperator = isOp(cleanName);

                Math::Vec3 spawnPos{8.0f, 68.0f, 8.0f};
                PlayerEntity* existingP = session.getPlayerById(eid);
                if (existingP) {
                    spawnPos = existingP->position;
                    session.addPlayer(eid, cleanName, spawnPos);
                } else {
                    session.addPlayer(eid, cleanName, spawnPos);
                }
                m_clients[sid] = std::move(newClient);

                // Enviar confirmación HandshakeAck
                HandshakeAckData ack;
                ack.protocolVersion = PROTOCOL_VERSION;
                ack.assignedPlayerId = eid;
                ack.sessionId = sid;
                ack.spawnX = spawnPos.x;
                ack.spawnY = spawnPos.y;
                ack.spawnZ = spawnPos.z;
                ack.worldSeed = session.getWorld().getSeed();
                ack.serverTick = session.getClock().getTickCount();
                ack.isOperator = isOp(cleanName);

                ByteWriter ackW;
                serializeHandshakeAck(ackW, ++m_sequence, ack);
                sendRaw(ackW.data(), ackW.size(), senderAddr);

                // Replicar entidades existentes al nuevo cliente
                for (const auto& mob : session.getMobs()) {
                    if (mob.isAlive && !mob.isDead) {
                        EntitySpawnData esp;
                        esp.entityId = mob.id;
                        esp.entityType = 1; // Mob
                        esp.x = mob.position.x;
                        esp.y = mob.position.y;
                        esp.z = mob.position.z;
                        esp.vx = mob.velocity.x;
                        esp.vy = mob.velocity.y;
                        esp.vz = mob.velocity.z;
                        esp.yaw = mob.yaw;
                        esp.pitch = mob.pitch;
                        esp.subType = static_cast<uint8_t>(mob.mobType);
                        esp.health = mob.health;
                        ByteWriter spW;
                        serializeEntitySpawn(spW, ++m_sequence, esp);
                        sendRaw(spW.data(), spW.size(), senderAddr);
                    }
                }

                for (const auto& item : session.getItemEntities()) {
                    if (!item.isDead) {
                        EntitySpawnData esp;
                        esp.entityId = item.id;
                        esp.entityType = 2; // Item
                        esp.x = item.position.x;
                        esp.y = item.position.y;
                        esp.z = item.position.z;
                        esp.vx = item.velocity.x;
                        esp.vy = item.velocity.y;
                        esp.vz = item.velocity.z;
                        esp.itemId = item.stack.itemId;
                        esp.count = item.stack.count;
                        ByteWriter spW;
                        serializeEntitySpawn(spW, ++m_sequence, esp);
                        sendRaw(spW.data(), spW.size(), senderAddr);
                    }
                }

                // Replicar los otros jugadores al nuevo cliente
                for (const auto& [otherSid, otherClient] : m_clients) {
                    if (otherSid != sid) {
                        const PlayerEntity* otherP = session.getPlayerById(otherClient.entityId);
                        if (otherP) {
                            EntitySpawnData esp;
                            esp.entityId = otherClient.entityId;
                            esp.entityType = 0; // Player
                            esp.x = otherP->position.x;
                            esp.y = otherP->position.y;
                            esp.z = otherP->position.z;
                            esp.yaw = otherP->yaw;
                            esp.pitch = otherP->pitch;
                            esp.health = otherP->health;
                            ByteWriter spW;
                            serializeEntitySpawn(spW, ++m_sequence, esp);
                            sendRaw(spW.data(), spW.size(), senderAddr);
                        }
                    }
                }

                // Notificar a todos los demás sobre el nuevo jugador
                EntitySpawnData espNew;
                espNew.entityId = eid;
                espNew.entityType = 0; // Player
                espNew.x = spawnPos.x;
                espNew.y = spawnPos.y;
                espNew.z = spawnPos.z;
                ByteWriter spW;
                serializeEntitySpawn(spW, ++m_sequence, espNew);
                for (const auto& [otherSid, otherClient] : m_clients) {
                    if (otherSid != sid) {
                        sendRaw(spW.data(), spW.size(), otherClient.address);
                    }
                }

                broadcastChat("Servidor", std::string(cleanName) + " se ha unido a la partida", true);
                Log::info("Cliente '{}' conectado con éxito (SessionId: {}, EntityId: {})", std::string_view(cleanName),
                          sid, eid);
                return;
            }

            ServerClientSession* client = findClientByAddr(senderAddr);
            if (!client) return;

            client->lastHeartbeatMs = nowMs;

            if (type == PacketType::Ping) {
                uint64_t ts = 0;
                deserializePing(r, ts);
                ByteWriter pongW;
                serializePong(pongW, ++m_sequence, ts);
                sendRaw(pongW.data(), pongW.size(), senderAddr);
            } else if (type == PacketType::Pong) {
                // Heartbeat recibido
            } else if (type == PacketType::ReliableAck) {
                uint32_t ackSeq = 0;
                if (deserializeReliableAck(r, ackSeq)) {
                    client->channel.onAckReceived(ackSeq);
                }
            } else if (type == PacketType::PlayerAction) {
                PlayerActionData act;
                if (deserializePlayerAction(r, act)) {
                    // Confirmar recepción fiable
                    ByteWriter ackW;
                    serializeReliableAck(ackW, ++m_sequence, seq);
                    sendRaw(ackW.data(), ackW.size(), senderAddr);

                    // Mapear a comando autoritativo
                    if (act.action == PlayerActionType::BreakBlock) {
                        CommandResult res = session.executeCommand(
                            BreakBlockCommand{.entityId = client->entityId, .x = act.x, .y = act.y, .z = act.z});
                        if (res == CommandResult::Success) {
                            broadcastBlockChange(act.x, act.y, act.z, AIR_BLOCK_STATE, client->entityId);
                        }
                    } else if (act.action == PlayerActionType::PlaceBlock) {
                        CommandResult res = session.executeCommand(PlaceBlockCommand{.entityId = client->entityId,
                                                                                     .x = act.x,
                                                                                     .y = act.y,
                                                                                     .z = act.z,
                                                                                     .blockState = act.blockState});
                        if (res == CommandResult::Success) {
                            broadcastBlockChange(act.x, act.y, act.z, act.blockState, client->entityId);
                        }
                    } else if (act.action == PlayerActionType::AttackEntity) {
                        session.executeCommand(
                            AttackEntityCommand{.entityId = client->entityId, .targetEntityId = act.targetEntityId});
                    } else if (act.action == PlayerActionType::UseShield) {
                        session.executeCommand(
                            UseShieldCommand{.entityId = client->entityId, .isBlocking = (act.flag != 0)});
                    } else if (act.action == PlayerActionType::SelectSlot) {
                        session.executeCommand(
                            SelectSlotCommand{.entityId = client->entityId, .slot = static_cast<size_t>(act.x)});
                    } else if (act.action == PlayerActionType::DropItem) {
                        session.executeCommand(DropItemCommand{.entityId = client->entityId,
                                                               .fromCursor = (act.flag != 0),
                                                               .slot = static_cast<size_t>(act.x),
                                                               .count = act.count});
                    } else if (act.action == PlayerActionType::Respawn) {
                        session.executeCommand(RespawnCommand{client->entityId});
                    }
                }
            } else if (type == PacketType::InventoryTransaction) {
                InventoryTransactionData tx;
                if (deserializeInventoryTransaction(r, tx)) {
                    ByteWriter ackW;
                    serializeReliableAck(ackW, ++m_sequence, seq);
                    sendRaw(ackW.data(), ackW.size(), senderAddr);

                    CommandResult res = session.executeCommand(InventoryTransactionCommand{
                        .entityId = client->entityId,
                        .action = static_cast<InventoryAction>(tx.action),
                        .fromContainer = static_cast<InventoryContainerType>(tx.fromContainer),
                        .fromSlot = tx.fromSlot,
                        .toContainer = static_cast<InventoryContainerType>(tx.toContainer),
                        .toSlot = tx.toSlot,
                        .amount = tx.amount,
                        .expectedRevision = tx.expectedRevision});

                    uint32_t newRev = 0;
                    auto chest = session.getWorld().getChestBlockEntity(session.getOpenContainerPos().x,
                                                                        session.getOpenContainerPos().y,
                                                                        session.getOpenContainerPos().z);
                    if (chest) newRev = chest->getRevision();

                    TransactionResultData trData{.entityId = client->entityId,
                                                 .success = (res == CommandResult::Success),
                                                 .errorCode = static_cast<uint8_t>(res),
                                                 .newRevision = newRev};
                    ByteWriter trW;
                    serializeTransactionResult(trW, ++m_sequence, trData);
                    sendRaw(trW.data(), trW.size(), senderAddr);
                }
            } else if (type == PacketType::ChatMessage) {
                ChatMessageData chat;
                if (deserializeChatMessage(r, chat)) {
                    if (!chat.text.empty() && chat.text[0] == '/') {
                        executeAdminCommand(client, chat.text, session);
                    } else {
                        broadcastChat(client->name, chat.text, false);
                    }
                }
            } else if (type == PacketType::PlayerDisconnect) {
                Log::info("Cliente '{}' desconectado voluntariamente.", client->name);
                broadcastChat("Servidor", client->name + " ha abandonado la partida", true);
                broadcastEntityDestroy(client->entityId);
                session.removePlayer(client->entityId);
                m_clients.erase(client->sessionId);
            }
            return;
        }
    }

    // Ruta de compatibilidad hacia atrás para paquetes de structs planos (F01)
    const auto* header = reinterpret_cast<const PacketHeader*>(buffer);
    if (header->type == PacketType::PlayerUpdate) {
        if (size < sizeof(PacketPlayerUpdate)) return; // datagrama truncado: no leer fuera del búfer
        const auto* pu = reinterpret_cast<const PacketPlayerUpdate*>(buffer);
        ServerClientSession* client = findClientByAddr(senderAddr);
        if (client) {
            client->lastHeartbeatMs = nowMs;
            PlayerEntity* p = session.getPlayerById(client->entityId);
            bool accepted = false;
            if (p) {
                // Autoridad del servidor: se rechaza un desplazamiento mayor del que permite la
                // física (vuelo/caída rápidos incluidos) en el tiempo transcurrido desde el último
                Math::Vec3 target{pu->posX, pu->posY, pu->posZ};
                bool finite = std::isfinite(target.x) && std::isfinite(target.y) && std::isfinite(target.z);
                uint64_t elapsedMs = client->lastMoveMs == 0 ? 50 : std::max<uint64_t>(nowMs - client->lastMoveMs, 1);
                float maxSpeed = (p->gameMode == 1 || p->gameMode == 3) ? 30.0f : 20.0f; // bloques/s
                float allowed = maxSpeed * static_cast<float>(std::min<uint64_t>(elapsedMs, 1000)) / 1000.0f + 1.0f;
                Math::Vec3 d = target - p->position;
                if (finite && d.dot(d) <= allowed * allowed && target.y > static_cast<float>(Chunk::MIN_Y) - 64.0f &&
                    target.y < static_cast<float>(Chunk::MAX_Y) + 64.0f) {
                    p->position = target;
                    p->yaw = pu->yaw;
                    p->pitch = pu->pitch;
                    client->lastMoveMs = nowMs;
                    accepted = true;
                } else {
                    ++client->rejectedMoves;
                    Log::warn("Movimiento imposible rechazado para '{}' ({:.1f} bloques en {} ms)", client->name,
                              std::sqrt(d.dot(d)), elapsedMs);
                }
            }
            if (!accepted) return;

            // Re-enviar telemetría a los demás clientes
            PacketPlayerUpdate fwd = *pu;
            fwd.playerID = client->entityId;
            for (auto& [id, c] : m_clients) {
                if (c.sessionId != client->sessionId) {
                    sendRaw(&fwd, sizeof(fwd), c.address);
                }
            }
        }
    }
}

void NetworkServer::update(WorldSession& session, uint64_t currentSimTimeMs) {
    if (!m_running || !isValidSocket(m_socket)) return;
    m_activeSession = &session;

    // 1. Drenar socket UDP de entrada
    uint8_t buffer[2048];
    sockaddr_in senderAddr{};

    while (true) {
        long long bytes = recvFrom(m_socket, buffer, sizeof(buffer), senderAddr);
        if (bytes <= 0) break;
        handleIncomingPacket(buffer, static_cast<size_t>(bytes), senderAddr, session, currentSimTimeMs);
    }

    // 2. Control de timeouts e inactividad
    for (auto it = m_clients.begin(); it != m_clients.end();) {
        if (currentSimTimeMs > it->second.lastHeartbeatMs + TIMEOUT_MS) {
            Log::info("Cliente '{}' desconectado por timeout (inactivo > 10s)", it->second.name);
            broadcastChat("Servidor", it->second.name + " perdió la conexión (timeout)", true);
            broadcastEntityDestroy(it->second.entityId);
            session.removePlayer(it->second.entityId);
            it = m_clients.erase(it);
        } else {
            // 3. Comprobar retransmisiones del canal fiable
            auto retransmissions = it->second.channel.checkRetransmissions(currentSimTimeMs);
            for (const auto& pktData : retransmissions) {
                sendRaw(pktData.data(), pktData.size(), it->second.address);
            }
            ++it;
        }
    }

    // 4. Enviar pings periódicos de heartbeat a clientes cada 2 segundos
    static uint64_t lastPingTime = 0;
    if (currentSimTimeMs >= lastPingTime + 2000) {
        lastPingTime = currentSimTimeMs;
        ByteWriter pw;
        serializePing(pw, ++m_sequence, currentSimTimeMs);
        for (auto& [id, client] : m_clients) {
            sendRaw(pw.data(), pw.size(), client.address);
        }
    }
}

void NetworkServer::savePlayer(WorldSession& session, WorldStorage& storage, uint32_t entityId) {
    PlayerEntity* p = session.getPlayerById(entityId);
    if (!p) {
        Log::warn("NetworkServer::savePlayer: no se encontró jugador con id {}", entityId);
        return;
    }

    auto& reg = BlockRegistry::global();
    auto& itemReg = ItemRegistry::global();

    PlayerSaveData data;
    data.schemaVersion = CURRENT_SCHEMA_VERSION;
    data.entityId = entityId;
    data.playerName = session.getPlayerName(entityId);
    data.position = p->position;
    data.velocity = p->velocity;
    data.yaw = p->yaw;
    data.pitch = p->pitch;
    data.isGrounded = p->isGrounded;
    data.health = p->health;
    data.hunger = p->hunger;
    data.saturation = p->saturation;
    data.exhaustion = p->exhaustion;
    data.air = p->air;
    data.gameMode = p->gameMode;
    data.spawnPoint = p->spawnPoint;
    data.hasCustomSpawnPoint = p->hasCustomSpawnPoint;
    data.isAlive = p->isAlive;
    data.selectedSlot = static_cast<uint32_t>(p->selectedSlot);

    data.hotbarNames.clear();
    for (BlockStateId slotState : p->hotbar) {
        data.hotbarNames.push_back(reg.getNamespacedId(slotState));
    }

    data.inventorySlots.clear();
    data.inventorySlots.reserve(p->inventory.getSize());
    for (size_t i = 0; i < p->inventory.getSize(); ++i) {
        const auto& stack = p->inventory.getSlot(i);
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
        data.inventorySlots.push_back(std::move(isData));
    }

    if (!p->cursorItem.isEmpty()) {
        data.cursorItem = ItemStackSaveData{.itemName = itemReg.get(p->cursorItem.itemId).namespacedId,
                                            .count = p->cursorItem.count,
                                            .durability = p->cursorItem.durability,
                                            .components = p->cursorItem.components};
    } else {
        data.cursorItem = ItemStackSaveData{.itemName = "opencraft:air", .count = 0, .durability = 0, .components = {}};
    }

    if (!p->offhandItem.isEmpty()) {
        data.offhandItem = ItemStackSaveData{.itemName = itemReg.get(p->offhandItem.itemId).namespacedId,
                                             .count = p->offhandItem.count,
                                             .durability = p->offhandItem.durability,
                                             .components = p->offhandItem.components};
    } else {
        data.offhandItem =
            ItemStackSaveData{.itemName = "opencraft:air", .count = 0, .durability = 0, .components = {}};
    }

    data.xpTotal = p->xpTotal;
    data.xpLevel = p->xpLevel;
    data.xpProgress = p->xpProgress;

    data.armorSlots.clear();
    for (const auto& piece : p->armorSlots) {
        if (!piece.isEmpty()) {
            data.armorSlots.push_back(ItemStackSaveData{.itemName = itemReg.get(piece.itemId).namespacedId,
                                                        .count = piece.count,
                                                        .durability = piece.durability,
                                                        .components = piece.components});
        } else {
            data.armorSlots.push_back(
                ItemStackSaveData{.itemName = "opencraft:air", .count = 0, .durability = 0, .components = {}});
        }
    }

    bool ownTx = !storage.isInTransaction();
    if (ownTx) {
        if (storage.beginTransaction() != StorageError::None) return;
    }

    storage.stagePlayer(data, entityId);

    if (ownTx) {
        storage.commitTransaction();
    }
}

void NetworkServer::savePlayer(WorldStorage& storage, uint32_t entityId) {
    if (m_activeSession) {
        savePlayer(*m_activeSession, storage, entityId);
    } else {
        Log::warn("NetworkServer::savePlayer: no hay WorldSession activa registrada");
    }
}

} // namespace OpenCraft::Net
