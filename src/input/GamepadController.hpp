#pragma once

#include <cstdint>
#include <string>
#include "math/Math.hpp"

namespace OpenCraft {

struct GamepadState {
    Math::Vec2 leftStick{0.0f, 0.0f};  // Movimiento [-1, 1]
    Math::Vec2 rightStick{0.0f, 0.0f}; // Mirada [-1, 1]
    float leftTrigger = 0.0f;          // 0 a 1 (Colocar / Usar)
    float rightTrigger = 0.0f;         // 0 a 1 (Atacar / Romper)

    bool buttonA = false;     // Salto / Confirmar UI
    bool buttonB = false;     // Agacharse / Cancelar UI
    bool buttonX = false;     // Dropear
    bool buttonY = false;     // Inventario
    bool buttonLB = false;    // Hotbar anterior
    bool buttonRB = false;    // Hotbar siguiente
    bool buttonStart = false; // Menú de pausa
    bool buttonBack = false;  // Estadísticas / Logros

    bool dpadUp = false;
    bool dpadDown = false;
    bool dpadLeft = false;
    bool dpadRight = false;
};

class GamepadController {
public:
    GamepadController();

    void updateAxis(int axisId, float normalizedValue, float deadzone = 0.15f);
    void updateButton(int buttonId, bool isDown);
    void reset();

    const GamepadState& getState() const { return m_state; }

    Math::Vec2 getMovementVector() const { return m_state.leftStick; }
    Math::Vec2 getLookVector(float sensitivity = 1.0f) const { return m_state.rightStick * sensitivity; }

    bool isJumpPressed() const { return m_state.buttonA; }
    bool isSneakPressed() const { return m_state.buttonB; }
    bool isAttackPressed() const { return m_state.rightTrigger > 0.5f; }
    bool isUsePressed() const { return m_state.leftTrigger > 0.5f; }

    bool consumeInventoryTrigger();
    bool consumePauseTrigger();
    int consumeHotbarDelta(); // -1, 0, +1

private:
    GamepadState m_state;
    bool m_inventoryPending = false;
    bool m_pausePending = false;
    int m_hotbarDelta = 0;
};

} // namespace OpenCraft
