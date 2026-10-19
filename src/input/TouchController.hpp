#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "math/Math.hpp"

namespace OpenCraft {

struct SafeAreaInsets {
    float top = 0.0f;
    float bottom = 0.0f;
    float left = 0.0f;
    float right = 0.0f;
};

enum class TouchButtonID {
    None = 0,
    Jump,
    Sneak,
    Attack,
    Use,
    Inventory,
    Pause,
    Hotbar
};

struct TouchHitbox {
    TouchButtonID id = TouchButtonID::None;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    bool isPressed = false;
    bool isToggle = false;
    bool isToggledOn = false;

    bool contains(float px, float py) const { return px >= x && px <= (x + w) && py >= y && py <= (y + h); }
};

enum class FingerRole {
    Unassigned = 0,
    Joystick,
    Look,
    Button,
    Hotbar
};

struct ActiveFinger {
    int64_t fingerId = -1;
    float startX = 0.0f;
    float startY = 0.0f;
    float currentX = 0.0f;
    float currentY = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    FingerRole role = FingerRole::Unassigned;
    TouchButtonID buttonId = TouchButtonID::None;
    size_t hotbarIndex = 0;
};

class TouchController {
public:
    TouchController();

    void updateScreenDimensions(int width, int height, float dpiScale = 1.0f);
    void setSafeArea(const SafeAreaInsets& insets);
    void setButtonScale(float scale);

    // Eventos táctiles (coordenadas en píxeles de pantalla)
    void handleFingerDown(int64_t fingerId, float x, float y);
    void handleFingerUp(int64_t fingerId, float x, float y);
    void handleFingerMotion(int64_t fingerId, float x, float y, float dx, float dy);
    void handleFingerCanceled(int64_t fingerId);
    void cancelAllFingers();

    // Consulta de estado para simulación
    Math::Vec2 getMovementVector() const; // [-1.0, 1.0]
    Math::Vec2 getLookDelta();            // Consume y retorna el delta acumulado
    bool isJumpPressed() const;
    bool isSneakActive() const;
    bool isAttackPressed() const;
    bool isUsePressed() const;
    bool isInventoryTriggered();
    bool isPauseTriggered();
    int getSelectedHotbarSlot() const; // -1 si no hubo cambio

    // Geometría y renderizado de controles para el HUD
    const std::vector<TouchHitbox>& getButtons() const { return m_buttons; }
    Math::Vec2 getJoystickCenter() const { return m_joystickCenter; }
    Math::Vec2 getJoystickKnob() const { return m_joystickKnob; }
    float getJoystickRadius() const { return m_joystickRadius; }

    const SafeAreaInsets& getSafeArea() const { return m_safeArea; }
    int getScreenWidth() const { return m_screenWidth; }
    int getScreenHeight() const { return m_screenHeight; }
    size_t getActiveFingerCount() const { return m_fingers.size(); }

private:
    int m_screenWidth = 1280;
    int m_screenHeight = 720;
    float m_dpiScale = 1.0f;
    float m_buttonScale = 1.0f;
    SafeAreaInsets m_safeArea;

    // Joystick virtual
    Math::Vec2 m_joystickCenter{120.0f, 600.0f};
    Math::Vec2 m_joystickKnob{120.0f, 600.0f};
    float m_joystickRadius = 60.0f;
    float m_joystickDeadzone = 10.0f;
    int64_t m_joystickFingerId = -1;

    // Mirada táctil
    int64_t m_lookFingerId = -1;
    Math::Vec2 m_accumulatedLookDelta{0.0f, 0.0f};

    // Botones virtuales
    std::vector<TouchHitbox> m_buttons;
    std::vector<ActiveFinger> m_fingers;

    bool m_inventoryTriggered = false;
    bool m_pauseTriggered = false;
    int m_selectedHotbarSlot = -1;

    void recalculateLayout();
    ActiveFinger* findFinger(int64_t fingerId);
};

} // namespace OpenCraft
