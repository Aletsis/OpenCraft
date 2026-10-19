#include "network/NetworkClient.hpp"
#include "network/PacketValidator.hpp"
#include "core/Log.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace OpenCraft::Net {

static bool setNonBlockingFd(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

NetworkClient::NetworkClient() = default;

NetworkClient::~NetworkClient() { disconnect(); }

bool NetworkClient::connect(const std::string& hostIp, uint16_t port, const std::string& playerName) {
    disconnect();

    m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) {
        Log::error("NetworkClient: Fallo al crear socket UDP");
        m_state = ClientConnectionState::ConnectionFailed;
        return false;
    }

    setNonBlockingFd(m_socket);

    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, hostIp.c_str(), &m_serverAddr.sin_addr) <= 0) {
        Log::error("NetworkClient: IP inválida: {}", hostIp);
        close(m_socket);
        m_socket = -1;
        m_state = ClientConnectionState::ConnectionFailed;
        return false;
    }

    m_playerName = playerName;
    m_state = ClientConnectionState::Connecting;
    m_disconnectReason.clear();
    m_reliable.reset();

    // Enviar Handshake explícito
    ByteWriter w;
    serializeHandshake(w, ++m_sequence, playerName, PROTOCOL_VERSION);
    sendRaw(w.data(), w.size());

    Log::info("NetworkClient: Conectando a {}:{} como '{}'...", hostIp, port, playerName);
    return true;
}

void NetworkClient::disconnect() {
    if (m_socket >= 0) {
        if (m_state == ClientConnectionState::Connected) {
            ByteWriter w;
            serializeHeader(w, PacketType::PlayerDisconnect, ++m_sequence);
            sendRaw(w.data(), w.size());
        }
        close(m_socket);
        m_socket = -1;
    }
    m_state = ClientConnectionState::Disconnected;
}

void NetworkClient::sendRaw(const void* data, size_t size) {
    if (m_socket < 0 || !data || size == 0) return;
    sendto(m_socket, data, size, 0, reinterpret_cast<const sockaddr*>(&m_serverAddr), sizeof(m_serverAddr));
}

void NetworkClient::sendAction(const PlayerActionData& action, uint64_t nowMs) {
    if (m_state != ClientConnectionState::Connected) return;

    ByteWriter w;
    serializePlayerAction(w, 0, action);

    std::vector<uint8_t> payload;
    uint32_t seq = m_reliable.prepareReliablePacket(payload, w.data(), w.size(), nowMs);

    // Reescribir secuencia en la cabecera del payload
    if (payload.size() >= 9) {
        payload[5] = static_cast<uint8_t>(seq & 0xFF);
        payload[6] = static_cast<uint8_t>((seq >> 8) & 0xFF);
        payload[7] = static_cast<uint8_t>((seq >> 16) & 0xFF);
        payload[8] = static_cast<uint8_t>((seq >> 24) & 0xFF);
    }

    sendRaw(payload.data(), payload.size());
}

void NetworkClient::sendInventoryTransaction(const InventoryTransactionData& tx, uint64_t nowMs) {
    if (m_state != ClientConnectionState::Connected) return;

    ByteWriter w;
    serializeInventoryTransaction(w, 0, tx);

    std::vector<uint8_t> payload;
    uint32_t seq = m_reliable.prepareReliablePacket(payload, w.data(), w.size(), nowMs);

    if (payload.size() >= 9) {
        payload[5] = static_cast<uint8_t>(seq & 0xFF);
        payload[6] = static_cast<uint8_t>((seq >> 8) & 0xFF);
        payload[7] = static_cast<uint8_t>((seq >> 16) & 0xFF);
        payload[8] = static_cast<uint8_t>((seq >> 24) & 0xFF);
    }

    sendRaw(payload.data(), payload.size());
}

void NetworkClient::sendChat(const std::string& text) {
    if (m_state != ClientConnectionState::Connected) return;

    ChatMessageData chat;
    chat.senderId = m_assignedPlayerId;
    chat.senderName = m_playerName;
    chat.text = text;
    chat.isSystem = false;

    ByteWriter w;
    serializeChatMessage(w, ++m_sequence, chat);
    sendRaw(w.data(), w.size());
}

void NetworkClient::handleIncomingPacket(const uint8_t* buffer, size_t size, WorldSession& session, uint64_t nowMs) {
    auto vres = PacketValidator::validate(buffer, size);
    if (vres != ValidationResult::Valid) {
        Log::warn("NetworkClient: Paquete recibido no válido (código: {})", static_cast<int>(vres));
        return;
    }

    m_lastServerPacketMs = nowMs;

    if (size >= 9) {
        uint32_t magic = 0;
        std::memcpy(&magic, buffer, sizeof(magic));
        if (magic == NETWORK_MAGIC) {
            ByteReader r(buffer, size);
            PacketType type = PacketType::Handshake;
            uint32_t seq = 0;
            if (!deserializeHeader(r, type, seq)) return;

            if (type == PacketType::HandshakeAck) {
                HandshakeAckData ack;
                if (deserializeHandshakeAck(r, ack)) {
                    m_state = ClientConnectionState::Connected;
                    m_assignedPlayerId = ack.assignedPlayerId;
                    m_sessionId = ack.sessionId;
                    session.getPlayer().id = ack.assignedPlayerId;
                    session.getPlayer().position = {ack.spawnX, ack.spawnY, ack.spawnZ};
                    Log::info("NetworkClient: Conexión aceptada por el servidor (EntityId: {}, SessionId: {})",
                              ack.assignedPlayerId, ack.sessionId);
                }
            } else if (type == PacketType::HandshakeReject) {
                HandshakeRejectData rej;
                if (deserializeHandshakeReject(r, rej)) {
                    m_state = ClientConnectionState::ConnectionFailed;
                    m_disconnectReason = rej.message;
                    Log::warn("NetworkClient: Conexión rechazada: {}", rej.message);
                }
            } else if (type == PacketType::Ping) {
                uint64_t ts = 0;
                deserializePing(r, ts);
                ByteWriter pw;
                serializePong(pw, ++m_sequence, ts);
                sendRaw(pw.data(), pw.size());
            } else if (type == PacketType::Pong) {
                uint64_t ts = 0;
                deserializePong(r, ts);
                if (nowMs >= ts) {
                    m_rttMs = static_cast<uint32_t>(nowMs - ts);
                }
            } else if (type == PacketType::ReliableAck) {
                uint32_t ackSeq = 0;
                if (deserializeReliableAck(r, ackSeq)) {
                    m_reliable.onAckReceived(ackSeq);
                }
            } else if (type == PacketType::ChatMessage) {
                ChatMessageData chat;
                if (deserializeChatMessage(r, chat)) {
                    m_chatHistory.push_back(chat);
                    Log::info("[Chat] <{}> {}", chat.senderName, chat.text);
                }
            } else if (type == PacketType::EntitySpawn) {
                EntitySpawnData sp;
                if (deserializeEntitySpawn(r, sp)) {
                    if (sp.entityType == 0) { // Remote Player
                        if (sp.entityId != m_assignedPlayerId) {
                            session.addPlayer(sp.entityId, "RemotePlayer", {sp.x, sp.y, sp.z});
                        }
                    } else if (sp.entityType == 1) { // Mob
                        session.spawnMob(static_cast<MobType>(sp.subType), {sp.x, sp.y, sp.z});
                    } else if (sp.entityType == 2) { // Item
                        session.spawnDroppedItem(ItemStack(sp.itemId, sp.count), {sp.x, sp.y, sp.z});
                    }
                }
            } else if (type == PacketType::EntityDestroy) {
                uint32_t eid = 0;
                if (r.readUint32(eid)) {
                    session.removePlayer(eid);
                }
            } else if (type == PacketType::EntityUpdate) {
                EntityUpdateData up;
                if (deserializeEntityUpdate(r, up)) {
                    PlayerEntity* rp = session.getPlayerById(up.entityId);
                    if (rp && up.entityId != m_assignedPlayerId) {
                        rp->position = {up.x, up.y, up.z};
                        rp->yaw = up.yaw;
                        rp->pitch = up.pitch;
                        rp->health = up.health;
                    }
                }
            } else if (type == PacketType::PlayerDisconnect) {
                std::string reason;
                r.readString(reason, 256);
                m_state = ClientConnectionState::Disconnected;
                m_disconnectReason = reason;
                Log::info("Desconectado del servidor: {}", reason);
            }
            return;
        }
    }

    // Compatibilidad hacia atrás (F01)
    const auto* header = reinterpret_cast<const PacketHeader*>(buffer);
    if (header->type == PacketType::BlockModify) {
        const auto* bm = reinterpret_cast<const PacketBlockModify*>(buffer);
        BlockStateId state = BlockRegistry::global().fromLegacyId(static_cast<BlockID>(bm->blockID));
        if (bm->action == 0) {
            session.getWorld().setBlockState(bm->x, bm->y, bm->z, AIR_BLOCK_STATE);
        } else {
            session.getWorld().setBlockState(bm->x, bm->y, bm->z, state);
        }
    } else if (header->type == PacketType::PlayerUpdate) {
        const auto* pu = reinterpret_cast<const PacketPlayerUpdate*>(buffer);
        if (pu->playerID != m_assignedPlayerId) {
            PlayerEntity* rp = session.getPlayerById(pu->playerID);
            if (!rp) {
                rp = session.addPlayer(pu->playerID, "Player_" + std::to_string(pu->playerID),
                                       {pu->posX, pu->posY, pu->posZ});
            }
            if (rp) {
                rp->position = {pu->posX, pu->posY, pu->posZ};
                rp->yaw = pu->yaw;
                rp->pitch = pu->pitch;
            }
        }
    }
}

void NetworkClient::update(WorldSession& session, uint64_t currentSimTimeMs) {
    if (m_socket < 0) return;

    // 1. Drenar paquetes entrantes
    uint8_t buffer[2048];
    sockaddr_in sender{};
    socklen_t addrLen = sizeof(sender);

    while (true) {
        ssize_t bytes = recvfrom(m_socket, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&sender), &addrLen);
        if (bytes <= 0) break;
        // Solo se aceptan paquetes del servidor al que se conectó (evita suplantación en la LAN)
        if (sender.sin_addr.s_addr != m_serverAddr.sin_addr.s_addr || sender.sin_port != m_serverAddr.sin_port) {
            continue;
        }
        handleIncomingPacket(buffer, static_cast<size_t>(bytes), session, currentSimTimeMs);
    }

    // 2. Retransmisiones del canal fiable
    if (m_state == ClientConnectionState::Connected) {
        auto retransmissions = m_reliable.checkRetransmissions(currentSimTimeMs);
        for (const auto& pkt : retransmissions) {
            sendRaw(pkt.data(), pkt.size());
        }

        // 3. Telemetría de movimiento periódica (cada 50 ms = 20 Hz)
        if (currentSimTimeMs >= m_lastTeleTime + 50) {
            m_lastTeleTime = currentSimTimeMs;
            const PlayerEntity* pPlayer = session.getPlayerById(m_assignedPlayerId);
            const auto& player = pPlayer ? *pPlayer : session.getPlayer();

            PacketPlayerUpdate pu{};
            pu.header.type = PacketType::PlayerUpdate;
            pu.header.sequence = ++m_sequence;
            pu.playerID = m_assignedPlayerId;
            pu.posX = player.position.x;
            pu.posY = player.position.y;
            pu.posZ = player.position.z;
            pu.yaw = player.yaw;
            pu.pitch = player.pitch;
            sendRaw(&pu, sizeof(pu));
        }
    }
}

} // namespace OpenCraft::Net
