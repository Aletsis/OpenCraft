#include "render/EntityRenderer.hpp"
#include "render/AtlasPainter.hpp"
#include "item/Item.hpp"
#include "sim/MobEntity.hpp"
#include "world/BlockRegistry.hpp"
#include "core/Log.hpp"
#include <algorithm>
#include <cmath>

namespace OpenCraft {

namespace {

constexpr float TILE = 1.0f / 16.0f;

void tileUV(int tile, float& u0, float& v0, float& u1, float& v1) {
    u0 = static_cast<float>(tile % 16) * TILE + 1.0f / 4096.0f;
    v0 = static_cast<float>(tile / 16) * TILE + 1.0f / 4096.0f;
    u1 = u0 + TILE - 2.0f / 4096.0f;
    v1 = v0 + TILE - 2.0f / 4096.0f;
}

Math::Vec4 col(float r, float g, float b, float a = 1.0f) { return {r, g, b, a}; }

bool isCubeBlockItem(const ItemDefinition& def) {
    if (def.placeableBlockState == AIR_BLOCK_STATE || def.iconTileIndex >= 96) return false;
    const auto& p = BlockRegistry::global().get(def.placeableBlockState);
    static const char* FLAT[] = {"torch", "rail",   "cinnabar_wire", "wheat",  "carrots",       "potatoes",
                                 "fire",  "portal", "lever",         "button", "pressure_plate"};
    for (const char* f : FLAT)
        if (p.namespacedId.find(f) != std::string::npos) return false;
    return true;
}

} // namespace

EntityRenderer::~EntityRenderer() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

bool EntityRenderer::init() {
    if (!m_shader.loadFromFiles("assets/shaders/entity.vert", "assets/shaders/entity.frag")) {
        Log::error("EntityRenderer: no se pudieron cargar los shaders de entidades");
        return false;
    }
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(5 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(9 * sizeof(float)));
    glBindVertexArray(0);
    return true;
}

Math::Vec3 EntityRenderer::transform(const Frame& f, Math::Vec3 p) const {
    // Balanceo alrededor del pivote (eje X local), escala y giro en Y hacia el mundo
    Math::Vec3 q = p - f.pivot;
    float cp = std::cos(f.pitchRad), sp = std::sin(f.pitchRad);
    q = Math::Vec3(q.x, q.y * cp - q.z * sp, q.y * sp + q.z * cp);
    q = (q + f.pivot) * f.scale;
    // Ejes locales dextrógiros (x, y, z) = (lateral, arriba, delante) con delante = (cos yaw, sin yaw):
    // una base levógira invertiría el sentido de giro de las caras y el descarte las eliminaría
    float cy = std::cos(f.yawRad), sy = std::sin(f.yawRad);
    Math::Vec3 forward(cy, 0.0f, sy), side(sy, 0.0f, -cy);
    return f.origin + side * q.x + Math::Vec3(0.0f, q.y, 0.0f) + forward * q.z;
}

void EntityRenderer::box(const Frame& f, Math::Vec3 mn, Math::Vec3 mx, Math::Vec4 c, int tile) {
    float u0, v0, u1, v1;
    tileUV(tile < 0 ? AtlasPainter::TILE_WHITE : tile, u0, v0, u1, v1);
    // Mismo orden de esquinas que el mallador de terreno (antihorario desde fuera)
    static const int CORNERS[6][4][3] = {
        {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
        {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}, {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
        {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}, {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}};
    static const float SHADE[6] = {1.0f, 0.5f, 0.8f, 0.8f, 0.6f, 0.6f};
    const float uv[4][2] = {{u0, v1}, {u1, v1}, {u1, v0}, {u0, v0}};
    for (int face = 0; face < 6; ++face) {
        Vertex v[4];
        for (int i = 0; i < 4; ++i) {
            const int* k = CORNERS[face][i];
            Math::Vec3 p(k[0] ? mx.x : mn.x, k[1] ? mx.y : mn.y, k[2] ? mx.z : mn.z);
            Math::Vec3 w = transform(f, p);
            float s = SHADE[face];
            v[i] = {w.x, w.y, w.z, uv[i][0], uv[i][1], c.x * s, c.y * s, c.z * s, c.w, m_sky, m_block};
        }
        m_tris.insert(m_tris.end(), {v[0], v[1], v[2], v[2], v[3], v[0]});
    }
}

void EntityRenderer::texturedCube(const Frame& f, Math::Vec3 mn, Math::Vec3 mx, const std::array<uint8_t, 6>& tiles,
                                  Math::Vec4 tint) {
    static const int CORNERS[6][4][3] = {
        {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}},
        {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}, {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
        {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}, {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}};
    static const float SHADE[6] = {1.0f, 0.5f, 0.8f, 0.8f, 0.6f, 0.6f};
    for (int face = 0; face < 6; ++face) {
        float u0, v0, u1, v1;
        tileUV(tiles[static_cast<size_t>(face)], u0, v0, u1, v1);
        const float uv[4][2] = {{u0, v1}, {u1, v1}, {u1, v0}, {u0, v0}};
        Vertex v[4];
        for (int i = 0; i < 4; ++i) {
            const int* k = CORNERS[face][i];
            Math::Vec3 p(k[0] ? mx.x : mn.x, k[1] ? mx.y : mn.y, k[2] ? mx.z : mn.z);
            Math::Vec3 w = transform(f, p);
            float s = SHADE[face];
            v[i] = {w.x, w.y, w.z, uv[i][0], uv[i][1], tint.x * s, tint.y * s, tint.z * s, tint.w, m_sky, m_block};
        }
        m_tris.insert(m_tris.end(), {v[0], v[1], v[2], v[2], v[3], v[0]});
    }
}

void EntityRenderer::spriteQuad(const Frame& f, float size, int tile, Math::Vec4 tint) {
    float u0, v0, u1, v1;
    tileUV(tile, u0, v0, u1, v1);
    float h = size * 0.5f;
    Math::Vec3 p[4] = {transform(f, {-h, 0, 0}), transform(f, {h, 0, 0}), transform(f, {h, size, 0}),
                       transform(f, {-h, size, 0})};
    const float uv[4][2] = {{u0, v1}, {u1, v1}, {u1, v0}, {u0, v0}};
    Vertex v[4];
    for (int i = 0; i < 4; ++i) {
        v[i] = {p[i].x, p[i].y, p[i].z, uv[i][0], uv[i][1], tint.x, tint.y, tint.z, tint.w, m_sky, m_block};
    }
    // Doble cara para que el sprite se vea al girar
    m_tris.insert(m_tris.end(), {v[0], v[1], v[2], v[2], v[3], v[0], v[0], v[3], v[2], v[2], v[1], v[0]});
}

void EntityRenderer::itemModel(const Frame& f, uint16_t itemId, float size) {
    const auto& def = ItemRegistry::global().get(itemId);
    if (isCubeBlockItem(def)) {
        const auto& p = BlockRegistry::global().get(def.placeableBlockState);
        float h = size * 0.5f;
        texturedCube(f, {-h, 0, -h}, {h, size, h}, p.textureIndices, col(1, 1, 1));
    } else {
        int tile = def.iconTileIndex;
        if (def.placeableBlockState != AIR_BLOCK_STATE && tile < 96) {
            tile = BlockRegistry::global().get(def.placeableBlockState).textureIndices[2];
        }
        spriteQuad(f, size * 1.6f, tile, col(1, 1, 1));
    }
}

void EntityRenderer::humanoid(const Frame& base, float walk, Math::Vec4 skin, Math::Vec4 shirt, Math::Vec4 pants,
                              bool armsForward, float limbW) {
    float swing = std::sin(walk) * 0.7f;
    Frame legL = base, legR = base;
    legL.pivot = legR.pivot = {0, 0.75f, 0};
    legL.pitchRad = swing;
    legR.pitchRad = -swing;
    box(legL, {-limbW * 2, 0, -limbW}, {0, 0.75f, limbW}, pants);
    box(legR, {0, 0, -limbW}, {limbW * 2, 0.75f, limbW}, pants);
    box(base, {-0.25f, 0.75f, -0.125f}, {0.25f, 1.5f, 0.125f}, shirt);
    Frame armL = base, armR = base;
    armL.pivot = armR.pivot = {0, 1.375f, 0};
    float armBase = armsForward ? -1.45f : 0.0f;
    armL.pitchRad = armBase - swing * (armsForward ? 0.2f : 1.0f);
    armR.pitchRad = armBase + swing * (armsForward ? 0.2f : 1.0f);
    box(armL, {-0.25f - limbW * 2, 0.75f, -limbW}, {-0.25f, 1.5f, limbW}, armsForward ? skin : shirt);
    box(armR, {0.25f, 0.75f, -limbW}, {0.25f + limbW * 2, 1.5f, limbW}, armsForward ? skin : shirt);
    box(base, {-0.25f, 1.5f, -0.25f}, {0.25f, 2.0f, 0.25f}, skin);
    // Ojos en la cara frontal (+z local)
    box(base, {-0.19f, 1.72f, 0.25f}, {-0.07f, 1.78f, 0.26f}, col(0.1f, 0.1f, 0.15f));
    box(base, {0.07f, 1.72f, 0.25f}, {0.19f, 1.78f, 0.26f}, col(0.1f, 0.1f, 0.15f));
}

void EntityRenderer::mobModel(const WorldSnapshot::MobSnapshot& mob, float alpha, double t) {
    Math::Vec3 pos = mob.transform.getInterpolatedPosition(alpha);
    Math::Vec3 delta = mob.transform.currentPosition - mob.transform.prevPosition;
    float speed = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    float walk = static_cast<float>(t * 9.0) * std::min(1.0f, speed * 8.0f);
    Frame f;
    f.origin = pos;
    // La IA de criaturas usa yaw = atan2(dx, dz) (delante = (sin, cos)); el render, delante = (cos, sin)
    f.yawRad = Math::radians(90.0f - mob.transform.getInterpolatedYaw(alpha));

    auto quadruped = [&](Math::Vec4 body, Math::Vec4 head, Math::Vec4 legs, float legH, float bodyW, float len) {
        float swing = std::sin(walk) * 0.6f;
        for (int i = 0; i < 4; ++i) {
            Frame lf = f;
            float lx = (i % 2 == 0) ? -bodyW + 0.12f : bodyW - 0.12f;
            float lz = (i < 2) ? len * 0.38f : -len * 0.38f;
            lf.pivot = {lx, legH, lz};
            lf.pitchRad = ((i == 0 || i == 3) ? swing : -swing);
            box(lf, {lx - 0.12f, 0, lz - 0.12f}, {lx + 0.12f, legH, lz + 0.12f}, legs);
        }
        box(f, {-bodyW, legH, -len * 0.5f}, {bodyW, legH + 0.55f, len * 0.5f}, body);
        box(f, {-0.22f, legH + 0.25f, len * 0.5f - 0.05f}, {0.22f, legH + 0.75f, len * 0.5f + 0.38f}, head);
        box(f, {-0.15f, legH + 0.55f, len * 0.5f + 0.38f}, {-0.05f, legH + 0.62f, len * 0.5f + 0.39f},
            col(0.1f, 0.1f, 0.1f));
        box(f, {0.05f, legH + 0.55f, len * 0.5f + 0.38f}, {0.15f, legH + 0.62f, len * 0.5f + 0.39f},
            col(0.1f, 0.1f, 0.1f));
    };

    switch (static_cast<MobType>(mob.mobType)) {
        case MobType::Zombie:
            humanoid(f, walk, col(0.33f, 0.55f, 0.32f), col(0.0f, 0.55f, 0.6f), col(0.25f, 0.22f, 0.55f), true, 0.125f);
            break;
        case MobType::Skeleton:
            humanoid(f, walk, col(0.82f, 0.82f, 0.8f), col(0.75f, 0.75f, 0.73f), col(0.7f, 0.7f, 0.68f), true, 0.06f);
            break;
        case MobType::SwampSkeleton:
            humanoid(f, walk, col(0.6f, 0.68f, 0.5f), col(0.45f, 0.55f, 0.35f), col(0.4f, 0.5f, 0.3f), true, 0.06f);
            break;
        case MobType::Villager: {
            humanoid(f, walk * 0.5f, col(0.75f, 0.58f, 0.45f), col(0.45f, 0.3f, 0.2f), col(0.4f, 0.27f, 0.18f), false,
                     0.125f);
            box(f, {-0.05f, 1.55f, 0.25f}, {0.05f, 1.72f, 0.36f}, col(0.7f, 0.52f, 0.4f));  // nariz
            box(f, {-0.26f, 0.3f, -0.14f}, {0.26f, 1.5f, 0.14f}, col(0.42f, 0.28f, 0.18f)); // túnica
            break;
        }
        case MobType::Lurker: {
            // Acechador: criatura baja de piedra cubierta de musgo que avanza a ras de suelo con seis
            // patas cortas; un único ojo ámbar y, al prepararse para estallar, grietas incandescentes
            float pulse = mob.isIgnited ? 0.5f + 0.5f * static_cast<float>(std::sin(t * 18.0)) : 0.0f;
            auto heat = [&](Math::Vec4 c) {
                return col(c.x + (1.0f - c.x) * pulse * 0.8f, c.y + (0.45f - c.y) * pulse * 0.8f,
                           c.z + (0.1f - c.z) * pulse * 0.8f);
            };
            Frame body = f;
            body.scale = 1.0f + 0.08f * pulse;
            Math::Vec4 stone = heat(col(0.33f, 0.34f, 0.31f));
            Math::Vec4 moss = heat(col(0.27f, 0.40f, 0.22f));
            Math::Vec4 ridge = heat(col(0.46f, 0.45f, 0.41f));
            Math::Vec4 dark = col(0.19f, 0.19f, 0.17f);
            float swing = std::sin(walk * 1.6f) * 0.45f;
            for (int i = 0; i < 6; ++i) {
                Frame lf = body;
                float lx = (i % 2 == 0) ? -0.40f : 0.40f;
                float lz = -0.3f + 0.3f * static_cast<float>(i / 2);
                lf.pivot = {lx, 0.28f, lz};
                lf.pitchRad = ((i / 2 + i % 2) % 2 == 0) ? swing : -swing;
                box(lf, {lx - 0.08f, 0.0f, lz - 0.07f}, {lx + 0.08f, 0.3f, lz + 0.07f}, dark);
            }
            box(body, {-0.42f, 0.25f, -0.45f}, {0.42f, 0.7f, 0.45f}, stone); // caparazón
            box(body, {-0.36f, 0.7f, -0.38f}, {0.36f, 0.8f, 0.30f}, moss);   // musgo dorsal
            for (int i = 0; i < 3; ++i) {                                    // crestas de roca
                float z = -0.30f + 0.25f * static_cast<float>(i);
                box(body, {-0.07f, 0.8f, z}, {0.07f, 0.92f, z + 0.14f}, ridge);
            }
            box(body, {-0.24f, 0.22f, 0.45f}, {0.24f, 0.52f, 0.68f}, heat(col(0.28f, 0.29f, 0.27f))); // cabeza baja
            float savedSky = m_sky, savedBlock = m_block;
            m_block = 15.0f; // el ojo brilla en la oscuridad
            box(body, {-0.15f, 0.38f, 0.68f}, {0.15f, 0.44f, 0.69f}, col(1.0f, 0.68f, 0.18f));
            m_sky = savedSky;
            m_block = savedBlock;
            break;
        }
        case MobType::Cow:
            quadruped(col(0.35f, 0.24f, 0.17f), col(0.4f, 0.3f, 0.22f), col(0.3f, 0.22f, 0.16f), 0.75f, 0.35f, 1.1f);
            box(f, {-0.2f, 1.05f, -0.2f}, {0.2f, 1.31f, 0.3f}, col(0.92f, 0.92f, 0.9f)); // mancha
            break;
        case MobType::Pig:
            quadruped(col(0.95f, 0.66f, 0.66f), col(0.95f, 0.66f, 0.66f), col(0.9f, 0.6f, 0.6f), 0.4f, 0.32f, 0.9f);
            box(f, {-0.1f, 0.7f, 0.83f}, {0.1f, 0.85f, 0.9f}, col(0.85f, 0.5f, 0.5f)); // hocico
            break;
        case MobType::Sheep:
            quadruped(col(0.92f, 0.92f, 0.9f), col(0.75f, 0.68f, 0.62f), col(0.75f, 0.68f, 0.62f), 0.6f, 0.38f, 1.0f);
            break;
        case MobType::Wolf:
            quadruped(col(0.85f, 0.83f, 0.8f), col(0.82f, 0.8f, 0.78f), col(0.8f, 0.78f, 0.75f), 0.45f, 0.22f, 0.85f);
            break;
        case MobType::Chicken: {
            float swing = std::sin(walk) * 0.6f;
            Frame l1 = f, l2 = f;
            l1.pivot = l2.pivot = {0, 0.3f, 0};
            l1.pitchRad = swing;
            l2.pitchRad = -swing;
            box(l1, {-0.12f, 0, -0.03f}, {-0.06f, 0.3f, 0.03f}, col(0.95f, 0.75f, 0.2f));
            box(l2, {0.06f, 0, -0.03f}, {0.12f, 0.3f, 0.03f}, col(0.95f, 0.75f, 0.2f));
            box(f, {-0.19f, 0.3f, -0.25f}, {0.19f, 0.62f, 0.2f}, col(0.95f, 0.95f, 0.95f));
            box(f, {-0.13f, 0.55f, 0.12f}, {0.13f, 0.9f, 0.3f}, col(0.95f, 0.95f, 0.95f));
            box(f, {-0.06f, 0.68f, 0.3f}, {0.06f, 0.75f, 0.4f}, col(0.95f, 0.65f, 0.15f));
            box(f, {-0.04f, 0.6f, 0.3f}, {0.04f, 0.68f, 0.33f}, col(0.85f, 0.1f, 0.1f));
            break;
        }
        case MobType::Whirl: {
            float bob = static_cast<float>(std::sin(t * 3.0)) * 0.08f;
            Frame b = f;
            b.origin.y += bob;
            box(b, {-0.25f, 1.1f, -0.25f}, {0.25f, 1.6f, 0.25f}, col(0.65f, 0.8f, 0.95f));
            Frame r = b;
            r.yawRad += static_cast<float>(t * 4.0);
            for (int i = 0; i < 3; ++i) {
                float a = static_cast<float>(i) * 2.094f;
                float x = std::cos(a) * 0.35f, z = std::sin(a) * 0.35f;
                box(r, {x - 0.05f, 0.3f, z - 0.05f}, {x + 0.05f, 1.0f, z + 0.05f}, col(0.75f, 0.9f, 1.0f, 0.85f));
            }
            break;
        }
        default: box(f, {-0.3f, 0, -0.3f}, {0.3f, 1.8f, 0.3f}, col(0.8f, 0.2f, 0.8f)); break;
    }
}

void EntityRenderer::renderWorldEntities(const WorldSnapshot& snap, float alpha, const World& world,
                                         const TextureAtlas& atlas, const Math::Mat4& view, const Math::Mat4& proj,
                                         const EntityLighting& lighting, double t) {
    m_tris.clear();
    auto lightAt = [&](const Math::Vec3& p) {
        int x = static_cast<int>(std::floor(p.x)), y = static_cast<int>(std::floor(p.y + 0.5f)),
            z = static_cast<int>(std::floor(p.z));
        m_sky = static_cast<float>(world.getSkyLight(x, y, z));
        m_block = static_cast<float>(world.getBlockLight(x, y, z));
        if (!world.getChunk(static_cast<int>(std::floor(p.x / 16.0f)), static_cast<int>(std::floor(p.z / 16.0f))))
            m_sky = 15.0f;
    };

    for (const auto& mob : snap.mobs) {
        if (mob.isDead) continue;
        lightAt(mob.transform.currentPosition);
        mobModel(mob, alpha, t);
    }
    for (const auto& rp : snap.remotePlayers) {
        Math::Vec3 pos = rp.transform.getInterpolatedPosition(alpha);
        lightAt(pos);
        Math::Vec3 d = rp.transform.currentPosition - rp.transform.prevPosition;
        float walk = static_cast<float>(t * 9.0) * std::min(1.0f, std::sqrt(d.x * d.x + d.z * d.z) * 8.0f);
        Frame f;
        f.origin = pos;
        f.yawRad = Math::radians(rp.transform.getInterpolatedYaw(alpha));
        humanoid(f, walk, col(0.78f, 0.58f, 0.45f), col(0.0f, 0.68f, 0.68f), col(0.25f, 0.22f, 0.6f), false, 0.125f);
        box(f, {-0.26f, 1.88f, -0.26f}, {0.26f, 2.02f, 0.26f}, col(0.27f, 0.17f, 0.1f)); // pelo
    }
    for (const auto& it : snap.itemEntities) {
        lightAt(it.position);
        Frame f;
        float age = static_cast<float>(it.ageTicks) + alpha;
        f.origin = it.position + Math::Vec3(0.0f, 0.1f + std::sin(age * 0.1f) * 0.06f, 0.0f);
        f.yawRad = age * 0.05f;
        itemModel(f, it.itemId, 0.25f);
        if (it.count > 1) {
            Frame f2 = f;
            f2.origin = f.origin + Math::Vec3(0.06f, 0.05f, 0.06f);
            itemModel(f2, it.itemId, 0.25f);
        }
    }
    flush(atlas, view, proj, lighting, false);
}

void EntityRenderer::renderBlockSelection(const World& world, int bx, int by, int bz, float progress,
                                          const TextureAtlas& atlas, const Math::Mat4& view, const Math::Mat4& proj,
                                          const EntityLighting& lighting) {
    (void)world;
    m_tris.clear();
    m_lines.clear();
    const float e = 0.002f;
    Math::Vec3 mn(static_cast<float>(bx) - e, static_cast<float>(by) - e, static_cast<float>(bz) - e);
    Math::Vec3 mx(static_cast<float>(bx) + 1 + e, static_cast<float>(by) + 1 + e, static_cast<float>(bz) + 1 + e);
    Math::Vec3 c[8] = {{mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mx.x, mn.y, mx.z}, {mn.x, mn.y, mx.z},
                       {mn.x, mx.y, mn.z}, {mx.x, mx.y, mn.z}, {mx.x, mx.y, mx.z}, {mn.x, mx.y, mx.z}};
    static const int EDGES[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                     {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    float u0, v0, u1, v1;
    tileUV(AtlasPainter::TILE_WHITE, u0, v0, u1, v1);
    for (const auto& ed : EDGES) {
        for (int k : ed) {
            m_lines.push_back({c[k].x, c[k].y, c[k].z, u0, v0, 0.0f, 0.0f, 0.0f, 0.55f, 15.0f, 15.0f});
        }
    }
    if (progress > 0.0f) {
        int stage = std::clamp(static_cast<int>(progress * 10.0f), 0, 9);
        Frame f;
        f.origin = Math::Vec3(0, 0, 0);
        m_sky = 15.0f;
        m_block = 15.0f;
        std::array<uint8_t, 6> tiles;
        tiles.fill(static_cast<uint8_t>(AtlasPainter::TILE_CRACK_FIRST + stage));
        texturedCube(f, mn, mx, tiles, col(1, 1, 1, 0.8f));
    }
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    flush(atlas, view, proj, lighting, true);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void EntityRenderer::renderFirstPerson(uint16_t heldItemId, float swing, float bob, float skyLight, float blockLight,
                                       const TextureAtlas& atlas, const Math::Mat4& proj,
                                       const EntityLighting& lighting) {
    m_tris.clear();
    m_lines.clear();
    m_sky = skyLight;
    m_block = blockLight;
    float s = std::sin(swing * 3.14159f);
    float s2 = std::sin(std::sqrt(swing) * 3.14159f);
    Frame f;
    // Espacio de vista: x derecha, y arriba, -z hacia delante; giro de 180° para que +z local mire a cámara
    f.origin = Math::Vec3(0.56f - s2 * 0.2f, -0.55f + std::sin(bob) * 0.02f - s * 0.25f, -0.75f - s2 * 0.15f);
    f.yawRad = Math::radians(90.0f + 45.0f) - s * 0.8f;
    if (heldItemId != EMPTY_ITEM_ID) {
        const auto& def = ItemRegistry::global().get(heldItemId);
        if (isCubeBlockItem(def)) {
            itemModel(f, heldItemId, 0.4f);
        } else {
            f.pitchRad = -0.3f;
            itemModel(f, heldItemId, 0.42f);
        }
    } else {
        f.yawRad = Math::radians(100.0f) - s * 0.5f;
        f.pitchRad = -1.1f + s * 0.6f;
        f.origin = Math::Vec3(0.6f - s2 * 0.15f, -0.6f + std::sin(bob) * 0.02f - s * 0.2f, -0.6f);
        box(f, {-0.09f, -0.5f, -0.09f}, {0.09f, 0.25f, 0.09f}, col(0.78f, 0.58f, 0.45f));
    }
    glClear(GL_DEPTH_BUFFER_BIT);
    flush(atlas, Math::Mat4::identity(), proj, lighting, false);
}

void EntityRenderer::flush(const TextureAtlas& atlas, const Math::Mat4& view, const Math::Mat4& proj,
                           const EntityLighting& l, bool fullBright) {
    if (m_tris.empty() && m_lines.empty()) return;
    m_shader.use();
    m_shader.setMat4("uView", view);
    m_shader.setMat4("uProjection", proj);
    m_shader.setFloat("uSunlightFactor", l.sunlight);
    m_shader.setFloat("uGamma", l.gamma);
    m_shader.setFloat("uFullBright", fullBright ? 1.0f : 0.0f);
    m_shader.setVec3("uFogColor", l.fogColor);
    m_shader.setFloat("uFogStart", l.fogStart);
    m_shader.setFloat("uFogEnd", l.fogEnd);
    m_shader.setInt("uTextureAtlas", 0);
    atlas.bind(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    if (!m_tris.empty()) {
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_tris.size() * sizeof(Vertex)), m_tris.data(),
                     GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_tris.size()));
    }
    if (!m_lines.empty()) {
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_lines.size() * sizeof(Vertex)), m_lines.data(),
                     GL_STREAM_DRAW);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lines.size()));
    }
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    m_tris.clear();
    m_lines.clear();
}

} // namespace OpenCraft
