#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include "network/NetworkProtocol.hpp"

namespace OpenCraft::Net {

enum class ValidationResult : uint8_t {
    Valid = 0,
    TooSmall,
    SizeMismatch,
    UnknownType,
    InvalidPayload,
    NonFiniteFloat
};

class PacketValidator {
public:
    // Valida un buffer crudo recibido de la red antes de castear a estructuras de paquetes
    static ValidationResult validate(const void* data, size_t size) noexcept;

    // Métodos específicos de extracción segura
    static bool sanitizePlayerName(const char* rawName, size_t maxLen, char* outBuffer, size_t outBufferSize) noexcept;
    static bool areFloatsFinite(const float* values, size_t count) noexcept;
};

} // namespace OpenCraft::Net
