#include "game/TutorialSystem.hpp"
#include "ui/LocalizationManager.hpp"
#include "world/Block.hpp"
#include <cmath>

namespace OpenCraft {

TutorialSystem::TutorialSystem() = default;

void TutorialSystem::reset() {
    m_currentStep = TutorialStep::MoveLook;
    m_accumulatedDistance = 0.0f;
    m_accumulatedRotation = 0.0f;
}

void TutorialSystem::onPlayerMoved(float distance) {
    if (m_currentStep != TutorialStep::MoveLook) return;
    m_accumulatedDistance += std::abs(distance);
    if (m_accumulatedDistance >= 5.0f && m_accumulatedRotation >= 30.0f) {
        m_currentStep = TutorialStep::GatherWood;
    }
}

void TutorialSystem::onPlayerRotated(float angleDelta) {
    if (m_currentStep != TutorialStep::MoveLook) return;
    m_accumulatedRotation += std::abs(angleDelta);
    if (m_accumulatedDistance >= 5.0f && m_accumulatedRotation >= 30.0f) {
        m_currentStep = TutorialStep::GatherWood;
    }
}

void TutorialSystem::onBlockMined(uint16_t blockLegacyId) {
    if (m_currentStep == TutorialStep::GatherWood) {
        if (blockLegacyId == static_cast<uint16_t>(BlockID::Wood)) {
            m_currentStep = TutorialStep::CraftPlanks;
        }
    }
}

void TutorialSystem::onItemCrafted(uint16_t itemLegacyId) {
    if (m_currentStep == TutorialStep::CraftPlanks) {
        if (itemLegacyId == static_cast<uint16_t>(BlockID::Planks) ||
            itemLegacyId == static_cast<uint16_t>(BlockID::CraftingTable)) {
            m_currentStep = TutorialStep::BuildShelter;
        }
    }
}

void TutorialSystem::onBlockPlaced(uint16_t blockLegacyId) {
    if (m_currentStep == TutorialStep::BuildShelter) {
        if (blockLegacyId == static_cast<uint16_t>(BlockID::CraftingTable) ||
            blockLegacyId == static_cast<uint16_t>(BlockID::Planks) ||
            blockLegacyId == static_cast<uint16_t>(BlockID::Dirt) ||
            blockLegacyId == static_cast<uint16_t>(BlockID::Cobblestone)) {
            m_currentStep = TutorialStep::SurviveAndSave;
        }
    }
}

void TutorialSystem::onGameSaved() {
    if (m_currentStep == TutorialStep::SurviveAndSave) {
        m_currentStep = TutorialStep::Completed;
    }
}

std::string TutorialSystem::getCurrentTitle() const {
    const auto& loc = LocalizationManager::global();
    switch (m_currentStep) {
        case TutorialStep::MoveLook: return loc.get("tutorial.step1.title");
        case TutorialStep::GatherWood: return loc.get("tutorial.step2.title");
        case TutorialStep::CraftPlanks: return loc.get("tutorial.step3.title");
        case TutorialStep::BuildShelter: return loc.get("tutorial.step4.title");
        case TutorialStep::SurviveAndSave: return loc.get("tutorial.step5.title");
        case TutorialStep::Completed: return loc.get("tutorial.completed");
    }
    return "";
}

std::string TutorialSystem::getCurrentDescription() const {
    const auto& loc = LocalizationManager::global();
    switch (m_currentStep) {
        case TutorialStep::MoveLook: return loc.get("tutorial.step1.desc");
        case TutorialStep::GatherWood: return loc.get("tutorial.step2.desc");
        case TutorialStep::CraftPlanks: return loc.get("tutorial.step3.desc");
        case TutorialStep::BuildShelter: return loc.get("tutorial.step4.desc");
        case TutorialStep::SurviveAndSave: return loc.get("tutorial.step5.desc");
        case TutorialStep::Completed: return loc.get("tutorial.completed");
    }
    return "";
}

} // namespace OpenCraft
