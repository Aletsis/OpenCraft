#include "render/Shader.hpp"
#include "core/Log.hpp"
#include "platform/PlatformPaths.hpp"

#include <fstream>
#include <sstream>
#include <vector>

namespace OpenCraft {

Shader::~Shader() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

Shader::Shader(Shader&& other) noexcept : m_program(other.m_program) { other.m_program = 0; }

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_program) {
            glDeleteProgram(m_program);
        }
        m_program = other.m_program;
        other.m_program = 0;
    }
    return *this;
}

static std::string readFile(const std::string& filepath) {
    std::filesystem::path resolved = PlatformPaths::resolveAsset(filepath);
    std::ifstream file(resolved);
    if (!file.is_open()) {
        Log::error("No se pudo abrir el archivo de shader: {} (resuelto como {}). "
                   "Defina OPENCRAFT_ASSETS_DIR o instale assets/ junto al ejecutable",
                   filepath, resolved.string());
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vSource = readFile(vertexPath);
    std::string fSource = readFile(fragmentPath);

    if (vSource.empty() || fSource.empty()) {
        return false;
    }

    return loadFromSource(vSource, fSource);
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* srcPtr = source.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> infoLog(logLength + 1);
        glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());

        const char* typeStr = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        Log::error("Error de compilación en {} Shader:\n{}", typeStr, infoLog.data());

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool Shader::loadFromSource(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (!vShader) return false;

    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!fShader) {
        glDeleteShader(vShader);
        return false;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> infoLog(logLength + 1);
        glGetProgramInfoLog(program, logLength, nullptr, infoLog.data());
        Log::error("Error de enlace (link) del programa Shader:\n{}", infoLog.data());

        glDeleteShader(vShader);
        glDeleteShader(fShader);
        glDeleteProgram(program);
        return false;
    }

    glDetachShader(program, vShader);
    glDetachShader(program, fShader);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    if (m_program) {
        glDeleteProgram(m_program);
    }
    m_program = program;
    return true;
}

void Shader::use() const {
    if (m_program) {
        glUseProgram(m_program);
    }
}

void Shader::unbind() const { glUseProgram(0); }

GLint Shader::getUniformLocation(const std::string& name) const {
    return glGetUniformLocation(m_program, name.c_str());
}

void Shader::setInt(const std::string& name, int value) const { glUniform1i(getUniformLocation(name), value); }

void Shader::setFloat(const std::string& name, float value) const { glUniform1f(getUniformLocation(name), value); }

void Shader::setVec2(const std::string& name, const Math::Vec2& value) const {
    glUniform2f(getUniformLocation(name), value.x, value.y);
}

void Shader::setVec3(const std::string& name, const Math::Vec3& value) const {
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setVec4(const std::string& name, const Math::Vec4& value) const {
    glUniform4f(getUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::setMat4(const std::string& name, const Math::Mat4& value) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, value.data());
}

} // namespace OpenCraft
