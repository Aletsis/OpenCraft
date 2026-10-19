#pragma once

#include <string>
#include <cstdint>
#include "math/Math.hpp"

namespace OpenCraft {

enum class TutorialStep {
    MoveLook = 0,
    GatherWood,
    CraftPlanks,
    BuildShelter,
    SurviveAndSave,
    Completed
};

class TutorialSystem {
public:
    TutorialSystem();

    TutorialStep getCurrentStep() const { return m_currentStep; }
    void setStep(TutorialStep step) { m_currentStep = step; }
    bool isCompleted() const { return m_currentStep == TutorialStep::Completed; }

    // Notificaciones de eventos para avanzar pasos
    void onPlayerMoved(float distance);
    void onPlayerRotated(float angleDelta);
    void onBlockMined(uint16_t blockLegacyId);
    void onItemCrafted(uint16_t itemLegacyId);
    void onBlockPlaced(uint16_t blockLegacyId);
    void onGameSaved();

    // Consulta de textos actuales para HUD
    std::string getCurrentTitle() const;
    std::string getCurrentDescription() const;

    // Resetear tutorial
    void reset();

private:
    TutorialStep m_currentStep = TutorialStep::MoveLook;
    float m_accumulatedDistance = 0.0f;
    float m_accumulatedRotation = 0.0f;
};

} // namespace OpenCraft
