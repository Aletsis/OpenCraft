#include "render/TextureAtlas.hpp"
#include "render/AtlasPainter.hpp"
#include "core/Log.hpp"
#include <cmath>
#include <algorithm>

namespace OpenCraft {

TextureAtlas::TextureAtlas() = default;

TextureAtlas::~TextureAtlas() {
    if (m_textureID) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
}

void TextureAtlas::generateProceduralAtlas() {
    constexpr int ATLAS_WIDTH = AtlasPainter::ATLAS_SIZE;
    constexpr int ATLAS_HEIGHT = AtlasPainter::ATLAS_SIZE;
    std::vector<uint8_t> pixels = AtlasPainter::paint();

    if (m_textureID) {
        glDeleteTextures(1, &m_textureID);
    }

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    // Pixel art filter: Nearest neighbor (crucial for the classic voxel pixel-art aesthetic and mobile performance)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_WIDTH, ATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    Log::info("TextureAtlas procedural generado ({}x{} pixels, 16x16 slots)", ATLAS_WIDTH, ATLAS_HEIGHT);
}

void TextureAtlas::bind(GLuint slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

void TextureAtlas::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }

} // namespace OpenCraft
