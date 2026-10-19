#include "world/ChunkMesher.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include <algorithm>

namespace OpenCraft {

BlockStateId ChunkMeshInput::blockAt(int lx, int y, int lz) const noexcept {
    if (y >= maxY) return AIR_BLOCK_STATE;
    if (y < minY) return ChunkMesher::UNLOADED_NEIGHBOR; // bajo el rango: macizo, sin caras
    return blocks[index(lx, y, lz)];
}

uint8_t ChunkMeshInput::skyAt(int lx, int y, int lz) const noexcept {
    if (y >= maxY) return 15;
    if (y < minY) return 0;
    return static_cast<uint8_t>(light[index(lx, y, lz)] >> 4);
}

uint8_t ChunkMeshInput::blockLightAt(int lx, int y, int lz) const noexcept {
    if (y >= maxY || y < minY) return 0;
    return static_cast<uint8_t>(light[index(lx, y, lz)] & 0x0F);
}

namespace ChunkMesher {

ChunkMeshInput capture(const World& world, const Chunk& chunk) {
    ChunkMeshInput in;
    in.chunkX = chunk.getChunkX();
    in.chunkZ = chunk.getChunkZ();
    in.meshVersion = chunk.getMeshVersion();

    int lowest = -1, highest = -1;
    for (int i = 0; i < Chunk::SECTION_COUNT; ++i) {
        const ChunkSection* sec = chunk.getSectionByIndex(static_cast<size_t>(i));
        bool has = sec && !sec->isEmpty();
        in.sectionHasBlocks[static_cast<size_t>(i)] = has;
        if (has) {
            if (lowest < 0) lowest = i;
            highest = i;
        }
    }
    if (lowest < 0) {
        in.minY = in.maxY = 0;
        return in;
    }
    in.minY = std::max(Chunk::MIN_Y, Chunk::indexToSectionY(lowest) * Chunk::SECTION_SIZE - 1);
    in.maxY = std::min(Chunk::MAX_Y, (Chunk::indexToSectionY(highest) + 1) * Chunk::SECTION_SIZE + 1);

    const size_t total = static_cast<size_t>(in.maxY - in.minY) * ChunkMeshInput::PAD * ChunkMeshInput::PAD;
    in.blocks.assign(total, UNLOADED_NEIGHBOR);
    in.light.assign(total, 0xF0);

    const Chunk* neighbors[3][3] = {};
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            neighbors[dz + 1][dx + 1] = world.getChunk(in.chunkX + dx, in.chunkZ + dz);
        }
    }

    for (int lz = -1; lz <= 16; ++lz) {
        int cz = (lz < 0) ? 0 : (lz > 15 ? 2 : 1);
        int sz = (lz + 16) % 16;
        for (int lx = -1; lx <= 16; ++lx) {
            int cx = (lx < 0) ? 0 : (lx > 15 ? 2 : 1);
            const Chunk* src = neighbors[cz][cx];
            if (!src) continue;
            int sx = (lx + 16) % 16;
            for (int y = in.minY; y < in.maxY; ++y) {
                size_t idx = in.index(lx, y, lz);
                in.blocks[idx] = src->getBlockState(sx, y, sz);
                in.light[idx] =
                    static_cast<uint8_t>((src->getSkyLight(sx, y, sz) << 4) | src->getBlockLight(sx, y, sz));
            }
        }
    }
    return in;
}

namespace {

enum class Shape : uint8_t {
    None,
    Cube,
    Fluid,
    Cross,
    Box
};

enum class Pass : uint8_t {
    Opaque = 0,
    Cutout = 1,
    Translucent = 2
};

struct BlockVisual {
    Shape shape = Shape::Cube;
    Pass pass = Pass::Opaque;
    // Caja en dieciseisavos para Shape::Box
    float minX = 0, minY = 0, minZ = 0, maxX = 16, maxY = 16, maxZ = 16;
};

bool idIs(const BlockProperties& p, std::string_view name) {
    return p.namespacedId.size() > 10 && std::string_view(p.namespacedId).substr(10) == name;
}

bool idHas(const BlockProperties& p, std::string_view part) { return p.namespacedId.find(part) != std::string::npos; }

BlockVisual classify(const BlockProperties& p) {
    BlockVisual v;
    if (p.legacyId == BlockID::Air || idIs(p, "air")) {
        v.shape = Shape::None;
        return v;
    }
    if (idHas(p, "water") || idIs(p, "ice")) {
        v.shape = idIs(p, "ice") ? Shape::Cube : Shape::Fluid;
        v.pass = Pass::Translucent;
        return v;
    }
    if (idHas(p, "lava")) {
        v.shape = Shape::Fluid;
        v.pass = Pass::Opaque;
        return v;
    }
    if (idIs(p, "wheat_crops") || idIs(p, "carrots") || idIs(p, "potatoes") || idIs(p, "fire") ||
        idIs(p, "abyss_portal")) {
        v.shape = Shape::Cross;
        v.pass = Pass::Cutout;
        return v;
    }
    auto box = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
        v.shape = Shape::Box;
        v.pass = Pass::Cutout;
        v.minX = x0;
        v.minY = y0;
        v.minZ = z0;
        v.maxX = x1;
        v.maxY = y1;
        v.maxZ = z1;
        return v;
    };
    if (idHas(p, "torch")) return box(7, 0, 7, 9, 10, 9);
    if (idHas(p, "rail") || idIs(p, "cinnabar_wire")) return box(0, 0, 0, 16, 1, 16);
    if (idIs(p, "snow")) return box(0, 0, 0, 16, 2, 16);
    if (idHas(p, "pressure_plate")) return box(1, 0, 1, 15, 1, 15);
    if (idHas(p, "button")) return box(5, 0, 6, 11, 2, 10);
    if (idIs(p, "lever")) return box(5, 0, 4, 11, 3, 12);
    if (idIs(p, "farmland")) return box(0, 0, 0, 16, 15, 16);
    if (idIs(p, "repeater") || idIs(p, "comparator") || idIs(p, "daylight_detector")) return box(0, 0, 0, 16, 2, 16);
    if (idIs(p, "rim_portal_frame")) return box(0, 0, 0, 16, 13, 16);
    if (idIs(p, "rim_portal")) return box(0, 11, 0, 16, 12, 16);
    if (idIs(p, "bed")) return box(0, 0, 0, 16, 9, 16);
    if (idIs(p, "chest")) return box(1, 0, 1, 15, 14, 15);
    if (idIs(p, "brewing_stand")) return box(7, 0, 7, 9, 14, 9);
    if (idIs(p, "dragon_egg")) return box(2, 0, 2, 14, 16, 14);
    if (idIs(p, "oak_sign")) return box(0, 7, 7, 16, 16, 9);
    if (idIs(p, "decorated_pot")) return box(2, 0, 2, 14, 15, 14);
    if (!p.isOpaque) {
        v.pass = Pass::Cutout;
    }
    return v;
}

// Tabla de visuales por estado de bloque (el registro es inmutable tras el arranque)
const BlockVisual& visualFor(BlockStateId state) {
    static thread_local std::vector<BlockVisual> cache;
    static thread_local std::vector<bool> cached;
    if (state >= cache.size()) {
        size_t n = std::max<size_t>(static_cast<size_t>(state) + 1, BlockRegistry::global().getStateCount());
        cache.resize(n);
        cached.resize(n, false);
    }
    if (!cached[state]) {
        cache[state] = classify(BlockRegistry::global().get(state));
        cached[state] = true;
    }
    return cache[state];
}

struct FaceDef {
    int normal[3];
    int corners[4][3]; // esquinas del cubo unitario en orden antihorario visto desde fuera
    float shade;       // sombreado direccional fijo (estilo clásico)
};

// Orden: Top, Bottom, North(-Z), South(+Z), West(-X), East(+X), igual que BlockFace/textureIndices
const FaceDef FACES[6] = {
    {{0, 1, 0}, {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}, 1.00f},
    {{0, -1, 0}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}, 0.50f},
    {{0, 0, -1}, {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}, 0.80f},
    {{0, 0, 1}, {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}, 0.80f},
    {{-1, 0, 0}, {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}, 0.60f},
    {{1, 0, 0}, {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}, 0.60f},
};

constexpr float AO_CURVE[4] = {0.42f, 0.62f, 0.81f, 1.0f};
constexpr float TILE = 1.0f / 16.0f;
constexpr float UV_INSET = 1.0f / 4096.0f; // evita sangrado entre teselas vecinas del atlas

class SectionBuilder {
public:
    SectionBuilder(const ChunkMeshInput& in, const BlockRegistry& reg, SectionMeshData& out, int baseY)
        : m_in(in),
          m_reg(reg),
          m_out(out),
          m_baseY(baseY) {}

    bool isOpaqueAt(int x, int y, int z) const {
        BlockStateId s = m_in.blockAt(x, y, z);
        if (s == UNLOADED_NEIGHBOR) return true;
        const auto& p = m_reg.get(s);
        return p.isOpaque && visualFor(s).shape == Shape::Cube;
    }

    void addBlock(int lx, int y, int lz, BlockStateId state) {
        const BlockVisual& vis = visualFor(state);
        const auto& props = m_reg.get(state);
        switch (vis.shape) {
            case Shape::None: return;
            case Shape::Cube: addCube(lx, y, lz, state, props, vis, 1.0f); return;
            case Shape::Fluid: addFluid(lx, y, lz, state, props, vis); return;
            case Shape::Cross: addCross(lx, y, lz, props, vis); return;
            case Shape::Box: addBox(lx, y, lz, props, vis); return;
        }
    }

private:
    const ChunkMeshInput& m_in;
    const BlockRegistry& m_reg;
    SectionMeshData& m_out;
    int m_baseY;

    bool shouldDrawFace(BlockStateId self, const BlockVisual& selfVis, int nx, int ny, int nz) const {
        BlockStateId n = m_in.blockAt(nx, ny, nz);
        if (n == UNLOADED_NEIGHBOR) return false;
        const auto& np = m_reg.get(n);
        const BlockVisual& nv = visualFor(n);
        if (nv.shape == Shape::Cube && np.isOpaque) return false;
        if (n == self && selfVis.pass != Pass::Opaque && !idHas(np, "leaves")) {
            return false; // cristal contra cristal, agua contra agua
        }
        if (selfVis.shape == Shape::Fluid && nv.shape == Shape::Fluid && selfVis.pass == nv.pass) {
            return false; // agua quieta contra agua fluyendo
        }
        return true;
    }

    void emitQuad(Pass pass, const float pos[4][3], const float uv[4][2], const float normal[3], const float ao[4],
                  const float sky[4], const float blk[4]) {
        auto& verts = m_out.vertices[static_cast<size_t>(pass)];
        auto& idx = m_out.indices[static_cast<size_t>(pass)];
        uint32_t base = static_cast<uint32_t>(verts.size());
        for (int i = 0; i < 4; ++i) {
            verts.push_back({pos[i][0], pos[i][1], pos[i][2], uv[i][0], uv[i][1], normal[0], normal[1], normal[2],
                             ao[i], sky[i], blk[i]});
        }
        // Girar la diagonal según la oclusión para evitar el artefacto anisótropo
        if (ao[0] + ao[2] >= ao[1] + ao[3]) {
            idx.insert(idx.end(), {base, base + 1, base + 2, base + 2, base + 3, base});
        } else {
            idx.insert(idx.end(), {base + 1, base + 2, base + 3, base + 3, base, base + 1});
        }
    }

    static void tileUV(uint8_t tile, float& u0, float& v0, float& u1, float& v1) {
        u0 = static_cast<float>(tile % 16) * TILE + UV_INSET;
        v0 = static_cast<float>(tile / 16) * TILE + UV_INSET;
        u1 = u0 + TILE - 2 * UV_INSET;
        v1 = v0 + TILE - 2 * UV_INSET;
    }

    // Oclusión ambiental y luz suave por vértice a partir de la celda frente a la cara
    void sampleVertex(int bx, int by, int bz, int face, const int corner[3], float& ao, float& sky, float& blk) const {
        const FaceDef& f = FACES[face];
        int px = bx + f.normal[0], py = by + f.normal[1], pz = bz + f.normal[2];
        int axis = f.normal[0] != 0 ? 0 : (f.normal[1] != 0 ? 1 : 2);
        int t1 = (axis + 1) % 3, t2 = (axis + 2) % 3;
        int d1[3] = {0, 0, 0}, d2[3] = {0, 0, 0};
        d1[t1] = corner[t1] ? 1 : -1;
        d2[t2] = corner[t2] ? 1 : -1;

        bool s1 = isOpaqueAt(px + d1[0], py + d1[1], pz + d1[2]);
        bool s2 = isOpaqueAt(px + d2[0], py + d2[1], pz + d2[2]);
        bool c = isOpaqueAt(px + d1[0] + d2[0], py + d1[1] + d2[1], pz + d1[2] + d2[2]);
        int occ = (s1 && s2) ? 0 : 3 - (static_cast<int>(s1) + static_cast<int>(s2) + static_cast<int>(c));
        ao = AO_CURVE[occ];

        // Promedio de luz de las celdas transparentes que rodean el vértice
        float skySum = m_in.skyAt(px, py, pz);
        float blkSum = m_in.blockLightAt(px, py, pz);
        int count = 1;
        auto addSample = [&](bool opaque, int x, int y, int z) {
            if (opaque) return;
            skySum += m_in.skyAt(x, y, z);
            blkSum += m_in.blockLightAt(x, y, z);
            ++count;
        };
        addSample(s1, px + d1[0], py + d1[1], pz + d1[2]);
        addSample(s2, px + d2[0], py + d2[1], pz + d2[2]);
        if (!(s1 && s2)) addSample(c, px + d1[0] + d2[0], py + d1[1] + d2[1], pz + d1[2] + d2[2]);
        sky = skySum / static_cast<float>(count);
        blk = blkSum / static_cast<float>(count);
    }

    void addCube(int lx, int y, int lz, BlockStateId state, const BlockProperties& props, const BlockVisual& vis,
                 float topHeight) {
        float fx = static_cast<float>(lx), fy = static_cast<float>(y - m_baseY), fz = static_cast<float>(lz);
        for (int face = 0; face < 6; ++face) {
            const FaceDef& f = FACES[face];
            if (!shouldDrawFace(state, vis, lx + f.normal[0], y + f.normal[1], lz + f.normal[2])) continue;

            float u0, v0, u1, v1;
            tileUV(props.textureIndices[static_cast<size_t>(face)], u0, v0, u1, v1);
            float pos[4][3], uv[4][2], ao[4], sky[4], blk[4];
            float normal[3] = {static_cast<float>(f.normal[0]), static_cast<float>(f.normal[1]),
                               static_cast<float>(f.normal[2])};
            for (int i = 0; i < 4; ++i) {
                const int* c = f.corners[i];
                pos[i][0] = fx + static_cast<float>(c[0]);
                pos[i][1] = fy + (c[1] ? topHeight : 0.0f);
                pos[i][2] = fz + static_cast<float>(c[2]);
                sampleVertex(lx, y, lz, face, c, ao[i], sky[i], blk[i]);
                ao[i] *= f.shade;
            }
            if (face <= 1) {
                // Caras horizontales: X -> u, Z -> v
                for (int i = 0; i < 4; ++i) {
                    uv[i][0] = f.corners[i][0] ? u1 : u0;
                    uv[i][1] = f.corners[i][2] ? v1 : v0;
                }
            } else {
                // Caras laterales en orden abajo-izq, abajo-der, arriba-der, arriba-izq
                float vTop = v0 + (1.0f - topHeight) * (v1 - v0);
                uv[0][0] = u0;
                uv[0][1] = v1;
                uv[1][0] = u1;
                uv[1][1] = v1;
                uv[2][0] = u1;
                uv[2][1] = vTop;
                uv[3][0] = u0;
                uv[3][1] = vTop;
            }
            emitQuad(vis.pass, pos, uv, normal, ao, sky, blk);
        }
    }

    void addFluid(int lx, int y, int lz, BlockStateId state, const BlockProperties& props, const BlockVisual& vis) {
        // Superficie rebajada salvo que haya fluido encima
        BlockStateId above = m_in.blockAt(lx, y + 1, lz);
        bool fluidAbove = above != UNLOADED_NEIGHBOR && visualFor(above).shape == Shape::Fluid;
        addCube(lx, y, lz, state, props, vis, fluidAbove ? 1.0f : 0.875f);
    }

    void addCross(int lx, int y, int lz, const BlockProperties& props, const BlockVisual& vis) {
        float fx = static_cast<float>(lx), fy = static_cast<float>(y - m_baseY), fz = static_cast<float>(lz);
        float u0, v0, u1, v1;
        tileUV(props.textureIndices[2], u0, v0, u1, v1);
        float sky = m_in.skyAt(lx, y, lz), blk = m_in.blockLightAt(lx, y, lz);
        float skyA[4] = {sky, sky, sky, sky}, blkA[4] = {blk, blk, blk, blk};
        float ao[4] = {0.9f, 0.9f, 0.9f, 0.9f};
        float normal[3] = {0.0f, 1.0f, 0.0f};
        float uv[4][2] = {{u0, v1}, {u1, v1}, {u1, v0}, {u0, v0}};
        const float d = 0.1464f; // planos diagonales con un margen como en el juego original
        const float planes[2][2][2] = {{{d, d}, {1 - d, 1 - d}}, {{d, 1 - d}, {1 - d, d}}};
        for (const auto& pl : planes) {
            float a[4][3] = {{fx + pl[0][0], fy, fz + pl[0][1]},
                             {fx + pl[1][0], fy, fz + pl[1][1]},
                             {fx + pl[1][0], fy + 1, fz + pl[1][1]},
                             {fx + pl[0][0], fy + 1, fz + pl[0][1]}};
            emitQuad(vis.pass, a, uv, normal, ao, skyA, blkA);
            // Cara trasera (orden invertido) para que se vea desde ambos lados con descarte activo
            float b[4][3] = {{a[1][0], a[1][1], a[1][2]},
                             {a[0][0], a[0][1], a[0][2]},
                             {a[3][0], a[3][1], a[3][2]},
                             {a[2][0], a[2][1], a[2][2]}};
            emitQuad(vis.pass, b, uv, normal, ao, skyA, blkA);
        }
    }

    void addBox(int lx, int y, int lz, const BlockProperties& props, const BlockVisual& vis) {
        float fx = static_cast<float>(lx), fy = static_cast<float>(y - m_baseY), fz = static_cast<float>(lz);
        float mn[3] = {vis.minX / 16.0f, vis.minY / 16.0f, vis.minZ / 16.0f};
        float mx[3] = {vis.maxX / 16.0f, vis.maxY / 16.0f, vis.maxZ / 16.0f};
        float sky = m_in.skyAt(lx, y, lz), blk = m_in.blockLightAt(lx, y, lz);
        float skyA[4] = {sky, sky, sky, sky}, blkA[4] = {blk, blk, blk, blk};
        for (int face = 0; face < 6; ++face) {
            const FaceDef& f = FACES[face];
            // Una cara a ras del bloque solo se omite si la tapa un vecino opaco
            bool flush = (f.normal[0] < 0 && mn[0] == 0.0f) || (f.normal[0] > 0 && mx[0] == 1.0f) ||
                         (f.normal[1] < 0 && mn[1] == 0.0f) || (f.normal[1] > 0 && mx[1] == 1.0f) ||
                         (f.normal[2] < 0 && mn[2] == 0.0f) || (f.normal[2] > 0 && mx[2] == 1.0f);
            if (flush && isOpaqueAt(lx + f.normal[0], y + f.normal[1], lz + f.normal[2])) continue;

            float u0, v0, u1, v1;
            tileUV(props.textureIndices[static_cast<size_t>(face)], u0, v0, u1, v1);
            float pos[4][3], uv[4][2];
            float ao[4] = {f.shade, f.shade, f.shade, f.shade};
            float normal[3] = {static_cast<float>(f.normal[0]), static_cast<float>(f.normal[1]),
                               static_cast<float>(f.normal[2])};
            for (int i = 0; i < 4; ++i) {
                const int* c = f.corners[i];
                float cx = c[0] ? mx[0] : mn[0], cy = c[1] ? mx[1] : mn[1], cz = c[2] ? mx[2] : mn[2];
                pos[i][0] = fx + cx;
                pos[i][1] = fy + cy;
                pos[i][2] = fz + cz;
                // Recortar la textura a la porción visible de la caja
                float su, sv;
                if (face <= 1) {
                    su = cx;
                    sv = cz;
                } else if (face <= 3) {
                    su = (face == 2) ? 1.0f - cx : cx;
                    sv = 1.0f - cy;
                } else {
                    su = (face == 5) ? 1.0f - cz : cz;
                    sv = 1.0f - cy;
                }
                uv[i][0] = u0 + su * (u1 - u0);
                uv[i][1] = v0 + sv * (v1 - v0);
            }
            emitQuad(vis.pass, pos, uv, normal, ao, skyA, blkA);
        }
    }
};

} // namespace

void build(const ChunkMeshInput& in, ChunkMeshBuildResult& result) {
    result.chunkX = in.chunkX;
    result.chunkZ = in.chunkZ;
    result.meshVersion = in.meshVersion;
    result.sections.clear();

    const auto& reg = BlockRegistry::global();
    for (int secIdx = 0; secIdx < Chunk::SECTION_COUNT; ++secIdx) {
        int secY = Chunk::indexToSectionY(secIdx);
        SectionMeshData sec;
        sec.sectionY = secY;
        if (in.sectionHasBlocks[static_cast<size_t>(secIdx)]) {
            int baseY = secY * Chunk::SECTION_SIZE;
            SectionBuilder builder(in, reg, sec, baseY);
            for (int y = baseY; y < baseY + Chunk::SECTION_SIZE; ++y) {
                for (int lz = 0; lz < Chunk::CHUNK_DEPTH; ++lz) {
                    for (int lx = 0; lx < Chunk::CHUNK_WIDTH; ++lx) {
                        BlockStateId st = in.blockAt(lx, y, lz);
                        if (st == AIR_BLOCK_STATE || st == UNLOADED_NEIGHBOR) continue;
                        builder.addBlock(lx, y, lz, st);
                    }
                }
            }
        }
        // También se emiten secciones vacías para que el renderer libere mallas obsoletas
        result.sections.push_back(std::move(sec));
    }
}

} // namespace ChunkMesher

} // namespace OpenCraft
