#pragma once

#include <string_view>
#include <vector>
#include <GLES3/gl3.h>
#include "render/Shader.hpp"
#include "render/TextureAtlas.hpp"

namespace OpenCraft {

struct UIColor {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

namespace UIColors {
constexpr UIColor White{1.0f, 1.0f, 1.0f, 1.0f};
constexpr UIColor Black{0.0f, 0.0f, 0.0f, 1.0f};
constexpr UIColor Gray{0.63f, 0.63f, 0.63f, 1.0f};
constexpr UIColor Yellow{1.0f, 1.0f, 0.33f, 1.0f};
constexpr UIColor Red{1.0f, 0.33f, 0.33f, 1.0f};
} // namespace UIColors

// Lote de primitivas 2D en coordenadas de pantalla (origen arriba a la izquierda).
// Conserva el orden de dibujo: se agrupan los tramos consecutivos del mismo tipo.
class UIBatch {
public:
    UIBatch() = default;
    ~UIBatch();

    UIBatch(const UIBatch&) = delete;
    UIBatch& operator=(const UIBatch&) = delete;

    bool init();

    void begin(int screenWidth, int screenHeight);
    void rect(float x, float y, float w, float h, UIColor c);
    void outline(float x, float y, float w, float h, float thickness, UIColor c);
    void image(float x, float y, float w, float h, float u0, float v0, float u1, float v1,
               UIColor tint = UIColors::White);
    // Cuadrilátero arbitrario texturizado (p. ej. caras isométricas de un bloque)
    void imageQuad(const float pts[4][2], const float uvs[4][2], UIColor tint);
    // Ícono de la rejilla 16x16 del atlas
    void atlasTile(float x, float y, float size, int tileIndex, UIColor tint = UIColors::White);
    void text(std::string_view text, float x, float y, float scale, UIColor c, bool shadow = true);
    void textCentered(std::string_view text, float centerX, float y, float scale, UIColor c, bool shadow = true);
    static float textWidth(std::string_view text, float scale);
    void end(const TextureAtlas* atlas);

    int width() const noexcept { return m_width; }
    int height() const noexcept { return m_height; }

private:
    struct Vertex {
        float x, y;
        float u, v;
        float r, g, b, a;
    };
    struct Span {
        bool textured;
        size_t first;
        size_t count;
    };

    Shader m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    int m_width = 0;
    int m_height = 0;
    std::vector<Vertex> m_vertices;
    std::vector<Span> m_spans;

    void pushQuad(bool textured, float x, float y, float w, float h, float u0, float v0, float u1, float v1, UIColor c);
};

} // namespace OpenCraft
