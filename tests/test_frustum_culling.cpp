#include <iostream>
#include <cassert>
#include "render/Frustum.hpp"
#include "math/Math.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F03 Test 5] Frustum Culling por sección y descarte AABB ---" << std::endl;

    // Configurar cámara mirando hacia -Z desde el origen (0, 0, 0)
    Math::Mat4 proj = Math::Mat4::perspective(1.04719755f, 16.0f / 9.0f, 0.1f, 100.0f); // 60 grados FOV
    Math::Mat4 view = Math::Mat4::identity(); // Cámara en origen mirando hacia -Z
    Math::Mat4 vp = proj * view;

    Frustum frustum;
    frustum.updateFromViewProj(vp);

    // 1. Caja dentro del frustum: sección enfrente de la cámara (Z = -20)
    Math::AABB boxInFront{.min = {-8.0f, -8.0f, -25.0f}, .max = {8.0f, 8.0f, -15.0f}};
    assert(frustum.intersectsAABB(boxInFront) && "Sección directamente al frente debe intersectar el frustum");
    (void)boxInFront;

    // 2. Caja detrás de la cámara: sección en Z positivo (+30)
    Math::AABB boxBehind{.min = {-8.0f, -8.0f, 20.0f}, .max = {8.0f, 8.0f, 35.0f}};
    assert(!frustum.intersectsAABB(boxBehind) && "Sección detrás de la cámara debe ser descartada (culled)");
    (void)boxBehind;

    // 3. Caja lateral fuera del campo de visión (X = -200)
    Math::AABB boxFarLeft{.min = {-210.0f, -8.0f, -25.0f}, .max = {-190.0f, 8.0f, -15.0f}};
    assert(!frustum.intersectsAABB(boxFarLeft) && "Sección lateral fuera de FOV debe ser descartada");
    (void)boxFarLeft;

    // 4. Caja más allá del plano lejano (Z = -200 con Far = 100)
    Math::AABB boxTooFar{.min = {-8.0f, -8.0f, -250.0f}, .max = {8.0f, 8.0f, -200.0f}};
    assert(!frustum.intersectsAABB(boxTooFar) && "Sección más allá del plano lejano debe ser descartada");
    (void)boxTooFar;

    // 5. Verificación de containsPoint
    assert(frustum.containsPoint(Math::Vec3(0.0f, 0.0f, -10.0f)));
    assert(!frustum.containsPoint(Math::Vec3(0.0f, 0.0f, 10.0f)));

    std::cout << "EXITO: Frustum Culling y descarte de secciones fuera de visión verificado correctamente."
              << std::endl;
    return 0;
}
