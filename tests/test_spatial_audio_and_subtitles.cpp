#include <cassert>
#include <iostream>
#include <cstring>
#include "audio/SpatialAudio.hpp"
#include "audio/AudioSubtitles.hpp"
#include "platform/DeviceSettings.hpp"

using namespace OpenCraft;

void testSpatialAudioAttenuationAndPanning() {
    SpatialAudioEngine engine;
    // Oyente en el origen, mirando hacia -Z (adelante), arriba +Y, derecha +X
    engine.setListener({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f});

    // 1. Sonido en la misma posición (centrado)
    auto centerRes = engine.calculateSpatial({0.0f, 0.0f, 0.0f}, 48.0f);
    assert(centerRes.audible);
    assert(std::abs(centerRes.pan) < 0.001f);
    assert(std::abs(centerRes.leftGain - centerRes.rightGain) < 0.001f);

    // 2. Sonido situado a la derecha (+X)
    auto rightRes = engine.calculateSpatial({10.0f, 0.0f, 0.0f}, 48.0f);
    assert(rightRes.audible);
    assert(rightRes.pan > 0.9f); // Paneo hacia la derecha
    assert(rightRes.rightGain > rightRes.leftGain);

    // 3. Sonido situado a la izquierda (-X)
    auto leftRes = engine.calculateSpatial({-10.0f, 0.0f, 0.0f}, 48.0f);
    assert(leftRes.audible);
    assert(leftRes.pan < -0.9f); // Paneo hacia la izquierda
    assert(leftRes.leftGain > leftRes.rightGain);

    // 4. Sonido más allá de la distancia máxima (48m)
    auto farRes = engine.calculateSpatial({0.0f, 0.0f, 60.0f}, 48.0f);
    assert(!farRes.audible);
    assert(farRes.leftGain == 0.0f);
    assert(farRes.rightGain == 0.0f);

    std::cout << "[PASS] Espacialización 3D (atenuación inversa y paneo L/R) verificada." << std::endl;
}

void testAudioCategoryMixingAndMute() {
    SpatialAudioEngine engine;
    engine.setListener({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f});

    DeviceSettings settings;
    settings.masterVolume = 100;
    settings.blocksVolume = 80;
    settings.hostileVolume = 0; // Muteado

    SoundEvent blockSound{
        .name = "block.break", .category = AudioCategory::Blocks, .position = {0.0f, 0.0f, -5.0f}, .volume = 1.0f};
    engine.playSound(blockSound, settings);
    assert(engine.getRecentEvents().size() == 1);

    SoundEvent hostileSound{.name = "entity.zombie.ambient",
                            .category = AudioCategory::Hostile,
                            .position = {0.0f, 0.0f, -5.0f},
                            .volume = 1.0f};
    engine.playSound(hostileSound, settings);
    // Como volumen de hostile es 0, no debe haberse reproducido
    assert(engine.getRecentEvents().size() == 1);

    // Silenciamiento global en segundo plano
    engine.setMuted(true);
    engine.playSound(blockSound, settings);
    assert(engine.getRecentEvents().size() == 1); // No añadido porque está silenciado

    engine.setMuted(false);
    engine.playSound(blockSound, settings);
    assert(engine.getRecentEvents().size() == 2);

    std::cout << "[PASS] Mezcladores de categorías y silenciamiento de fondo verificados." << std::endl;
}

void testAudioSubtitles() {
    AudioSubtitleManager sub;
    Math::Vec3 listenerPos{0.0f, 0.0f, 0.0f};
    Math::Vec3 listenerFront{0.0f, 0.0f, -1.0f};
    Math::Vec3 listenerUp{0.0f, 1.0f, 0.0f};

    // Subtítulo a la derecha
    sub.addSubtitle("Pisadas", {10.0f, 0.0f, 0.0f}, AudioCategory::Blocks, 2.0f);
    // Subtítulo a la izquierda
    sub.addSubtitle("Zombi gime", {-10.0f, 0.0f, 0.0f}, AudioCategory::Hostile, 2.0f);

    sub.update(0.1f, listenerPos, listenerFront, listenerUp);

    const auto& items = sub.getItems();
    assert(items.size() == 2);
    assert(items[0].direction == SubtitleDirection::Right);
    assert(items[0].getFormattedText() == "Pisadas >");
    assert(items[1].direction == SubtitleDirection::Left);
    assert(items[1].getFormattedText() == "< Zombi gime");

    // Avanzar tiempo para probar expiración
    sub.update(2.5f, listenerPos, listenerFront, listenerUp);
    assert(sub.getItems().empty());

    std::cout << "[PASS] Subtítulos direccionales y ciclo de vida de señales auditivas verificados." << std::endl;
}

void testProceduralAudioSynthesisAndWAVExport() {
    auto footstep = SpatialAudioEngine::generateFootstep();
    assert(!footstep.samples.empty());
    assert(footstep.sampleRate == 44100);

    auto blockBreak = SpatialAudioEngine::generateBlockBreak();
    assert(!blockBreak.samples.empty());

    auto explosion = SpatialAudioEngine::generateExplosion();
    assert(!explosion.samples.empty());

    // Codificación a formato WAV
    std::vector<uint8_t> wavData = SpatialAudioEngine::encodeWAV(footstep);
    assert(wavData.size() > 44);

    // Validar cabecera RIFF / WAVE canónica
    assert(std::memcmp(wavData.data(), "RIFF", 4) == 0);
    assert(std::memcmp(wavData.data() + 8, "WAVE", 4) == 0);
    assert(std::memcmp(wavData.data() + 12, "fmt ", 4) == 0);
    assert(std::memcmp(wavData.data() + 36, "data", 4) == 0);

    std::cout << "[PASS] Síntesis procedural de audio y codificador WAV (sin dependencias binarias) verificados."
              << std::endl;
}

int main() {
    std::cout << "=== Ejecutando Suite de Pruebas: SpatialAudioAndSubtitlesTest ===" << std::endl;
    testSpatialAudioAttenuationAndPanning();
    testAudioCategoryMixingAndMute();
    testAudioSubtitles();
    testProceduralAudioSynthesisAndWAVExport();
    std::cout << "=== Todas las pruebas de Audio Espacial y Subtítulos SUPERADAS ===" << std::endl;
    return 0;
}
