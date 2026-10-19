#pragma once

#include <string>
#include <SDL3/SDL.h>

namespace OpenCraft {

struct WindowConfig {
    std::string title = "OpenCraft";
    int width = 1280;
    int height = 720;
    bool vsync = true;
    bool fullscreen = false;
};

class Window {
public:
    explicit Window(const WindowConfig& config = WindowConfig{});
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool init();
    void pollEvents();
    // Actualiza tamaños ante eventos de ventana (la aplicación consume la cola de eventos)
    void onEvent(const SDL_Event& event);
    void setFullscreen(bool fullscreen);
    bool isFullscreen() const noexcept { return m_config.fullscreen; }
    void setVsync(bool vsync);
    void swapBuffers();

    bool shouldClose() const noexcept { return m_shouldClose; }
    void setShouldClose(bool close) noexcept { m_shouldClose = close; }

    int getWidth() const noexcept { return m_width; }
    int getHeight() const noexcept { return m_height; }
    int getFramebufferWidth() const noexcept { return m_fbWidth; }
    int getFramebufferHeight() const noexcept { return m_fbHeight; }
    float getAspectRatio() const noexcept {
        return m_fbHeight > 0 ? static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight) : 1.0f;
    }

    SDL_Window* getSDLWindow() const noexcept { return m_window; }
    SDL_GLContext getGLContext() const noexcept { return m_glContext; }

private:
    WindowConfig m_config;
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    int m_width = 0;
    int m_height = 0;
    int m_fbWidth = 0;
    int m_fbHeight = 0;
    bool m_shouldClose = false;
};

} // namespace OpenCraft
