#include "core/Window.hpp"
#include "core/Log.hpp"

#if defined(_WIN32)
#include <windows.h>
#endif

#include "render/GL.hpp"

#include <cstdlib>
#include <string_view>
#include <filesystem>
#include <optional>
#include <vector>

namespace OpenCraft {

Window::Window(const WindowConfig& config)
    : m_config(config),
      m_width(config.width),
      m_height(config.height),
      m_fbWidth(config.width),
      m_fbHeight(config.height) {}

Window::~Window() {
    if (m_glContext) {
        SDL_GL_DestroyContext(m_glContext);
        m_glContext = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_Quit();
    Log::info("SDL3 y subsistemas cerrados correctamente");
}

namespace {

#if defined(_WIN32)
constexpr const char* ANGLE_EGL = "libEGL.dll";
constexpr const char* ANGLE_GLES = "libGLESv2.dll";
#elif defined(__APPLE__)
constexpr const char* ANGLE_EGL = "libEGL.dylib";
constexpr const char* ANGLE_GLES = "libGLESv2.dylib";
#else
constexpr const char* ANGLE_EGL = "libEGL.so";
constexpr const char* ANGLE_GLES = "libGLESv2.so";
#endif

// Busca ANGLE en OPENCRAFT_ANGLE_DIR, junto al ejecutable o en ../lib/opencraft/angle (instalación Linux)
std::optional<std::filesystem::path> findAngleDir() {
    std::vector<std::filesystem::path> candidates;
    if (const char* env = std::getenv("OPENCRAFT_ANGLE_DIR")) candidates.emplace_back(env);
    if (const char* base = SDL_GetBasePath()) {
        std::filesystem::path exeDir(base);
        candidates.push_back(exeDir);
        candidates.push_back(exeDir / "angle");
        candidates.push_back(exeDir / ".." / "lib" / "opencraft" / "angle");
    }
    std::error_code ec;
    for (const auto& dir : candidates) {
        if (std::filesystem::exists(dir / ANGLE_EGL, ec) && std::filesystem::exists(dir / ANGLE_GLES, ec)) {
            return dir;
        }
    }
    return std::nullopt;
}

void* sdlProcLoader(const char* name) { return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name)); }

} // namespace

bool Window::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
        Log::error("Fallo al inicializar SDL3: {}", SDL_GetError());
        return false;
    }
    Log::info("SDL3 inicializado exitosamente");

    // Backend de OpenGL ES: "native" usa el controlador del sistema; "angle" traduce a Direct3D, Vulkan o
    // Metal. En modo automático se usa ANGLE en Windows y macOS si viene con el juego, y el nativo en el resto.
    const std::string& requested = m_config.glBackend;
#if defined(_WIN32) || defined(__APPLE__)
    bool preferAngle = requested != "native";
#else
    bool preferAngle = requested == "angle";
#endif
    bool created = false;
    if (preferAngle) {
        if (auto dir = findAngleDir()) {
            SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
            SDL_SetHint(SDL_HINT_EGL_LIBRARY, (*dir / ANGLE_EGL).string().c_str());
#if defined(_WIN32)
            // libEGL.dll carga libGLESv2.dll por nombre: debe poder encontrarla en esta carpeta
            SetDllDirectoryW(dir->wstring().c_str());
#endif
            created = createWindowAndContext((*dir / ANGLE_GLES).string());
            if (!created) {
                Log::warn("ANGLE no pudo crear el contexto ({}); se usa el controlador nativo", SDL_GetError());
                destroyWindowAndContext();
                SDL_ResetHint(SDL_HINT_OPENGL_ES_DRIVER);
                SDL_ResetHint(SDL_HINT_EGL_LIBRARY);
                SDL_ResetHint(SDL_HINT_OPENGL_LIBRARY);
            }
        } else if (requested == "angle") {
            Log::warn("No se encontró ANGLE ({} y {}); se usa el controlador nativo", ANGLE_EGL, ANGLE_GLES);
        }
    }
    if (!created && !createWindowAndContext({})) {
        return false;
    }

    const char* glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glslVer = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    Log::info("Contexto Gráfico Inicializado ({}):", m_glBackendName);
    Log::info("  Proveedor : {}", glVendor ? glVendor : "Desconocido");
    Log::info("  GPU       : {}", glRenderer ? glRenderer : "Desconocido");
    Log::info("  Versión   : {}", glVersion ? glVersion : "Desconocido");
    Log::info("  GLSL ES   : {}", glslVer ? glslVer : "Desconocido");
    Log::info("  Resolución: {}x{} (Framebuffer: {}x{})", m_width, m_height, m_fbWidth, m_fbHeight);

    return true;
}

bool Window::createWindowAndContext(const std::string& angleGlesPath) {
    const bool useAngle = !angleGlesPath.empty();
    // Configurar atributos de contexto OpenGL ES 3.0
    SDL_GL_ResetAttributes();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (m_config.fullscreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }

    m_window = SDL_CreateWindow(m_config.title.c_str(), m_config.width, m_config.height, windowFlags);
    if (!m_window) {
        Log::error("Fallo al crear la ventana SDL3: {}", SDL_GetError());
        return false;
    }

    // SDL lee SDL_HINT_OPENGL_LIBRARY dos veces: al crear la ventana carga con ella GLX/WGL (y fallaría con
    // ANGLE) y al crear el contexto ES la usa como biblioteca GLES de EGL. Por eso se fija justo aquí.
    if (useAngle) SDL_SetHint(SDL_HINT_OPENGL_LIBRARY, angleGlesPath.c_str());
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        Log::error("Fallo al crear el contexto OpenGL ES 3.0: {}", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(m_window, m_glContext);
    const char* missing = nullptr;
    if (!GL::load(sdlProcLoader, &missing)) {
        Log::error("El contexto no ofrece OpenGL ES 3.0 completo (falta {})", missing ? missing : "?");
        return false;
    }
    // El nombre sale del propio contexto: un controlador «nativo» también puede ser ANGLE (p. ej. Wine)
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    bool isAngle = useAngle || (version && std::string_view(version).find("ANGLE") != std::string_view::npos);
    m_glBackendName = isAngle ? "ANGLE" : "nativo";
    SDL_GL_SetSwapInterval(m_config.vsync ? 1 : 0);

    // Obtener dimensiones reales de píxeles (High-DPI / Retina / Pantallas móviles)
    SDL_GetWindowSizeInPixels(m_window, &m_fbWidth, &m_fbHeight);
    SDL_GetWindowSize(m_window, &m_width, &m_height);
    return true;
}

void Window::destroyWindowAndContext() {
    if (m_glContext) {
        SDL_GL_DestroyContext(m_glContext);
        m_glContext = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    SDL_GL_UnloadLibrary();
}

void Window::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_shouldClose = true;
        } else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            m_fbWidth = event.window.data1;
            m_fbHeight = event.window.data2;
            glViewport(0, 0, m_fbWidth, m_fbHeight);
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            m_width = event.window.data1;
            m_height = event.window.data2;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) {
                m_shouldClose = true;
            }
        }
    }
}

void Window::onEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        m_fbWidth = event.window.data1;
        m_fbHeight = event.window.data2;
    } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        m_width = event.window.data1;
        m_height = event.window.data2;
    } else if (event.type == SDL_EVENT_QUIT) {
        m_shouldClose = true;
    }
}

void Window::setFullscreen(bool fullscreen) {
    m_config.fullscreen = fullscreen;
    if (m_window) SDL_SetWindowFullscreen(m_window, fullscreen);
}

void Window::setVsync(bool vsync) {
    m_config.vsync = vsync;
    if (m_window) SDL_GL_SetSwapInterval(vsync ? 1 : 0);
}

void Window::swapBuffers() {
    if (m_window) {
        SDL_GL_SwapWindow(m_window);
    }
}

} // namespace OpenCraft
