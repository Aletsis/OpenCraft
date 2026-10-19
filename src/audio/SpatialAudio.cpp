#include "audio/SpatialAudio.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <random>

namespace OpenCraft {

SpatialAudioEngine::SpatialAudioEngine() = default;
SpatialAudioEngine::~SpatialAudioEngine() = default;

void SpatialAudioEngine::setListener(const Math::Vec3& pos, const Math::Vec3& front, const Math::Vec3& up) {
    m_listenerPos = pos;
    m_listenerFront = front.normalized();
    m_listenerUp = up.normalized();
}

AudioSpatialResult SpatialAudioEngine::calculateSpatial(const Math::Vec3& emitterPos, float maxDistance) const {
    AudioSpatialResult result;
    Math::Vec3 diff = emitterPos - m_listenerPos;
    result.distance = diff.length();

    if (result.distance >= maxDistance || maxDistance <= 0.0001f) {
        result.audible = false;
        result.leftGain = 0.0f;
        result.rightGain = 0.0f;
        result.pan = 0.0f;
        return result;
    }

    result.audible = true;

    // Atenuación cuadrática suave/lineal
    float distNorm = result.distance / maxDistance;
    float attenuation = std::clamp(1.0f - distNorm, 0.0f, 1.0f);

    if (result.distance < 0.01f) {
        // En la misma posición del oyente (sonido centrado estéreo)
        result.pan = 0.0f;
        result.leftGain = attenuation;
        result.rightGain = attenuation;
        return result;
    }

    Math::Vec3 dir = diff.normalized();
    Math::Vec3 right = m_listenerFront.cross(m_listenerUp).normalized();

    // Paneo estéreo según el vector derecha del oyente
    float panDot = dir.dot(right);
    result.pan = std::clamp(panDot, -1.0f, 1.0f);

    // Reparto de ganancia L/R usando ley de paneo de potencia constante
    float angle = (result.pan + 1.0f) * 0.25f * Math::PI; // [0, PI/2]
    result.leftGain = attenuation * std::cos(angle);
    result.rightGain = attenuation * std::sin(angle);

    return result;
}

void SpatialAudioEngine::playSound(const SoundEvent& event, const DeviceSettings& settings) {
    if (m_isMuted) return;

    int categoryVol = 100;
    switch (event.category) {
        case AudioCategory::Master: categoryVol = settings.masterVolume; break;
        case AudioCategory::Music: categoryVol = settings.musicVolume; break;
        case AudioCategory::Weather: categoryVol = settings.weatherVolume; break;
        case AudioCategory::Blocks: categoryVol = settings.blocksVolume; break;
        case AudioCategory::Hostile: categoryVol = settings.hostileVolume; break;
        case AudioCategory::Friendly: categoryVol = settings.friendlyVolume; break;
        case AudioCategory::Players: categoryVol = settings.playersVolume; break;
        case AudioCategory::Ambient: categoryVol = settings.ambientVolume; break;
        default: break;
    }

    float effectiveGain = settings.getEffectiveVolume(categoryVol) * event.volume;
    if (effectiveGain <= 0.001f) return;

    if (event.isSpatial) {
        auto spatial = calculateSpatial(event.position, event.maxDistance);
        if (!spatial.audible) return;
    }

    // Registrar evento para seguimiento y subtítulos
    m_recentEvents.push_back(event);
    if (m_recentEvents.size() > 64) {
        m_recentEvents.erase(m_recentEvents.begin(), m_recentEvents.begin() + 16);
    }
}

// ==============================================================================
// Sintetizador Procedural de Audio
// ==============================================================================

AudioSampleBuffer SpatialAudioEngine::generateTone(float freqHz, float durationSec, int sampleRate) {
    AudioSampleBuffer buf;
    buf.sampleRate = sampleRate;
    size_t count = static_cast<size_t>(durationSec * sampleRate);
    buf.samples.resize(count);

    float phase = 0.0f;
    float phaseInc = 2.0f * Math::PI * freqHz / static_cast<float>(sampleRate);

    for (size_t i = 0; i < count; ++i) {
        // Envolvente lineal attack/decay
        float env = 1.0f;
        float t = static_cast<float>(i) / static_cast<float>(count);
        if (t < 0.1f)
            env = t / 0.1f;
        else
            env = 1.0f - (t - 0.1f) / 0.9f;

        float sample = std::sin(phase) * env * 16000.0f;
        buf.samples[i] = static_cast<int16_t>(std::clamp(sample, -32767.0f, 32767.0f));

        phase += phaseInc;
        if (phase > 2.0f * Math::PI) phase -= 2.0f * Math::PI;
    }
    return buf;
}

AudioSampleBuffer SpatialAudioEngine::generateNoiseBurst(float durationSec, int sampleRate) {
    AudioSampleBuffer buf;
    buf.sampleRate = sampleRate;
    size_t count = static_cast<size_t>(durationSec * sampleRate);
    buf.samples.resize(count);

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (size_t i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(count);
        float env = (1.0f - t); // Decaimiento exponencial simple
        float sample = dist(rng) * env * 14000.0f;
        buf.samples[i] = static_cast<int16_t>(std::clamp(sample, -32767.0f, 32767.0f));
    }
    return buf;
}

AudioSampleBuffer SpatialAudioEngine::generateFootstep(int sampleRate) { return generateNoiseBurst(0.08f, sampleRate); }

AudioSampleBuffer SpatialAudioEngine::generateBlockBreak(int sampleRate) {
    return generateNoiseBurst(0.18f, sampleRate);
}

AudioSampleBuffer SpatialAudioEngine::generateExplosion(int sampleRate) {
    AudioSampleBuffer buf;
    buf.sampleRate = sampleRate;
    size_t count = static_cast<size_t>(0.85f * sampleRate);
    buf.samples.resize(count);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    float lowPass = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(count);
        float env = std::pow(1.0f - t, 1.8f);
        float rawNoise = dist(rng);
        lowPass += 0.15f * (rawNoise - lowPass); // Filtro paso bajo para estruendo

        float sample = lowPass * env * 28000.0f;
        buf.samples[i] = static_cast<int16_t>(std::clamp(sample, -32767.0f, 32767.0f));
    }
    return buf;
}

AudioSampleBuffer SpatialAudioEngine::generateSplash(int sampleRate) {
    AudioSampleBuffer buf;
    buf.sampleRate = sampleRate;
    size_t count = static_cast<size_t>(0.35f * sampleRate);
    buf.samples.resize(count);

    std::mt19937 rng(999);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    for (size_t i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(count);
        float env = std::sin(t * Math::PI) * (1.0f - t * 0.7f);
        float sample = dist(rng) * env * 18000.0f;
        buf.samples[i] = static_cast<int16_t>(std::clamp(sample, -32767.0f, 32767.0f));
    }
    return buf;
}

AudioSampleBuffer SpatialAudioEngine::generateHurt(int sampleRate) {
    AudioSampleBuffer buf;
    buf.sampleRate = sampleRate;
    size_t count = static_cast<size_t>(0.22f * sampleRate);
    buf.samples.resize(count);

    float phase = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(count);
        float freq = 220.0f - t * 110.0f; // Caída de tono 220 Hz -> 110 Hz
        float phaseInc = 2.0f * Math::PI * freq / static_cast<float>(sampleRate);
        float env = (1.0f - t);

        float sample = std::sin(phase) * env * 20000.0f;
        buf.samples[i] = static_cast<int16_t>(std::clamp(sample, -32767.0f, 32767.0f));

        phase += phaseInc;
        if (phase > 2.0f * Math::PI) phase -= 2.0f * Math::PI;
    }
    return buf;
}

AudioSampleBuffer SpatialAudioEngine::generateClick(int sampleRate) {
    AudioSampleBuffer buf;
    buf.sampleRate = sampleRate;
    size_t count = static_cast<size_t>(0.02f * sampleRate);
    buf.samples.resize(count);

    for (size_t i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(count);
        float env = (1.0f - t);
        float sample = (i % 2 == 0 ? 1.0f : -1.0f) * env * 15000.0f;
        buf.samples[i] = static_cast<int16_t>(std::clamp(sample, -32767.0f, 32767.0f));
    }
    return buf;
}

// ==============================================================================
// Codificador WAV
// ==============================================================================

std::vector<uint8_t> SpatialAudioEngine::encodeWAV(const AudioSampleBuffer& buffer, int channels) {
    std::vector<uint8_t> out;
    uint32_t dataBytes = static_cast<uint32_t>(buffer.samples.size() * sizeof(int16_t));
    uint32_t totalBytes = 36 + dataBytes;

    out.resize(44 + dataBytes);
    uint8_t* p = out.data();

    // RIFF header
    std::memcpy(p, "RIFF", 4);
    std::memcpy(p + 4, &totalBytes, 4);
    std::memcpy(p + 8, "WAVE", 4);

    // fmt subchunk
    std::memcpy(p + 12, "fmt ", 4);
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = static_cast<uint16_t>(channels);
    uint32_t sRate = static_cast<uint32_t>(buffer.sampleRate);
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = sRate * numChannels * (bitsPerSample / 8);
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);

    std::memcpy(p + 16, &subchunk1Size, 4);
    std::memcpy(p + 20, &audioFormat, 2);
    std::memcpy(p + 22, &numChannels, 2);
    std::memcpy(p + 24, &sRate, 4);
    std::memcpy(p + 28, &byteRate, 4);
    std::memcpy(p + 32, &blockAlign, 2);
    std::memcpy(p + 34, &bitsPerSample, 2);

    // data subchunk
    std::memcpy(p + 36, "data", 4);
    std::memcpy(p + 40, &dataBytes, 4);

    if (dataBytes > 0) {
        std::memcpy(p + 44, buffer.samples.data(), dataBytes);
    }

    return out;
}

} // namespace OpenCraft
