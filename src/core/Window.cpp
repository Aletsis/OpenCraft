#include "core/Window.hpp"
#include "core/Log.hpp"

#include <GLES3/gl3.h>

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

bool Window::init() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD)) {
        Log::error("Fallo al inicializar SDL3: {}", SDL_GetError());
        return false;
    }
    Log::info("SDL3 inicializado exitosamente");

    // Configurar atributos de contexto OpenGL ES 3.0
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

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        Log::error("Fallo al crear el contexto OpenGL ES 3.0: {}", SDL_GetError());
        return false;
    }

    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(m_config.vsync ? 1 : 0);

    // Obtener dimensiones reales de píxeles (High-DPI / Retina / Pantallas móviles)
    SDL_GetWindowSizeInPixels(m_window, &m_fbWidth, &m_fbHeight);
    SDL_GetWindowSize(m_window, &m_width, &m_height);

    const char* glVendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* glRenderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* glslVer = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));

    Log::info("Contexto Gráfico Inicializado:");
    Log::info("  Proveedor : {}", glVendor ? glVendor : "Desconocido");
    Log::info("  GPU       : {}", glRenderer ? glRenderer : "Desconocido");
    Log::info("  Versión   : {}", glVersion ? glVersion : "Desconocido");
    Log::info("  GLSL ES   : {}", glslVer ? glslVer : "Desconocido");
    Log::info("  Resolución: {}x{} (Framebuffer: {}x{})", m_width, m_height, m_fbWidth, m_fbHeight);

    return true;
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
