#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <SDL3/SDL.h>
#include "audio/SpatialAudio.hpp"

namespace OpenCraft {

// Salida de audio real: mezcla en el hilo de audio de SDL los efectos sintetizados por
// SpatialAudioEngine, con ganancias por canal calculadas en el hilo principal.
class AudioOutput {
public:
    AudioOutput() = default;
    ~AudioOutput();

    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;

    // Devuelve false si no hay dispositivo (el juego sigue sin sonido)
    bool init();
    bool isAvailable() const noexcept { return m_stream != nullptr; }

    // Reproduce un efecto por nombre canónico ("block.break", "entity.player.hurt"...)
    void play(const std::string& name, float leftGain, float rightGain, float pitch = 1.0f);
    void setMasterVolume(float volume);
    void setPaused(bool paused);

private:
    struct Voice {
        std::shared_ptr<const std::vector<float>> samples;
        double position = 0.0;
        double step = 1.0;
        float left = 1.0f;
        float right = 1.0f;
    };

    SDL_AudioStream* m_stream = nullptr;
    std::mutex m_mutex;
    std::vector<Voice> m_voices;
    std::unordered_map<std::string, std::shared_ptr<const std::vector<float>>> m_cache;
    float m_master = 1.0f;
    std::vector<float> m_mixBuffer;

    std::shared_ptr<const std::vector<float>> bufferFor(const std::string& name);
    static void SDLCALL feed(void* userdata, SDL_AudioStream* stream, int additional, int total);
    void mix(int frames);
};

} // namespace OpenCraft
