#pragma once

#include <vector>
#include "render/GL.hpp"
#include "math/Math.hpp"
#include "render/Shader.hpp"
#include "render/TextureAtlas.hpp"
#include "sim/Snapshot.hpp"
#include "world/World.hpp"

namespace OpenCraft {

struct EntityLighting {
    float sunlight = 1.0f;
    float gamma = 0.5f;
    Math::Vec3 fogColor{0.6f, 0.75f, 0.95f};
    float fogStart = 60.0f;
    float fogEnd = 96.0f;
};

// Geometría de entidades construida en CPU por fotograma: modelos de cajas para criaturas y
// jugadores, objetos tirados, contorno del bloque apuntado y objeto en la mano.
class EntityRenderer {
public:
    EntityRenderer() = default;
    ~EntityRenderer();

    EntityRenderer(const EntityRenderer&) = delete;
    EntityRenderer& operator=(const EntityRenderer&) = delete;

    bool init();

    void renderWorldEntities(const WorldSnapshot& snap, float alpha, const World& world, const TextureAtlas& atlas,
                             const Math::Mat4& view, const Math::Mat4& proj, const EntityLighting& lighting,
                             double timeSeconds);
    void renderBlockSelection(const World& world, int bx, int by, int bz, float miningProgress,
                              const TextureAtlas& atlas, const Math::Mat4& view, const Math::Mat4& proj,
                              const EntityLighting& lighting);
    // Mano u objeto sostenido en primera persona; swing en [0, 1)
    void renderFirstPerson(uint16_t heldItemId, float swing, float bob, float skyLight, float blockLight,
                           const TextureAtlas& atlas, const Math::Mat4& proj, const EntityLighting& lighting);

private:
    struct Vertex {
        float x, y, z;
        float u, v;
        float r, g, b, a;
        float sky, block;
    };

    Shader m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::vector<Vertex> m_tris;
    std::vector<Vertex> m_lines;
    float m_sky = 15.0f;
    float m_block = 0.0f;

    // Transformación local de una pieza: base + rotación en Y (yaw) y pivote de balanceo en X
    struct Frame {
        Math::Vec3 origin;
        float yawRad = 0.0f;
        float pitchRad = 0.0f; // giro de la pieza alrededor de su eje X local
        Math::Vec3 pivot{0, 0, 0};
        float scale = 1.0f;
    };

    void box(const Frame& f, Math::Vec3 mn, Math::Vec3 mx, Math::Vec4 color, int tile = -1);
    void texturedCube(const Frame& f, Math::Vec3 mn, Math::Vec3 mx, const std::array<uint8_t, 6>& tiles,
                      Math::Vec4 tint);
    void spriteQuad(const Frame& f, float size, int tile, Math::Vec4 tint);
    void itemModel(const Frame& f, uint16_t itemId, float size);
    void mobModel(const WorldSnapshot::MobSnapshot& mob, float alpha, double t);
    void humanoid(const Frame& f, float walk, Math::Vec4 skin, Math::Vec4 shirt, Math::Vec4 pants, bool armsForward,
                  float limbW);
    Math::Vec3 transform(const Frame& f, Math::Vec3 p) const;
    void flush(const TextureAtlas& atlas, const Math::Mat4& view, const Math::Mat4& proj, const EntityLighting& l,
               bool fullBright);
};

} // namespace OpenCraft
