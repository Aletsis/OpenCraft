#include <iostream>
#include <cassert>
#include <vector>
#include <cstring>
#include <limits>
#include "network/PacketValidator.hpp"

using namespace OpenCraft::Net;

int main() {
    std::cout << "--- [F01 Test 3] Validación y saneamiento de paquetes de red (Fuzzing y límites) ---" << std::endl;

    // 1. Inyección de tamaños 0 a 1024 con buffers arbitrarios
    std::vector<uint8_t> fuzzBuffer(1024, 0xAA);

    for (size_t size = 0; size <= fuzzBuffer.size(); ++size) {
        ValidationResult res = PacketValidator::validate(fuzzBuffer.data(), size);
        assert(res != ValidationResult::Valid && "Un buffer de bytes arbitrarios no debe ser aceptado como válido");
        (void)res;
    }

    // 2. Paquetes válidos truncados
    PacketPlayerUpdate puValid{};
    puValid.header.type = PacketType::PlayerUpdate;
    puValid.header.sequence = 1;
    puValid.playerID = 5;
    puValid.posX = 10.0f;
    puValid.posY = 32.0f;
    puValid.posZ = 10.0f;
    puValid.yaw = 90.0f;
    puValid.pitch = 0.0f;

    // Comprobar que el paquete completo es válido
    assert(PacketValidator::validate(&puValid, sizeof(puValid)) == ValidationResult::Valid);

    // Comprobar que cualquier tamaño truncado es rechazado
    for (size_t truncated = 0; truncated < sizeof(puValid); ++truncated) {
        ValidationResult res = PacketValidator::validate(&puValid, truncated);
        assert(res == ValidationResult::TooSmall || res == ValidationResult::SizeMismatch);
        (void)res;
    }

    // 3. Tipos desconocidos
    PacketHeader unknownHdr{};
    unknownHdr.type = static_cast<PacketType>(99);
    unknownHdr.sequence = 1;
    assert(PacketValidator::validate(&unknownHdr, sizeof(unknownHdr)) == ValidationResult::UnknownType);

    unknownHdr.type = static_cast<PacketType>(0);
    assert(PacketValidator::validate(&unknownHdr, sizeof(unknownHdr)) == ValidationResult::UnknownType);
    (void)unknownHdr;

    // 4. Inyección de floats no finitos (NaN, Infinito)
    PacketPlayerUpdate puCorrupt = puValid;
    puCorrupt.posX = std::numeric_limits<float>::quiet_NaN();
    assert(PacketValidator::validate(&puCorrupt, sizeof(puCorrupt)) == ValidationResult::NonFiniteFloat);

    puCorrupt = puValid;
    puCorrupt.posY = std::numeric_limits<float>::infinity();
    assert(PacketValidator::validate(&puCorrupt, sizeof(puCorrupt)) == ValidationResult::NonFiniteFloat);

    // 5. Nombres sin terminador nulo o desbordados
    PacketHandshake hs{};
    hs.header.type = PacketType::Handshake;
    hs.header.sequence = 2;
    std::memset(hs.playerName, 'A', sizeof(hs.playerName)); // sin '\0'

    char cleanName[32];
    bool sanitized =
        PacketValidator::sanitizePlayerName(hs.playerName, sizeof(hs.playerName), cleanName, sizeof(cleanName));
    assert(sanitized && "Debe sanitizar nombre truncándolo a buffer seguro");
    assert(cleanName[sizeof(cleanName) - 1] == '\0' && "El nombre resultante debe estar terminado en null");
    assert(std::strlen(cleanName) < sizeof(cleanName));
    (void)sanitized;

    // 6. Coordenadas de bloque fuera de rango
    PacketBlockModify bm{};
    bm.header.type = PacketType::BlockModify;
    bm.header.sequence = 3;
    bm.x = 0;
    bm.y = -100; // Fuera de rango de altura (< MIN_Y)
    bm.z = 0;
    bm.blockID = 1;
    bm.action = 0;

    assert(PacketValidator::validate(&bm, sizeof(bm)) == ValidationResult::InvalidPayload);

    bm.y = 400; // >= MAX_Y (320)
    assert(PacketValidator::validate(&bm, sizeof(bm)) == ValidationResult::InvalidPayload);

    bm.y = 30;
    bm.action = 5; // Acción inválida
    assert(PacketValidator::validate(&bm, sizeof(bm)) == ValidationResult::InvalidPayload);
    (void)bm;

    std::cout
        << "EXITO: Todos los casos de fuzzing, paquetes truncados y payloads no válidos fueron rechazados limpiamente."
        << std::endl;
    return 0;
}
