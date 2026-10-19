#include "audio/AudioOutput.hpp"
#include "core/Log.hpp"
#include <algorithm>
#include <cmath>

namespace OpenCraft {

namespace {
constexpr int SAMPLE_RATE = 44100;
constexpr size_t MAX_VOICES = 24;
} // namespace

AudioOutput::~AudioOutput() {
    if (m_stream) {
        SDL_DestroyAudioStream(m_stream);
        m_stream = nullptr;
    }
}

bool AudioOutput::init() {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        Log::warn("Audio no disponible: {}", SDL_GetError());
        return false;
    }
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = SAMPLE_RATE;
    m_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &AudioOutput::feed, this);
    if (!m_stream) {
        Log::warn("No se pudo abrir el dispositivo de audio: {}", SDL_GetError());
        return false;
    }
    SDL_ResumeAudioStreamDevice(m_stream);
    Log::info("Audio inicializado ({} Hz, estéreo)", SAMPLE_RATE);
    return true;
}

std::shared_ptr<const std::vector<float>> AudioOutput::bufferFor(const std::string& name) {
    auto it = m_cache.find(name);
    if (it != m_cache.end()) return it->second;

    AudioSampleBuffer pcm;
    if (name == "block.break")
        pcm = SpatialAudioEngine::generateBlockBreak(SAMPLE_RATE);
    else if (name == "block.place")
        pcm = SpatialAudioEngine::generateClick(SAMPLE_RATE);
    else if (name == "step")
        pcm = SpatialAudioEngine::generateFootstep(SAMPLE_RATE);
    else if (name == "entity.player.hurt" || name == "entity.hurt")
        pcm = SpatialAudioEngine::generateHurt(SAMPLE_RATE);
    else if (name == "entity.generic.explode")
        pcm = SpatialAudioEngine::generateExplosion(SAMPLE_RATE);
    else if (name == "entity.generic.splash")
        pcm = SpatialAudioEngine::generateSplash(SAMPLE_RATE);
    else if (name == "entity.item.pickup")
        pcm = SpatialAudioEngine::generateTone(880.0f, 0.06f, SAMPLE_RATE);
    else if (name == "ui.click")
        pcm = SpatialAudioEngine::generateClick(SAMPLE_RATE);
    else if (name == "entity.player.eat")
        pcm = SpatialAudioEngine::generateNoiseBurst(0.12f, SAMPLE_RATE);
    else
        pcm = SpatialAudioEngine::generateClick(SAMPLE_RATE);

    auto samples = std::make_shared<std::vector<float>>();
    samples->reserve(pcm.samples.size());
    for (int16_t v : pcm.samples) samples->push_back(static_cast<float>(v) / 32768.0f);
    m_cache.emplace(name, samples);
    return samples;
}

void AudioOutput::play(const std::string& name, float leftGain, float rightGain, float pitch) {
    if (!m_stream) return;
    if (leftGain <= 0.001f && rightGain <= 0.001f) return;
    auto buf = bufferFor(name);
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_voices.size() >= MAX_VOICES) {
        m_voices.erase(m_voices.begin()); // se descarta la voz más antigua
    }
    m_voices.push_back(Voice{buf, 0.0, std::clamp(static_cast<double>(pitch), 0.5, 2.0), leftGain, rightGain});
}

void AudioOutput::setMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_master = std::clamp(volume, 0.0f, 1.0f);
}

void AudioOutput::setPaused(bool paused) {
    if (!m_stream) return;
    if (paused)
        SDL_PauseAudioStreamDevice(m_stream);
    else
        SDL_ResumeAudioStreamDevice(m_stream);
}

void SDLCALL AudioOutput::feed(void* userdata, SDL_AudioStream* stream, int additional, int /*total*/) {
    auto* self = static_cast<AudioOutput*>(userdata);
    int frames = additional / static_cast<int>(sizeof(float) * 2);
    if (frames <= 0) return;
    self->mix(frames);
    SDL_PutAudioStreamData(stream, self->m_mixBuffer.data(), frames * static_cast<int>(sizeof(float) * 2));
}

void AudioOutput::mix(int frames) {
    m_mixBuffer.assign(static_cast<size_t>(frames) * 2, 0.0f);
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& v : m_voices) {
        const auto& s = *v.samples;
        for (int f = 0; f < frames; ++f) {
            size_t idx = static_cast<size_t>(v.position);
            if (idx >= s.size()) break;
            float sample = s[idx] * m_master;
            m_mixBuffer[static_cast<size_t>(f) * 2] += sample * v.left;
            m_mixBuffer[static_cast<size_t>(f) * 2 + 1] += sample * v.right;
            v.position += v.step;
        }
    }
    std::erase_if(m_voices, [](const Voice& v) { return static_cast<size_t>(v.position) >= v.samples->size(); });
    for (float& x : m_mixBuffer) x = std::tanh(x); // limitador suave contra saturación
}

} // namespace OpenCraft
