#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include "core/PngWriter.hpp"
#include "item/Item.hpp"
#include "render/AtlasPainter.hpp"
#include "storage/WorldFormat.hpp"
#include "ui/BitmapFont.hpp"
#include "world/ChunkLighting.hpp"
#include "world/ChunkMesher.hpp"
#include "world/World.hpp"

using namespace OpenCraft;

static bool tileHasOpaquePixels(const std::vector<uint8_t>& px, int tile) {
    int tx = (tile % 16) * 16, ty = (tile / 16) * 16;
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            if (px[(static_cast<size_t>(ty + y) * AtlasPainter::ATLAS_SIZE + static_cast<size_t>(tx + x)) * 4 + 3] > 25)
                return true;
    return false;
}

static size_t countIndices(const ChunkMeshBuildResult& r, size_t pass) {
    size_t n = 0;
    for (const auto& s : r.sections) n += s.indices[pass].size();
    return n;
}

int main() {
    std::cout << "--- Atlas, mallado, luz, fuente y PNG ---" << std::endl;

    // 1. Atlas: cada textura de bloque y cada sprite de objeto tiene píxeles visibles
    auto atlas = AtlasPainter::paint();
    assert(atlas.size() == static_cast<size_t>(AtlasPainter::ATLAS_SIZE) * AtlasPainter::ATLAS_SIZE * 4);
    const auto& reg = BlockRegistry::global();
    std::set<int> blockTiles;
    for (size_t i = 1; i < reg.getStateCount(); ++i) {
        for (uint8_t t : reg.get(static_cast<BlockStateId>(i)).textureIndices) blockTiles.insert(t);
    }
    for (int t : blockTiles) {
        // El cristal es transparente salvo el marco; los bloques nunca pueden quedar invisibles
        assert(tileHasOpaquePixels(atlas, t) && "Ninguna textura de bloque puede ser totalmente transparente");
    }
    const auto& items = ItemRegistry::global();
    size_t sprites = 0;
    for (size_t i = 1; i < items.getItemCount(); ++i) {
        const auto& def = items.get(static_cast<ItemId>(i));
        int tile = AtlasPainter::itemIconTile(def.namespacedId);
        if (tile < 0) continue;
        assert(def.iconTileIndex == tile && "El registro debe usar el sprite propio del objeto");
        assert(tileHasOpaquePixels(atlas, tile));
        ++sprites;
    }
    assert(sprites > 80);
    assert(AtlasPainter::itemIconTile("opencraft:no_existe") == -1);
    assert(tileHasOpaquePixels(atlas, AtlasPainter::TILE_WHITE));
    for (int s = 0; s < 10; ++s) assert(tileHasOpaquePixels(atlas, AtlasPainter::TILE_CRACK_FIRST + s));
    std::cout << "[OK] Atlas: " << blockTiles.size() << " texturas de bloque y " << sprites << " sprites" << std::endl;

    // 2. Luz inicial: cielo abierto a 15, sombra bajo un techo, luz de bloque de un emisor
    World world;
    auto chunk = std::make_unique<Chunk>(0, 0);
    auto stone = reg.fromLegacyId(BlockID::Stone);
    for (int x = 0; x < 16; ++x)
        for (int z = 0; z < 16; ++z)
            for (int y = 0; y < 64; ++y) chunk->setBlockState(x, y, z, stone);
    for (int x = 2; x <= 6; ++x)
        for (int z = 2; z <= 6; ++z) chunk->setBlockState(x, 70, z, stone); // techo
    chunk->setBlockState(12, 64, 12, reg.fromLegacyId(BlockID::GlowRock));
    chunk->setBlockState(8, 64, 2, reg.fromLegacyId(BlockID::Water));
    chunk->setBlockState(9, 64, 2, reg.fromLegacyId(BlockID::Torch));
    chunk->setBlockState(10, 64, 2, reg.fromLegacyId(BlockID::WheatCrops));
    chunk->setBlockState(11, 64, 2, reg.fromLegacyId(BlockID::Glass));
    chunk->setBlockState(13, 64, 2, reg.fromLegacyId(BlockID::Leaves));
    world.setChunk(0, 0, std::move(chunk));
    Chunk* c = world.getChunk(0, 0);
    assert(c->isLightComputed());
    assert(c->getSkyLight(10, 80, 10) == 15 && "Aire a cielo abierto: luz plena");
    assert(c->getSkyLight(10, 64, 10) == 15);
    uint8_t shaded = c->getSkyLight(4, 64, 4);
    assert(shaded < 15 && shaded > 0 && "Bajo el techo llega luz difusa atenuada");
    assert(c->getBlockLight(12, 65, 12) > 10 && "La piedra luminosa ilumina su entorno");
    assert(c->getSkyLight(4, 30, 4) == 0 && "Dentro de la roca no hay luz");
    std::cout << "[OK] Luz inicial de cielo y de bloque" << std::endl;

    // 3. Mallado: caras ocultas, pases de render y vecinos sin cargar
    ChunkMeshInput input = ChunkMesher::capture(world, *c);
    assert(input.blockAt(-1, 30, 5) == ChunkMesher::UNLOADED_NEIGHBOR && "Sin vecino cargado se trata como macizo");
    assert(input.blockAt(5, 400, 5) == AIR_BLOCK_STATE);
    assert(input.skyAt(5, 400, 5) == 15);
    ChunkMeshBuildResult mesh;
    ChunkMesher::build(input, mesh);
    assert(mesh.sections.size() == static_cast<size_t>(Chunk::SECTION_COUNT));
    size_t opaque = countIndices(mesh, 0), cutout = countIndices(mesh, 1), translucent = countIndices(mesh, 2);
    assert(opaque > 0 && cutout > 0 && translucent > 0);
    // La roca interior no genera caras: el suelo macizo de 16x64x16 solo aporta su superficie
    assert(opaque < 16 * 16 * 6 * 6 * 4 && "Las caras internas deben descartarse");
    for (const auto& sec : mesh.sections) {
        for (size_t p = 0; p < 3; ++p) {
            assert(sec.indices[p].size() % 6 == 0 && "Cada cara son dos triángulos");
            for (const auto& v : sec.vertices[p]) {
                assert(v.skyLight >= 0.0f && v.skyLight <= 15.0f && v.ao > 0.0f && v.ao <= 1.0f);
            }
        }
    }
    // Con el vecino presente, las caras del borde aparecen
    auto east = std::make_unique<Chunk>(1, 0);
    world.setChunk(1, 0, std::move(east));
    ChunkMeshBuildResult mesh2;
    ChunkMesher::build(ChunkMesher::capture(world, *c), mesh2);
    assert(countIndices(mesh2, 0) > opaque && "Un vecino de aire descubre las caras del borde");
    // Un chunk vacío no produce geometría
    Chunk empty(5, 5);
    ChunkMeshBuildResult none;
    ChunkMesher::build(ChunkMesher::capture(world, empty), none);
    assert(countIndices(none, 0) + countIndices(none, 1) + countIndices(none, 2) == 0);
    std::cout << "[OK] Mallado: opacos " << opaque << ", recortados " << cutout << ", translúcidos " << translucent
              << std::endl;

    // 4. Fuente: UTF-8, acentos y medidas
    auto cps = BitmapFont::decodeUtf8("Ñandú ¿sí? ¡ok!");
    assert(cps.size() == 15 && cps[0] == U'Ñ');
    auto bad = BitmapFont::decodeUtf8(std::string("\xC3", 1));
    assert(bad.size() == 1 && bad[0] == U'?');
    auto badCont = BitmapFont::decodeUtf8(std::string("\xE2\x28\xA1", 3));
    assert(!badCont.empty() && badCont[0] == U'?');
    assert(BitmapFont::measure("") == 0);
    assert(BitmapFont::measure("il") < BitmapFont::measure("WW") && "Fuente proporcional");
    assert(BitmapFont::measure("Á") == BitmapFont::measure("A") && "Las mayúsculas acentuadas usan la letra base");
    assert(BitmapFont::glyphFor(U'☃').advance == BitmapFont::glyphFor(U'?').advance &&
           "Carácter sin glifo: interrogación");
    assert(BitmapFont::glyphFor(U' ').advance == 4);
    std::cout << "[OK] Fuente de mapa de bits" << std::endl;

    // 5. PNG: firma, cabecera, CRC de bloques y datos almacenados sin compresión
    std::vector<uint8_t> rgba(3 * 2 * 4, 0);
    for (size_t i = 0; i < rgba.size(); ++i) rgba[i] = static_cast<uint8_t>(i * 10);
    auto png = PngWriter::encodeRGBA(rgba, 3, 2);
    const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    assert(png.size() > 8 && std::memcmp(png.data(), sig, 8) == 0);
    assert(std::memcmp(png.data() + 12, "IHDR", 4) == 0);
    assert(png[16] == 0 && png[19] == 3 && png[23] == 2 && png[24] == 8 && png[25] == 6);
    uint32_t crc = computeCRC32(std::span<const uint8_t>(png.data() + 12, 17));
    uint32_t stored = (uint32_t(png[29]) << 24) | (uint32_t(png[30]) << 16) | (uint32_t(png[31]) << 8) | png[32];
    assert(crc == stored && "CRC del bloque IHDR");
    assert(std::memcmp(png.data() + png.size() - 8, "IEND", 4) == 0);
    assert(PngWriter::encodeRGBA(rgba, 0, 2).empty());
    assert(PngWriter::encodeRGBA(rgba, 10, 10).empty() && "Datos insuficientes");
    auto file = std::filesystem::temp_directory_path() / "opencraft_png_test" / "a.png";
    assert(PngWriter::writeRGBA(file, rgba, 3, 2));
    assert(std::filesystem::file_size(file) == png.size());
    std::filesystem::remove_all(file.parent_path());
    std::cout << "[OK] Codificador PNG" << std::endl;

    std::cout << "EXITO: atlas, mallado, luz, fuente y PNG verificados." << std::endl;
    return 0;
}
