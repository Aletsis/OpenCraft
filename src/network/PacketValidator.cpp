#include "network/PacketValidator.hpp"
#include "world/Chunk.hpp"
#include <cmath>
#include <cstring>
#include <cctype>

namespace OpenCraft::Net {

bool PacketValidator::areFloatsFinite(const float* values, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        if (!std::isfinite(values[i])) {
            return false;
        }
    }
    return true;
}

bool PacketValidator::sanitizePlayerName(const char* rawName, size_t maxLen, char* outBuffer,
                                         size_t outBufferSize) noexcept {
    if (!rawName || !outBuffer || outBufferSize == 0) return false;

    size_t writeIdx = 0;
    for (size_t i = 0; i < maxLen && rawName[i] != '\0' && writeIdx + 1 < outBufferSize; ++i) {
        unsigned char c = static_cast<unsigned char>(rawName[i]);
        if (std::isprint(c)) {
            outBuffer[writeIdx++] = static_cast<char>(c);
        }
    }
    outBuffer[writeIdx] = '\0';
    return writeIdx > 0; // Rechaza nombres vacíos o que solo contenían caracteres no imprimibles
}

ValidationResult PacketValidator::validate(const void* data, size_t size) noexcept {
    if (!data || size < sizeof(PacketHeader)) {
        return ValidationResult::TooSmall;
    }

    // Comprobar si es un paquete con cabecera OCNT explícita (Fase 09)
    if (size >= 9) {
        uint32_t magic = 0;
        std::memcpy(&magic, data, sizeof(magic));
        if (magic == NETWORK_MAGIC) {
            uint8_t rawType = *(reinterpret_cast<const uint8_t*>(data) + 4);
            auto type = static_cast<PacketType>(rawType);
            ByteReader r(data, size);
            r.skip(9); // saltar magic (4) + type (1) + seq (4)

            switch (type) {
                case PacketType::Handshake: {
                    std::string name;
                    uint32_t ver = 0;
                    if (!deserializeHandshake(r, name, ver)) return ValidationResult::InvalidPayload;
                    if (name.empty() || name.size() > 32) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::HandshakeAck: {
                    HandshakeAckData ack;
                    if (!deserializeHandshakeAck(r, ack)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::HandshakeReject: {
                    HandshakeRejectData rej;
                    if (!deserializeHandshakeReject(r, rej)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::PlayerAction: {
                    PlayerActionData act;
                    if (!deserializePlayerAction(r, act)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::InventoryTransaction: {
                    InventoryTransactionData tx;
                    if (!deserializeInventoryTransaction(r, tx)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::TransactionResult: {
                    TransactionResultData res;
                    if (!deserializeTransactionResult(r, res)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::ChatMessage: {
                    ChatMessageData chat;
                    if (!deserializeChatMessage(r, chat)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::EntitySpawn: {
                    EntitySpawnData sp;
                    if (!deserializeEntitySpawn(r, sp)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::EntityUpdate: {
                    EntityUpdateData up;
                    if (!deserializeEntityUpdate(r, up)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::ReliableAck: {
                    uint32_t ackSeq = 0;
                    if (!deserializeReliableAck(r, ackSeq)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::Ping: {
                    uint64_t ts = 0;
                    if (!deserializePing(r, ts)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::Pong: {
                    uint64_t ts = 0;
                    if (!deserializePong(r, ts)) return ValidationResult::InvalidPayload;
                    return ValidationResult::Valid;
                }
                case PacketType::PlayerDisconnect: return ValidationResult::Valid;
                default: return ValidationResult::UnknownType;
            }
        }
    }

    // Ruta de compatibilidad hacia atrás para paquetes de structs planos (F01)
    const auto* header = reinterpret_cast<const PacketHeader*>(data);
    switch (header->type) {
        case PacketType::Handshake: {
            if (size != sizeof(PacketHandshake)) {
                return ValidationResult::SizeMismatch;
            }
            const auto* hs = reinterpret_cast<const PacketHandshake*>(data);
            char cleanName[32];
            if (!sanitizePlayerName(hs->playerName, sizeof(hs->playerName), cleanName, sizeof(cleanName))) {
                return ValidationResult::InvalidPayload;
            }
            return ValidationResult::Valid;
        }

        case PacketType::PlayerUpdate: {
            if (size != sizeof(PacketPlayerUpdate)) {
                return ValidationResult::SizeMismatch;
            }
            const auto* pu = reinterpret_cast<const PacketPlayerUpdate*>(data);
            float floats[] = {pu->posX, pu->posY, pu->posZ, pu->yaw, pu->pitch};
            if (!areFloatsFinite(floats, 5)) {
                return ValidationResult::NonFiniteFloat;
            }
            // Rango de seguridad de coordenadas para evitar corrupción de física
            if (std::abs(pu->posX) > 500000.0f || std::abs(pu->posZ) > 500000.0f || pu->posY < -256.0f ||
                pu->posY > 2048.0f) {
                return ValidationResult::InvalidPayload;
            }
            return ValidationResult::Valid;
        }

        case PacketType::BlockModify: {
            if (size != sizeof(PacketBlockModify)) {
                return ValidationResult::SizeMismatch;
            }
            const auto* bm = reinterpret_cast<const PacketBlockModify*>(data);
            if (bm->y < Chunk::MIN_Y || bm->y >= Chunk::MAX_Y) {
                return ValidationResult::InvalidPayload;
            }
            if (bm->action > 1) { // 0 = break, 1 = place
                return ValidationResult::InvalidPayload;
            }
            if (bm->blockID >= static_cast<uint8_t>(BlockID::Count)) {
                return ValidationResult::InvalidPayload;
            }
            return ValidationResult::Valid;
        }

        case PacketType::HandshakeAck:
        case PacketType::PlayerDisconnect:
            if (size != sizeof(PacketHeader)) {
                return ValidationResult::SizeMismatch;
            }
            return ValidationResult::Valid;

        default: return ValidationResult::UnknownType;
    }
}

} // namespace OpenCraft::Net
