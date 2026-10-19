#pragma once

#include <string>
#include <GLES3/gl3.h>
#include "math/Math.hpp"

namespace OpenCraft {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSource(const std::string& vertexSource, const std::string& fragmentSource);

    void use() const;
    void unbind() const;

    GLuint getProgramID() const noexcept { return m_program; }

    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const Math::Vec2& value) const;
    void setVec3(const std::string& name, const Math::Vec3& value) const;
    void setVec4(const std::string& name, const Math::Vec4& value) const;
    void setMat4(const std::string& name, const Math::Mat4& value) const;

private:
    GLuint m_program = 0;

    static GLuint compileShader(GLenum type, const std::string& source);
    GLint getUniformLocation(const std::string& name) const;
};

} // namespace OpenCraft
