#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include "world/Block.hpp"
#include "network/NetworkSerializer.hpp"

namespace OpenCraft::Net {

constexpr uint16_t DEFAULT_PORT = 35565;
constexpr uint16_t DEFAULT_DISCOVERY_PORT = 35566;
constexpr const char* DISCOVERY_MAGIC = "OPENCRAFT_LAN_DISCOVER_V1";
constexpr uint32_t NETWORK_MAGIC = 0x4F434E54; // 'OCNT' (OpenCraft Network Transport)
constexpr uint32_t PROTOCOL_VERSION = 100;     // Fase 09 v1.0

enum class PacketType : uint8_t {
    Handshake = 1,
    HandshakeAck = 2,
    PlayerUpdate = 3,
    BlockModify = 4,
    PlayerDisconnect = 5,
    PlayerHurt = 6,
    PlayerRespawn = 7,
    SleepState = 8,
    // Extensiones Fase 09
    HandshakeReject = 9,
    Ping = 10,
    Pong = 11,
    PlayerAction = 12,
    InventoryTransaction = 13,
    TransactionResult = 14,
    WorldSnapshot = 15,
    ChunkData = 16,
    EntitySpawn = 17,
    EntityDestroy = 18,
    EntityUpdate = 19,
    ChatMessage = 20,
    ReliableAck = 21
};

#pragma pack(push, 1)

struct PacketHeader {
    PacketType type;
    uint32_t sequence;
};

struct PacketHandshake {
    PacketHeader header;
    char playerName[32];
    uint32_t protocolVersion = PROTOCOL_VERSION;
};

struct PacketPlayerUpdate {
    PacketHeader header;
    uint32_t playerID;
    float posX, posY, posZ;
    float yaw, pitch;
};

struct PacketBlockModify {
    PacketHeader header;
    int32_t x, y, z;
    uint8_t blockID;
    uint8_t action; // 0 = break, 1 = place
};

struct PacketPlayerHurt {
    PacketHeader header;
    uint32_t playerID;
    float damage;
    float healthRemaining;
    uint8_t cause;
};

struct PacketPlayerRespawn {
    PacketHeader header;
    uint32_t playerID;
};

struct PacketSleepState {
    PacketHeader header;
    uint32_t playerID;
    uint8_t isSleeping;
    int32_t bedX, bedY, bedZ;
};

#pragma pack(pop)

// ==============================================================================
// Estructuras de alto nivel y tipos para Fase 09
// ==============================================================================

enum class RejectReason : uint8_t {
    IncompatibleVersion = 1,
    ServerFull = 2,
    BannedOrNotWhitelisted = 3,
    InvalidName = 4,
    AlreadyConnected = 5
};

enum class PlayerActionType : uint8_t {
    BreakBlock = 0,
    PlaceBlock = 1,
    DigStart = 2,
    DigProgress = 3,
    DigCancel = 4,
    DigComplete = 5,
    AttackEntity = 6,
    UseShield = 7,
    SelectSlot = 8,
    DropItem = 9,
    Respawn = 10,
    InteractBlock = 11
};

struct HandshakeAckData {
    uint32_t protocolVersion = PROTOCOL_VERSION;
    uint32_t assignedPlayerId = 0;
    uint32_t sessionId = 0;
    float spawnX = 8.0f;
    float spawnY = 68.0f;
    float spawnZ = 8.0f;
    uint64_t worldSeed = 0;
    uint64_t serverTick = 0;
    bool isOperator = false;
};

struct HandshakeRejectData {
    RejectReason reason = RejectReason::IncompatibleVersion;
    std::string message;
};

struct PlayerActionData {
    uint32_t entityId = 0;
    PlayerActionType action = PlayerActionType::BreakBlock;
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    uint16_t blockState = 0;
    uint32_t targetEntityId = 0;
    uint8_t flag = 0; // p. ej. isBlocking para escudo
    uint16_t count = 0;
};

struct InventoryTransactionData {
    uint32_t entityId = 0;
    uint8_t action = 0; // 0=Move, 1=Swap, 2=Split, 3=QuickTransfer
    uint8_t fromContainer = 0;
    uint16_t fromSlot = 0;
    uint8_t toContainer = 0;
    uint16_t toSlot = 0;
    uint16_t amount = 0;
    uint32_t expectedRevision = 0;
};

struct TransactionResultData {
    uint32_t entityId = 0;
    bool success = false;
    uint8_t errorCode = 0;
    uint32_t newRevision = 0;
};

struct ChatMessageData {
    uint32_t senderId = 0;
    std::string senderName;
    std::string text;
    bool isSystem = false;
};

struct EntitySpawnData {
    uint32_t entityId = 0;
    uint8_t entityType = 0; // 0=Player, 1=Mob, 2=Item, 3=Projectile, 4=ExpOrb
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    float yaw = 0.0f, pitch = 0.0f;
    uint8_t subType = 0; // MobType o metadata
    float health = 20.0f;
    uint16_t itemId = 0;
    uint16_t count = 0;
};

struct EntityUpdateData {
    uint32_t entityId = 0;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float vx = 0.0f, vy = 0.0f, vz = 0.0f;
    float yaw = 0.0f, pitch = 0.0f;
    float health = 20.0f;
    uint8_t stateFlags = 0; // bit 0: isSprinting, bit 1: isSneaking, bit 2: isBlocking, bit 3: isGrounded
};

// ==============================================================================
// Funciones de serialización y deserialización explícita
// ==============================================================================

inline void serializeHeader(ByteWriter& w, PacketType type, uint32_t seq) {
    w.writeUint32(NETWORK_MAGIC);
    w.writeUint8(static_cast<uint8_t>(type));
    w.writeUint32(seq);
}

inline bool deserializeHeader(ByteReader& r, PacketType& outType, uint32_t& outSeq) {
    uint32_t magic = 0;
    if (!r.readUint32(magic) || magic != NETWORK_MAGIC) {
        return false;
    }
    uint8_t t = 0;
    if (!r.readUint8(t)) return false;
    outType = static_cast<PacketType>(t);
    return r.readUint32(outSeq);
}

inline void serializeHandshake(ByteWriter& w, uint32_t seq, const std::string& playerName,
                               uint32_t version = PROTOCOL_VERSION) {
    serializeHeader(w, PacketType::Handshake, seq);
    w.writeUint32(version);
    w.writeString(playerName);
}

inline bool deserializeHandshake(ByteReader& r, std::string& outName, uint32_t& outVersion) {
    if (!r.readUint32(outVersion)) return false;
    return r.readString(outName, 32);
}

inline void serializeHandshakeAck(ByteWriter& w, uint32_t seq, const HandshakeAckData& ack) {
    serializeHeader(w, PacketType::HandshakeAck, seq);
    w.writeUint32(ack.protocolVersion);
    w.writeUint32(ack.assignedPlayerId);
    w.writeUint32(ack.sessionId);
    w.writeFloat(ack.spawnX);
    w.writeFloat(ack.spawnY);
    w.writeFloat(ack.spawnZ);
    w.writeUint64(ack.worldSeed);
    w.writeUint64(ack.serverTick);
    w.writeUint8(ack.isOperator ? 1 : 0);
}

inline bool deserializeHandshakeAck(ByteReader& r, HandshakeAckData& outAck) {
    if (!r.readUint32(outAck.protocolVersion) || !r.readUint32(outAck.assignedPlayerId) ||
        !r.readUint32(outAck.sessionId) || !r.readFloat(outAck.spawnX) || !r.readFloat(outAck.spawnY) ||
        !r.readFloat(outAck.spawnZ) || !r.readUint64(outAck.worldSeed) || !r.readUint64(outAck.serverTick)) {
        return false;
    }
    uint8_t op = 0;
    if (!r.readUint8(op)) return false;
    outAck.isOperator = (op != 0);
    return true;
}

inline void serializeHandshakeReject(ByteWriter& w, uint32_t seq, const HandshakeRejectData& rej) {
    serializeHeader(w, PacketType::HandshakeReject, seq);
    w.writeUint8(static_cast<uint8_t>(rej.reason));
    w.writeString(rej.message);
}

inline bool deserializeHandshakeReject(ByteReader& r, HandshakeRejectData& outRej) {
    uint8_t reason = 0;
    if (!r.readUint8(reason)) return false;
    outRej.reason = static_cast<RejectReason>(reason);
    return r.readString(outRej.message, 256);
}

inline void serializeChatMessage(ByteWriter& w, uint32_t seq, const ChatMessageData& chat) {
    serializeHeader(w, PacketType::ChatMessage, seq);
    w.writeUint32(chat.senderId);
    w.writeString(chat.senderName);
    w.writeString(chat.text);
    w.writeUint8(chat.isSystem ? 1 : 0);
}

inline bool deserializeChatMessage(ByteReader& r, ChatMessageData& outChat) {
    if (!r.readUint32(outChat.senderId) || !r.readString(outChat.senderName, 32) || !r.readString(outChat.text, 512)) {
        return false;
    }
    uint8_t sys = 0;
    if (!r.readUint8(sys)) return false;
    outChat.isSystem = (sys != 0);
    return true;
}

inline void serializePlayerAction(ByteWriter& w, uint32_t seq, const PlayerActionData& act) {
    serializeHeader(w, PacketType::PlayerAction, seq);
    w.writeUint32(act.entityId);
    w.writeUint8(static_cast<uint8_t>(act.action));
    w.writeInt32(act.x);
    w.writeInt32(act.y);
    w.writeInt32(act.z);
    w.writeUint16(act.blockState);
    w.writeUint32(act.targetEntityId);
    w.writeUint8(act.flag);
    w.writeUint16(act.count);
}

inline bool deserializePlayerAction(ByteReader& r, PlayerActionData& outAct) {
    uint8_t a = 0;
    if (!r.readUint32(outAct.entityId) || !r.readUint8(a) || !r.readInt32(outAct.x) || !r.readInt32(outAct.y) ||
        !r.readInt32(outAct.z) || !r.readUint16(outAct.blockState) || !r.readUint32(outAct.targetEntityId) ||
        !r.readUint8(outAct.flag) || !r.readUint16(outAct.count)) {
        return false;
    }
    outAct.action = static_cast<PlayerActionType>(a);
    return true;
}

inline void serializeInventoryTransaction(ByteWriter& w, uint32_t seq, const InventoryTransactionData& tx) {
    serializeHeader(w, PacketType::InventoryTransaction, seq);
    w.writeUint32(tx.entityId);
    w.writeUint8(tx.action);
    w.writeUint8(tx.fromContainer);
    w.writeUint16(tx.fromSlot);
    w.writeUint8(tx.toContainer);
    w.writeUint16(tx.toSlot);
    w.writeUint16(tx.amount);
    w.writeUint32(tx.expectedRevision);
}

inline bool deserializeInventoryTransaction(ByteReader& r, InventoryTransactionData& outTx) {
    return r.readUint32(outTx.entityId) && r.readUint8(outTx.action) && r.readUint8(outTx.fromContainer) &&
           r.readUint16(outTx.fromSlot) && r.readUint8(outTx.toContainer) && r.readUint16(outTx.toSlot) &&
           r.readUint16(outTx.amount) && r.readUint32(outTx.expectedRevision);
}

inline void serializeTransactionResult(ByteWriter& w, uint32_t seq, const TransactionResultData& res) {
    serializeHeader(w, PacketType::TransactionResult, seq);
    w.writeUint32(res.entityId);
    w.writeUint8(res.success ? 1 : 0);
    w.writeUint8(res.errorCode);
    w.writeUint32(res.newRevision);
}

inline bool deserializeTransactionResult(ByteReader& r, TransactionResultData& outRes) {
    uint8_t s = 0;
    if (!r.readUint32(outRes.entityId) || !r.readUint8(s) || !r.readUint8(outRes.errorCode) ||
        !r.readUint32(outRes.newRevision)) {
        return false;
    }
    outRes.success = (s != 0);
    return true;
}

inline void serializeEntitySpawn(ByteWriter& w, uint32_t seq, const EntitySpawnData& sp) {
    serializeHeader(w, PacketType::EntitySpawn, seq);
    w.writeUint32(sp.entityId);
    w.writeUint8(sp.entityType);
    w.writeFloat(sp.x);
    w.writeFloat(sp.y);
    w.writeFloat(sp.z);
    w.writeFloat(sp.vx);
    w.writeFloat(sp.vy);
    w.writeFloat(sp.vz);
    w.writeFloat(sp.yaw);
    w.writeFloat(sp.pitch);
    w.writeUint8(sp.subType);
    w.writeFloat(sp.health);
    w.writeUint16(sp.itemId);
    w.writeUint16(sp.count);
}

inline bool deserializeEntitySpawn(ByteReader& r, EntitySpawnData& outSp) {
    return r.readUint32(outSp.entityId) && r.readUint8(outSp.entityType) && r.readFloat(outSp.x) &&
           r.readFloat(outSp.y) && r.readFloat(outSp.z) && r.readFloat(outSp.vx) && r.readFloat(outSp.vy) &&
           r.readFloat(outSp.vz) && r.readFloat(outSp.yaw) && r.readFloat(outSp.pitch) && r.readUint8(outSp.subType) &&
           r.readFloat(outSp.health) && r.readUint16(outSp.itemId) && r.readUint16(outSp.count);
}

inline void serializeEntityUpdate(ByteWriter& w, uint32_t seq, const EntityUpdateData& up) {
    serializeHeader(w, PacketType::EntityUpdate, seq);
    w.writeUint32(up.entityId);
    w.writeFloat(up.x);
    w.writeFloat(up.y);
    w.writeFloat(up.z);
    w.writeFloat(up.vx);
    w.writeFloat(up.vy);
    w.writeFloat(up.vz);
    w.writeFloat(up.yaw);
    w.writeFloat(up.pitch);
    w.writeFloat(up.health);
    w.writeUint8(up.stateFlags);
}

inline bool deserializeEntityUpdate(ByteReader& r, EntityUpdateData& outUp) {
    return r.readUint32(outUp.entityId) && r.readFloat(outUp.x) && r.readFloat(outUp.y) && r.readFloat(outUp.z) &&
           r.readFloat(outUp.vx) && r.readFloat(outUp.vy) && r.readFloat(outUp.vz) && r.readFloat(outUp.yaw) &&
           r.readFloat(outUp.pitch) && r.readFloat(outUp.health) && r.readUint8(outUp.stateFlags);
}

inline void serializeReliableAck(ByteWriter& w, uint32_t seq, uint32_t ackSeq) {
    serializeHeader(w, PacketType::ReliableAck, seq);
    w.writeUint32(ackSeq);
}

inline bool deserializeReliableAck(ByteReader& r, uint32_t& outAckSeq) { return r.readUint32(outAckSeq); }

inline void serializePing(ByteWriter& w, uint32_t seq, uint64_t timestamp) {
    serializeHeader(w, PacketType::Ping, seq);
    w.writeUint64(timestamp);
}

inline bool deserializePing(ByteReader& r, uint64_t& outTimestamp) { return r.readUint64(outTimestamp); }

inline void serializePong(ByteWriter& w, uint32_t seq, uint64_t timestamp) {
    serializeHeader(w, PacketType::Pong, seq);
    w.writeUint64(timestamp);
}

inline bool deserializePong(ByteReader& r, uint64_t& outTimestamp) { return r.readUint64(outTimestamp); }

} // namespace OpenCraft::Net
