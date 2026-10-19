#include <cassert>
#include <iostream>
#include "input/TouchController.hpp"

using namespace OpenCraft;

void testSimultaneousMultiTouch() {
    TouchController touch;
    touch.updateScreenDimensions(1280, 720, 1.0f);

    // Dedo 1: Toca el joystick virtual (zona inferior izquierda)
    Math::Vec2 joyCenter = touch.getJoystickCenter();
    // Arrastrar 30px hacia arriba (-Y en coordenadas de pantalla)
    touch.handleFingerDown(1, joyCenter.x, joyCenter.y);
    touch.handleFingerMotion(1, joyCenter.x, joyCenter.y - 35.0f, 0.0f, -35.0f);

    Math::Vec2 moveVec = touch.getMovementVector();
    assert(moveVec.y > 0.3f); // Movimiento hacia adelante
    assert(std::abs(moveVec.x) < 0.1f);

    // Dedo 2: Mirada táctil (mitad derecha, libre de botones)
    touch.handleFingerDown(2, 800.0f, 300.0f);
    touch.handleFingerMotion(2, 820.0f, 290.0f, 20.0f, -10.0f);

    Math::Vec2 look = touch.getLookDelta();
    assert(look.x == 20.0f);
    assert(look.y == -10.0f);

    // Dedo 3: Presionar botón de Salto (Jump) simultáneamente
    const auto& buttons = touch.getButtons();
    const TouchHitbox* jumpBtn = nullptr;
    const TouchHitbox* attackBtn = nullptr;
    for (const auto& btn : buttons) {
        if (btn.id == TouchButtonID::Jump) jumpBtn = &btn;
        if (btn.id == TouchButtonID::Attack) attackBtn = &btn;
    }
    assert(jumpBtn != nullptr);
    assert(attackBtn != nullptr);

    touch.handleFingerDown(3, jumpBtn->x + 10.0f, jumpBtn->y + 10.0f);
    assert(touch.isJumpPressed());

    // Dedo 4: Presionar botón de Ataque simultáneamente
    touch.handleFingerDown(4, attackBtn->x + 10.0f, attackBtn->y + 10.0f);
    assert(touch.isAttackPressed());

    // Verificar que los 4 dedos son simultáneamente activos y válidos
    assert(touch.getActiveFingerCount() == 4);
    assert(touch.getMovementVector().y > 0.3f);
    assert(touch.isJumpPressed());
    assert(touch.isAttackPressed());

    // Levantar dedo 3 (salto)
    touch.handleFingerUp(3, jumpBtn->x + 10.0f, jumpBtn->y + 10.0f);
    assert(!touch.isJumpPressed());
    assert(touch.isAttackPressed());
    assert(touch.getMovementVector().y > 0.3f);

    touch.cancelAllFingers();
    assert(touch.getActiveFingerCount() == 0);
    assert(!touch.isAttackPressed());
    assert(touch.getMovementVector().length() == 0.0f);

    std::cout << "[PASS] Entrada multitáctil simultánea (4 dedos) verificada." << std::endl;
}

void testSafeAreaInsetsAndRotation() {
    TouchController touch;
    touch.updateScreenDimensions(1280, 720, 1.0f);

    Math::Vec2 originalJoy = touch.getJoystickCenter();

    // Aplicar muesca/área segura (iPhone con notch o Android recortado)
    SafeAreaInsets insets{.top = 28.0f, .bottom = 34.0f, .left = 48.0f, .right = 48.0f};
    touch.setSafeArea(insets);

    Math::Vec2 insetsJoy = touch.getJoystickCenter();
    // El joystick debe haberse desplazado a la derecha por la muesca izquierda (+48px)
    assert(insetsJoy.x > originalJoy.x);
    // Y desplazado hacia arriba por el margen inferior
    assert(insetsJoy.y < originalJoy.y);

    // Botón de salto debe desplazarse hacia adentro (lejos del borde derecho recortado)
    for (const auto& btn : touch.getButtons()) {
        if (btn.id == TouchButtonID::Jump) {
            assert(btn.x + btn.w <= (1280.0f - insets.right));
            assert(btn.y + btn.h <= (720.0f - insets.bottom));
        }
    }

    // Rotación / cambio de tamaño de pantalla (ej. de 1280x720 a 2400x1080)
    touch.updateScreenDimensions(2400, 1080, 1.5f);
    assert(touch.getScreenWidth() == 2400);
    assert(touch.getScreenHeight() == 1080);

    for (const auto& btn : touch.getButtons()) {
        assert(btn.x + btn.w <= 2400.0f);
        assert(btn.y + btn.h <= 1080.0f);
    }

    std::cout << "[PASS] Adaptación a área segura (Notch/Cutout) y rotación de pantalla verificadas." << std::endl;
}

void testFingerCancellationHandling() {
    TouchController touch;
    touch.updateScreenDimensions(1280, 720, 1.0f);

    // Presionar botón de ataque y mover joystick
    touch.handleFingerDown(10, 120.0f, 600.0f);
    touch.handleFingerMotion(10, 120.0f, 570.0f, 0.0f, -30.0f);

    const auto& buttons = touch.getButtons();
    for (const auto& btn : buttons) {
        if (btn.id == TouchButtonID::Attack) {
            touch.handleFingerDown(11, btn.x + 5.0f, btn.y + 5.0f);
        }
    }

    assert(touch.isAttackPressed());
    assert(touch.getMovementVector().length() > 0.0f);

    // El sistema operativo emite SDL_EVENT_FINGER_CANCELED (por ejemplo, llamada entrante o gesto OS)
    touch.handleFingerCanceled(11);
    assert(!touch.isAttackPressed()); // El botón ya no está pegado
    assert(touch.getMovementVector().length() > 0.0f);

    touch.handleFingerCanceled(10);
    assert(touch.getMovementVector().length() == 0.0f);
    assert(touch.getActiveFingerCount() == 0);

    std::cout << "[PASS] Cancelación limpia de eventos táctiles (anti-dedos pegados) verificada." << std::endl;
}

int main() {
    std::cout << "=== Ejecutando Suite de Pruebas: TouchControllerAndSafeAreaTest ===" << std::endl;
    testSimultaneousMultiTouch();
    testSafeAreaInsetsAndRotation();
    testFingerCancellationHandling();
    std::cout << "=== Todas las pruebas de Controles Táctiles y Área Segura SUPERADAS ===" << std::endl;
    return 0;
}
