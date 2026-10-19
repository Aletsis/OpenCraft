#include "ui/UIBatch.hpp"
#include "ui/BitmapFont.hpp"
#include "core/Log.hpp"
#include <cmath>

namespace OpenCraft {

UIBatch::~UIBatch() {
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

bool UIBatch::init() {
    if (!m_shader.loadFromFiles("assets/shaders/ui.vert", "assets/shaders/ui.frag")) {
        Log::error("UIBatch: no se pudieron cargar los shaders de interfaz");
        return false;
    }
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(4 * sizeof(float)));
    glBindVertexArray(0);
    return true;
}

void UIBatch::begin(int screenWidth, int screenHeight) {
    m_width = screenWidth;
    m_height = screenHeight;
    m_vertices.clear();
    m_spans.clear();
}

void UIBatch::pushQuad(bool textured, float x, float y, float w, float h, float u0, float v0, float u1, float v1,
                       UIColor c) {
    if (m_spans.empty() || m_spans.back().textured != textured) {
        m_spans.push_back({textured, m_vertices.size(), 0});
    }
    m_vertices.push_back({x, y, u0, v0, c.r, c.g, c.b, c.a});
    m_vertices.push_back({x + w, y, u1, v0, c.r, c.g, c.b, c.a});
    m_vertices.push_back({x + w, y + h, u1, v1, c.r, c.g, c.b, c.a});
    m_vertices.push_back({x, y, u0, v0, c.r, c.g, c.b, c.a});
    m_vertices.push_back({x + w, y + h, u1, v1, c.r, c.g, c.b, c.a});
    m_vertices.push_back({x, y + h, u0, v1, c.r, c.g, c.b, c.a});
    m_spans.back().count += 6;
}

void UIBatch::rect(float x, float y, float w, float h, UIColor c) { pushQuad(false, x, y, w, h, 0, 0, 0, 0, c); }

void UIBatch::outline(float x, float y, float w, float h, float t, UIColor c) {
    rect(x, y, w, t, c);
    rect(x, y + h - t, w, t, c);
    rect(x, y + t, t, h - 2 * t, c);
    rect(x + w - t, y + t, t, h - 2 * t, c);
}

void UIBatch::image(float x, float y, float w, float h, float u0, float v0, float u1, float v1, UIColor tint) {
    pushQuad(true, x, y, w, h, u0, v0, u1, v1, tint);
}

void UIBatch::imageQuad(const float pts[4][2], const float uvs[4][2], UIColor c) {
    if (m_spans.empty() || !m_spans.back().textured) {
        m_spans.push_back({true, m_vertices.size(), 0});
    }
    const int order[6] = {0, 1, 2, 0, 2, 3};
    for (int i : order) {
        m_vertices.push_back({pts[i][0], pts[i][1], uvs[i][0], uvs[i][1], c.r, c.g, c.b, c.a});
    }
    m_spans.back().count += 6;
}

void UIBatch::atlasTile(float x, float y, float size, int tileIndex, UIColor tint) {
    constexpr float tile = 1.0f / 16.0f;
    float u0 = static_cast<float>(tileIndex % 16) * tile;
    float v0 = static_cast<float>(tileIndex / 16) * tile;
    image(x, y, size, size, u0, v0, u0 + tile, v0 + tile, tint);
}

void UIBatch::text(std::string_view str, float x, float y, float scale, UIColor c, bool shadow) {
    // Alinear a píxel para que la fuente no se vea borrosa
    x = std::round(x);
    y = std::round(y);
    if (shadow) {
        UIColor sc{c.r * 0.25f, c.g * 0.25f, c.b * 0.25f, c.a};
        text(str, x + scale, y + scale, scale, sc, false);
    }
    float penX = x;
    for (char32_t cp : BitmapFont::decodeUtf8(str)) {
        const Glyph& g = BitmapFont::glyphFor(cp);
        for (int row = 0; row < BitmapFont::GLYPH_HEIGHT; ++row) {
            uint8_t bits = g.rows[row];
            int col = 0;
            while (col < 5) {
                if (!(bits & (1u << (4 - col)))) {
                    ++col;
                    continue;
                }
                // Fusionar píxeles horizontales consecutivos en un único quad
                int start = col;
                while (col < 5 && (bits & (1u << (4 - col)))) ++col;
                rect(penX + start * scale, y + row * scale, (col - start) * scale, scale, c);
            }
        }
        penX += g.advance * scale;
    }
}

void UIBatch::textCentered(std::string_view str, float centerX, float y, float scale, UIColor c, bool shadow) {
    text(str, centerX - textWidth(str, scale) * 0.5f, y, scale, c, shadow);
}

float UIBatch::textWidth(std::string_view str, float scale) {
    return static_cast<float>(BitmapFont::measure(str)) * scale;
}

void UIBatch::end(const TextureAtlas* atlas) {
    if (m_vertices.empty() || m_width <= 0 || m_height <= 0) return;

    Math::Mat4 ortho = Math::Mat4::identity();
    ortho.m[0] = 2.0f / static_cast<float>(m_width);
    ortho.m[5] = -2.0f / static_cast<float>(m_height);
    ortho.m[10] = -1.0f;
    ortho.m[12] = -1.0f;
    ortho.m[13] = 1.0f;

    // La proyección invierte Y y con ello el sentido de giro de los triángulos: sin desactivar el
    // descarte de caras la GPU eliminaría toda la interfaz
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_shader.use();
    m_shader.setMat4("uOrtho", ortho);
    m_shader.setInt("uTexture", 0);
    if (atlas) atlas->bind(0);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)), m_vertices.data(),
                 GL_STREAM_DRAW);
    for (const auto& span : m_spans) {
        if (span.textured && !atlas) continue;
        m_shader.setInt("uUseTexture", span.textured ? 1 : 0);
        glDrawArrays(GL_TRIANGLES, static_cast<GLint>(span.first), static_cast<GLsizei>(span.count));
    }
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    m_vertices.clear();
    m_spans.clear();
}

} // namespace OpenCraft
