#include <iostream>
#include <cassert>
#include "world/BlockRegistry.hpp"
#include "world/ChunkSection.hpp"
#include "world/Chunk.hpp"
#include "world/ChunkMesh.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "[TEST] Running test_render_passes_and_ao..." << std::endl;

    const auto& reg = BlockRegistry::global();

    // 1. Verificar clasificación de RenderPass para bloques de Phase 7
    BlockProperties stoneProp = reg.get(reg.getStateId("opencraft:stone"));
    assert(stoneProp.isOpaque);

    BlockProperties leavesProp = reg.get(reg.getStateId("opencraft:leaves"));
    assert(!leavesProp.isOpaque && leavesProp.isTransparent);

    BlockProperties glassProp = reg.get(reg.getStateId("opencraft:glass"));
    assert(!glassProp.isOpaque && glassProp.isTransparent);

    BlockProperties waterProp = reg.get(reg.getStateId("opencraft:water"));
    assert(!waterProp.isOpaque && waterProp.isTransparent);

    BlockProperties torchProp = reg.get(reg.getStateId("opencraft:torch"));
    assert(!torchProp.isOpaque && torchProp.lightEmission == 14);

    BlockProperties lavaProp = reg.get(reg.getStateId("opencraft:lava"));
    assert(lavaProp.lightEmission == 15);
    std::cout << "  ✓ Block registry attributes correctly assign render properties" << std::endl;

    // 2. VoxelVertex contiene atributos de iluminación dual (skyLight y blockLight)
    VoxelVertex v{.x = 1.0f,
                  .y = 2.0f,
                  .z = 3.0f,
                  .u = 0.5f,
                  .v = 0.5f,
                  .nx = 0.0f,
                  .ny = 1.0f,
                  .nz = 0.0f,
                  .ao = 0.8f,
                  .skyLight = 15.0f,
                  .blockLight = 14.0f};
    assert(v.skyLight == 15.0f);
    assert(v.blockLight == 14.0f);
    assert(v.ao == 0.8f);
    std::cout << "  ✓ VoxelVertex supports dual light channels and AO factor" << std::endl;

    // 3. RenderPass enum tiene 3 pases contiguos
    assert(static_cast<uint8_t>(RenderPass::Opaque) == 0);
    assert(static_cast<uint8_t>(RenderPass::Cutout) == 1);
    assert(static_cast<uint8_t>(RenderPass::Translucent) == 2);
    assert(static_cast<uint8_t>(RenderPass::Count) == 3);
    std::cout << "  ✓ RenderPass enum correctly ordered for 3-pass pipeline" << std::endl;

    // 4. ChunkSection almacena 4096 bytes de luz en formato de nibbles empaquetados
    ChunkSection section(0);
    section.setSkyLight(5, 5, 5, 15);
    section.setBlockLight(5, 5, 5, 14);

    assert(section.getSkyLight(5, 5, 5) == 15);
    assert(section.getBlockLight(5, 5, 5) == 14);

    // Otro bloque en la misma sección con diferentes niveles
    section.setSkyLight(6, 5, 5, 7);
    section.setBlockLight(6, 5, 5, 3);
    assert(section.getSkyLight(6, 5, 5) == 7);
    assert(section.getBlockLight(6, 5, 5) == 3);

    // El primer bloque no debe haber sido alterado
    assert(section.getSkyLight(5, 5, 5) == 15);
    assert(section.getBlockLight(5, 5, 5) == 14);
    std::cout << "  ✓ ChunkSection packed nibbles maintain dual channel independence without corruption" << std::endl;

    std::cout << "[TEST PASSED] test_render_passes_and_ao passed successfully!" << std::endl;
    return 0;
}
