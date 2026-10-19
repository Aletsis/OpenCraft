#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F01 Test 2] Registro con > 255 estados y estabilidad de nombres con namespace ---" << std::endl;

    BlockRegistry registry;
    registry.reset();

    constexpr size_t CUSTOM_BLOCK_COUNT = 300;

    // 1. Registrar 300 bloques adicionales
    std::vector<BlockStateId> registeredIds;
    registeredIds.reserve(CUSTOM_BLOCK_COUNT);

    for (size_t i = 0; i < CUSTOM_BLOCK_COUNT; ++i) {
        std::string id = "mod_test:block_" + std::to_string(i);
        BlockProperties props{.namespacedId = id,
                              .displayName = "Test Block " + std::to_string(i),
                              .isSolid = (i % 2 == 0),
                              .isOpaque = true,
                              .isTransparent = false,
                              .textureIndices = {static_cast<uint8_t>(i % 16), 0, 0, 0, 0, 0},
                              .legacyId = BlockID::Stone};
        BlockStateId stateId = registry.registerBlock(id, props);
        registeredIds.push_back(stateId);
    }

    std::cout << "Total de estados registrados: " << registry.getStateCount() << std::endl;
    assert(registry.getStateCount() > 255 && "El registro debe soportar más de 255 estados sin limitación de 8 bits");

    // Verificar que los IDs mayores a 255 no sufren truncamiento
    BlockStateId highId = registeredIds[280];
    assert(highId >= 280 && "El ID numérico no debe truncarse a 8 bits (uint16_t requerido)");
    assert(registry.getNamespacedId(highId) == "mod_test:block_280");
    assert(registry.getStateId("mod_test:block_280") == highId);
    (void)highId;

    // 2. Fixture lógica basada en nombres con namespace
    struct LogicalPlacement {
        int x, y, z;
        std::string blockName;
    };

    std::vector<LogicalPlacement> fixture = {{0, 10, 0, "mod_test:block_10"},
                                             {5, 20, 5, "mod_test:block_256"},
                                             {9, 30, 9, "mod_test:block_299"},
                                             {2, 15, 2, "opencraft:grass_block"}};

    // 3. Crear una nueva instancia del registro e insertar en orden inverso
    BlockRegistry reversedRegistry;
    reversedRegistry.reset();

    for (int i = static_cast<int>(CUSTOM_BLOCK_COUNT) - 1; i >= 0; --i) {
        std::string id = "mod_test:block_" + std::to_string(i);
        BlockProperties props{.namespacedId = id,
                              .displayName = "Reversed Block " + std::to_string(i),
                              .isSolid = (i % 2 == 0),
                              .isOpaque = true,
                              .isTransparent = false,
                              .textureIndices = {static_cast<uint8_t>(i % 16), 0, 0, 0, 0, 0},
                              .legacyId = BlockID::Stone};
        reversedRegistry.registerBlock(id, props);
    }

    // 4. Comprobar que resolver la fixture en el registro reordenado preserva exactamente la identidad del contenido
    for (const auto& entry : fixture) {
        BlockStateId idInReversed = reversedRegistry.getStateId(entry.blockName);
        assert(idInReversed != AIR_BLOCK_STATE || entry.blockName == "opencraft:air");
        const std::string& resolvedName = reversedRegistry.getNamespacedId(idInReversed);
        assert(resolvedName == entry.blockName &&
               "La identidad del contenido debe conservarse independientemente del orden de registro");
        (void)resolvedName;
    }

    std::cout << "EXITO: Registro soporta " << registry.getStateCount()
              << " estados sin truncamiento a 8-bit y preserva identidades de namespace." << std::endl;
    return 0;
}
