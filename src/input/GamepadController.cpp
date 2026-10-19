#include "input/GamepadController.hpp"
#include <cmath>
#include <algorithm>

namespace OpenCraft {

GamepadController::GamepadController() { reset(); }

void GamepadController::reset() {
    m_state = GamepadState{};
    m_inventoryPending = false;
    m_pausePending = false;
    m_hotbarDelta = 0;
}

static float applyDeadzone(float val, float deadzone) {
    if (std::abs(val) < deadzone) return 0.0f;
    float sign = (val > 0.0f) ? 1.0f : -1.0f;
    return sign * (std::abs(val) - deadzone) / (1.0f - deadzone);
}

void GamepadController::updateAxis(int axisId, float normalizedValue, float deadzone) {
    float filtered = applyDeadzone(normalizedValue, deadzone);

    switch (axisId) {
        case 0: // Left Stick X
            m_state.leftStick.x = filtered;
            break;
        case 1:                              // Left Stick Y
            m_state.leftStick.y = -filtered; // Invertir para que arriba sea +1
            break;
        case 2: // Right Stick X
            m_state.rightStick.x = filtered;
            break;
        case 3: // Right Stick Y
            m_state.rightStick.y = filtered;
            break;
        case 4: // Left Trigger
            m_state.leftTrigger = std::clamp(normalizedValue, 0.0f, 1.0f);
            break;
        case 5: // Right Trigger
            m_state.rightTrigger = std::clamp(normalizedValue, 0.0f, 1.0f);
            break;
        default: break;
    }
}

void GamepadController::updateButton(int buttonId, bool isDown) {
    switch (buttonId) {
        case 0: m_state.buttonA = isDown; break;
        case 1: m_state.buttonB = isDown; break;
        case 2: m_state.buttonX = isDown; break;
        case 3:
            m_state.buttonY = isDown;
            if (isDown) m_inventoryPending = true;
            break;
        case 4:
            m_state.buttonLB = isDown;
            if (isDown) m_hotbarDelta -= 1;
            break;
        case 5:
            m_state.buttonRB = isDown;
            if (isDown) m_hotbarDelta += 1;
            break;
        case 6:
            m_state.buttonStart = isDown;
            if (isDown) m_pausePending = true;
            break;
        case 7: m_state.buttonBack = isDown; break;
        case 11: m_state.dpadUp = isDown; break;
        case 12: m_state.dpadDown = isDown; break;
        case 13: m_state.dpadLeft = isDown; break;
        case 14: m_state.dpadRight = isDown; break;
        default: break;
    }
}

bool GamepadController::consumeInventoryTrigger() {
    bool val = m_inventoryPending;
    m_inventoryPending = false;
    return val;
}

bool GamepadController::consumePauseTrigger() {
    bool val = m_pausePending;
    m_pausePending = false;
    return val;
}

int GamepadController::consumeHotbarDelta() {
    int val = m_hotbarDelta;
    m_hotbarDelta = 0;
    return val;
}

} // namespace OpenCraft
