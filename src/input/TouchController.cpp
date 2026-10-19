#include "input/TouchController.hpp"
#include <cmath>
#include <algorithm>

namespace OpenCraft {

TouchController::TouchController() { recalculateLayout(); }

void TouchController::updateScreenDimensions(int width, int height, float dpiScale) {
    m_screenWidth = width;
    m_screenHeight = height;
    m_dpiScale = (dpiScale > 0.1f) ? dpiScale : 1.0f;
    recalculateLayout();
}

void TouchController::setSafeArea(const SafeAreaInsets& insets) {
    m_safeArea = insets;
    recalculateLayout();
}

void TouchController::setButtonScale(float scale) {
    m_buttonScale = std::clamp(scale, 0.5f, 2.5f);
    recalculateLayout();
}

void TouchController::recalculateLayout() {
    m_buttons.clear();

    float baseSize = 56.0f * m_dpiScale * m_buttonScale;
    float padding = 16.0f * m_dpiScale;

    // Joystick en esquina inferior izquierda, respetando safe area
    float joyX = m_safeArea.left + padding + 70.0f * m_dpiScale * m_buttonScale;
    float joyY = m_screenHeight - m_safeArea.bottom - padding - 70.0f * m_dpiScale * m_buttonScale;
    m_joystickCenter = {joyX, joyY};
    m_joystickKnob = m_joystickCenter;
    m_joystickRadius = 65.0f * m_dpiScale * m_buttonScale;
    m_joystickDeadzone = 8.0f * m_dpiScale;

    // Botones de acción en esquina inferior derecha
    float rightEdge = m_screenWidth - m_safeArea.right - padding;
    float bottomEdge = m_screenHeight - m_safeArea.bottom - padding;

    // Salto (Jump)
    m_buttons.push_back(TouchHitbox{.id = TouchButtonID::Jump,
                                    .x = rightEdge - baseSize,
                                    .y = bottomEdge - baseSize,
                                    .w = baseSize,
                                    .h = baseSize});

    // Agacharse / Sneak (Toggle)
    m_buttons.push_back(TouchHitbox{.id = TouchButtonID::Sneak,
                                    .x = rightEdge - baseSize * 2.2f,
                                    .y = bottomEdge - baseSize,
                                    .w = baseSize,
                                    .h = baseSize,
                                    .isToggle = true});

    // Atacar / Romper (Attack)
    m_buttons.push_back(TouchHitbox{.id = TouchButtonID::Attack,
                                    .x = rightEdge - baseSize,
                                    .y = bottomEdge - baseSize * 2.2f,
                                    .w = baseSize,
                                    .h = baseSize});

    // Usar / Colocar (Use)
    m_buttons.push_back(TouchHitbox{.id = TouchButtonID::Use,
                                    .x = rightEdge - baseSize * 2.2f,
                                    .y = bottomEdge - baseSize * 2.2f,
                                    .w = baseSize,
                                    .h = baseSize});

    // Inventario (esquina superior derecha)
    m_buttons.push_back(TouchHitbox{.id = TouchButtonID::Inventory,
                                    .x = rightEdge - baseSize,
                                    .y = m_safeArea.top + padding,
                                    .w = baseSize,
                                    .h = baseSize});

    // Menú / Pausa (esquina superior izquierda)
    m_buttons.push_back(TouchHitbox{.id = TouchButtonID::Pause,
                                    .x = m_safeArea.left + padding,
                                    .y = m_safeArea.top + padding,
                                    .w = baseSize,
                                    .h = baseSize});

    // Hotbar táctil (9 ranuras en la parte inferior central)
    float hotbarSlotSize = 40.0f * m_dpiScale;
    float totalHotbarWidth = 9.0f * hotbarSlotSize;
    float hotbarStartX = (m_screenWidth - totalHotbarWidth) * 0.5f;
    float hotbarY = m_screenHeight - m_safeArea.bottom - hotbarSlotSize - 4.0f;

    for (size_t i = 0; i < 9; ++i) {
        TouchHitbox slot;
        slot.id = TouchButtonID::Hotbar;
        slot.x = hotbarStartX + i * hotbarSlotSize;
        slot.y = hotbarY;
        slot.w = hotbarSlotSize;
        slot.h = hotbarSlotSize;
        m_buttons.push_back(slot);
    }
}

ActiveFinger* TouchController::findFinger(int64_t fingerId) {
    for (auto& f : m_fingers) {
        if (f.fingerId == fingerId) return &f;
    }
    return nullptr;
}

void TouchController::handleFingerDown(int64_t fingerId, float x, float y) {
    // Si ya existe este fingerId, limpiarlo
    handleFingerUp(fingerId, x, y);

    ActiveFinger f;
    f.fingerId = fingerId;
    f.startX = x;
    f.startY = y;
    f.currentX = x;
    f.currentY = y;
    f.role = FingerRole::Unassigned;

    // 1. Comprobar si golpea algún botón
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        auto& btn = m_buttons[i];
        if (btn.contains(x, y)) {
            f.role = FingerRole::Button;
            f.buttonId = btn.id;

            if (btn.id == TouchButtonID::Hotbar) {
                // Calcular índice de slot (desde el primer botón de hotbar que está en el offset m_buttons.size() - 9)
                f.role = FingerRole::Hotbar;
                f.hotbarIndex = i - (m_buttons.size() - 9);
                m_selectedHotbarSlot = static_cast<int>(f.hotbarIndex);
            } else if (btn.isToggle) {
                btn.isToggledOn = !btn.isToggledOn;
                btn.isPressed = btn.isToggledOn;
            } else {
                btn.isPressed = true;
                if (btn.id == TouchButtonID::Inventory) m_inventoryTriggered = true;
                if (btn.id == TouchButtonID::Pause) m_pauseTriggered = true;
            }
            m_fingers.push_back(f);
            return;
        }
    }

    // 2. Comprobar si está en la zona del Joystick (mitad izquierda y parte inferior)
    float distToJoyCenter = (Math::Vec2{x, y} - m_joystickCenter).length();
    if (distToJoyCenter <= m_joystickRadius * 1.5f || (x < m_screenWidth * 0.45f && y > m_screenHeight * 0.4f)) {
        if (m_joystickFingerId == -1) {
            f.role = FingerRole::Joystick;
            m_joystickFingerId = fingerId;

            // Centrar el joystick donde tocó si fue cerca del cuadrante inferior izquierdo
            Math::Vec2 offset = Math::Vec2{x, y} - m_joystickCenter;
            float len = offset.length();
            if (len > m_joystickRadius) {
                offset = offset * (m_joystickRadius / len);
            }
            m_joystickKnob = m_joystickCenter + offset;

            m_fingers.push_back(f);
            return;
        }
    }

    // 3. De lo contrario, asignar a mirada/cámara táctil (generalmente mitad derecha o centro libre)
    if (m_lookFingerId == -1) {
        f.role = FingerRole::Look;
        m_lookFingerId = fingerId;
        m_fingers.push_back(f);
        return;
    }

    // Si ya hay look y joystick, registrar dedo genérico
    m_fingers.push_back(f);
}

void TouchController::handleFingerMotion(int64_t fingerId, float x, float y, float dx, float dy) {
    ActiveFinger* f = findFinger(fingerId);
    if (!f) return;

    f->currentX = x;
    f->currentY = y;
    f->deltaX += dx;
    f->deltaY += dy;

    if (f->role == FingerRole::Joystick && fingerId == m_joystickFingerId) {
        Math::Vec2 offset = Math::Vec2{x, y} - m_joystickCenter;
        float len = offset.length();
        if (len > m_joystickRadius) {
            offset = offset * (m_joystickRadius / len);
        }
        m_joystickKnob = m_joystickCenter + offset;
    } else if (f->role == FingerRole::Look && fingerId == m_lookFingerId) {
        m_accumulatedLookDelta.x += dx;
        m_accumulatedLookDelta.y += dy;
    }
}

void TouchController::handleFingerUp(int64_t fingerId, float /*x*/, float /*y*/) {
    for (auto it = m_fingers.begin(); it != m_fingers.end(); ++it) {
        if (it->fingerId == fingerId) {
            if (it->role == FingerRole::Joystick) {
                m_joystickFingerId = -1;
                m_joystickKnob = m_joystickCenter;
            } else if (it->role == FingerRole::Look) {
                m_lookFingerId = -1;
            } else if (it->role == FingerRole::Button) {
                for (auto& btn : m_buttons) {
                    if (btn.id == it->buttonId && !btn.isToggle) {
                        btn.isPressed = false;
                    }
                }
            }
            m_fingers.erase(it);
            return;
        }
    }
}

void TouchController::handleFingerCanceled(int64_t fingerId) { handleFingerUp(fingerId, 0.0f, 0.0f); }

void TouchController::cancelAllFingers() {
    m_fingers.clear();
    m_joystickFingerId = -1;
    m_joystickKnob = m_joystickCenter;
    m_lookFingerId = -1;
    m_accumulatedLookDelta = {0.0f, 0.0f};

    for (auto& btn : m_buttons) {
        if (!btn.isToggle) {
            btn.isPressed = false;
        }
    }
}

Math::Vec2 TouchController::getMovementVector() const {
    if (m_joystickFingerId == -1) return {0.0f, 0.0f};

    Math::Vec2 diff = m_joystickKnob - m_joystickCenter;
    float dist = diff.length();
    if (dist <= m_joystickDeadzone) return {0.0f, 0.0f};

    float effectiveDist = (dist - m_joystickDeadzone) / (m_joystickRadius - m_joystickDeadzone);
    effectiveDist = std::clamp(effectiveDist, 0.0f, 1.0f);

    Math::Vec2 dir = diff / dist;
    // x = strafe (-1 izquierda, +1 derecha), y = forward (+1 adelante, -1 atrás)
    // En coordenadas de pantalla: Y hacia abajo es positivo, por lo que adelante es -dy
    return {dir.x * effectiveDist, -dir.y * effectiveDist};
}

Math::Vec2 TouchController::getLookDelta() {
    Math::Vec2 result = m_accumulatedLookDelta;
    m_accumulatedLookDelta = {0.0f, 0.0f};
    return result;
}

bool TouchController::isJumpPressed() const {
    for (const auto& btn : m_buttons) {
        if (btn.id == TouchButtonID::Jump) return btn.isPressed;
    }
    return false;
}

bool TouchController::isSneakActive() const {
    for (const auto& btn : m_buttons) {
        if (btn.id == TouchButtonID::Sneak) return btn.isToggledOn;
    }
    return false;
}

bool TouchController::isAttackPressed() const {
    for (const auto& btn : m_buttons) {
        if (btn.id == TouchButtonID::Attack) return btn.isPressed;
    }
    return false;
}

bool TouchController::isUsePressed() const {
    for (const auto& btn : m_buttons) {
        if (btn.id == TouchButtonID::Use) return btn.isPressed;
    }
    return false;
}

bool TouchController::isInventoryTriggered() {
    bool val = m_inventoryTriggered;
    m_inventoryTriggered = false;
    return val;
}

bool TouchController::isPauseTriggered() {
    bool val = m_pauseTriggered;
    m_pauseTriggered = false;
    return val;
}

int TouchController::getSelectedHotbarSlot() const { return m_selectedHotbarSlot; }

} // namespace OpenCraft
