#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "math/Math.hpp"
#include "platform/DeviceSettings.hpp"

namespace OpenCraft {

enum class AudioCategory {
    Master = 0,
    Music,
    Records,
    Weather,
    Blocks,
    Hostile,
    Friendly,
    Players,
    Ambient,
    Count
};

struct AudioSpatialResult {
    float leftGain = 1.0f;
    float rightGain = 1.0f;
    float distance = 0.0f;
    float pan = 0.0f; // -1.0 (izquierda) a +1.0 (derecha)
    bool audible = true;
};

struct SoundEvent {
    std::string name;
    AudioCategory category = AudioCategory::Blocks;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    bool isSpatial = true;
    float volume = 1.0f;
    float pitch = 1.0f;
    float maxDistance = 48.0f;
};

struct AudioSampleBuffer {
    std::vector<int16_t> samples; // 16-bit PCM mono
    int sampleRate = 44100;
};

class SpatialAudioEngine {
public:
    SpatialAudioEngine();
    ~SpatialAudioEngine();

    void setListener(const Math::Vec3& pos, const Math::Vec3& front, const Math::Vec3& up);
    AudioSpatialResult calculateSpatial(const Math::Vec3& emitterPos, float maxDistance = 48.0f) const;

    // Emisión de evento sonoro
    void playSound(const SoundEvent& event, const DeviceSettings& settings);

    // Sintetizador procedural de efectos de sonido canónicos
    static AudioSampleBuffer generateTone(float freqHz, float durationSec, int sampleRate = 44100);
    static AudioSampleBuffer generateNoiseBurst(float durationSec, int sampleRate = 44100);
    static AudioSampleBuffer generateFootstep(int sampleRate = 44100);
    static AudioSampleBuffer generateBlockBreak(int sampleRate = 44100);
    static AudioSampleBuffer generateExplosion(int sampleRate = 44100);
    static AudioSampleBuffer generateSplash(int sampleRate = 44100);
    static AudioSampleBuffer generateHurt(int sampleRate = 44100);
    static AudioSampleBuffer generateClick(int sampleRate = 44100);

    // Exportador WAV para verificación y persistencia
    static std::vector<uint8_t> encodeWAV(const AudioSampleBuffer& buffer, int channels = 1);

    // Consulta de eventos reproducidos recientemente (para tests y subtítulos)
    const std::vector<SoundEvent>& getRecentEvents() const { return m_recentEvents; }
    void clearRecentEvents() { m_recentEvents.clear(); }

    // Control de silencio / pausa en segundo plano
    void setMuted(bool muted) { m_isMuted = muted; }
    bool isMuted() const { return m_isMuted; }

private:
    Math::Vec3 m_listenerPos{0.0f, 0.0f, 0.0f};
    Math::Vec3 m_listenerFront{0.0f, 0.0f, -1.0f};
    Math::Vec3 m_listenerUp{0.0f, 1.0f, 0.0f};

    bool m_isMuted = false;
    std::vector<SoundEvent> m_recentEvents;
};

} // namespace OpenCraft
