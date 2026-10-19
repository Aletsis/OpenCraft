#pragma once

#include <cstdint>
#include <vector>
#include "render/GL.hpp"

namespace OpenCraft {

class TextureAtlas {
public:
    TextureAtlas();
    ~TextureAtlas();

    TextureAtlas(const TextureAtlas&) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;

    void generateProceduralAtlas();
    void bind(GLuint slot = 0) const;
    void unbind() const;

    GLuint getID() const noexcept { return m_textureID; }

private:
    GLuint m_textureID = 0;
};

} // namespace OpenCraft
