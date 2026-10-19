#include <iostream>
#include <cassert>
#include <cmath>
#include "sim/WorldSession.hpp"
#include "world/BlockRegistry.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F06 Test 3] Movimiento, Normalización y Prevención de Tunneling ---" << std::endl;

    WorldSession session;
    session.init(1, 999);

    PlayerEntity& player = session.getPlayer();
    player.gameMode = 0; // Supervivencia

    // Asegurar suelo sólido plano en Y=30
    BlockStateId stone = BlockRegistry::global().getStateId("opencraft:stone");
    for (int x = -10; x <= 25; ++x) {
        for (int z = -10; z <= 25; ++z) {
            session.getWorld().setBlockState(x, 30, z, stone);
            session.getWorld().setBlockState(x, 31, z, AIR_BLOCK_STATE);
            session.getWorld().setBlockState(x, 32, z, AIR_BLOCK_STATE);
        }
    }

    player.position = Math::Vec3(8.0f, 31.0f, 8.0f);
    player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
    player.isGrounded = true;
    player.yaw = 0.0f; // Mirando hacia +X

    // 1. Velocidad de caminata estándar (~4.3 m/s)
    session.executeCommand(MoveCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                       .moveInput = Math::Vec3(0.0f, 0.0f, 1.0f), // Adelante
                                       .yaw = 0.0f,
                                       .pitch = 0.0f,
                                       .jump = false,
                                       .sprint = false,
                                       .sneak = false});
    session.tick();
    float walkSpeed = std::sqrt(player.velocity.x * player.velocity.x + player.velocity.z * player.velocity.z);
    assert(std::abs(walkSpeed - 4.3f) < 0.05f);
    std::cout << "Paso 1: Velocidad de caminata verificada en 4.3 m/s." << std::endl;

    // 2. Velocidad de Correr (Sprinting ~5.6 m/s)
    player.hunger = 20.0f;
    session.executeCommand(MoveCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                       .moveInput = Math::Vec3(0.0f, 0.0f, 1.0f),
                                       .yaw = 0.0f,
                                       .pitch = 0.0f,
                                       .jump = false,
                                       .sprint = true,
                                       .sneak = false});
    session.tick();
    float sprintSpeed = std::sqrt(player.velocity.x * player.velocity.x + player.velocity.z * player.velocity.z);
    assert(std::abs(sprintSpeed - 5.6f) < 0.05f);
    assert(player.isSprinting);

    // Con hambre <= 6.0 no se puede correr
    player.hunger = 5.0f;
    session.executeCommand(MoveCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                       .moveInput = Math::Vec3(0.0f, 0.0f, 1.0f),
                                       .yaw = 0.0f,
                                       .pitch = 0.0f,
                                       .jump = false,
                                       .sprint = true,
                                       .sneak = false});
    session.tick();
    assert(!player.isSprinting);
    float restrictedSpeed = std::sqrt(player.velocity.x * player.velocity.x + player.velocity.z * player.velocity.z);
    assert(std::abs(restrictedSpeed - 4.3f) < 0.05f);
    std::cout << "Paso 2: Velocidad de sprint (5.6 m/s) y restricción con hambre <= 6 verificada." << std::endl;

    // 3. Velocidad de Agacharse (Sneaking ~1.3 m/s)
    session.executeCommand(MoveCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                       .moveInput = Math::Vec3(0.0f, 0.0f, 1.0f),
                                       .yaw = 0.0f,
                                       .pitch = 0.0f,
                                       .jump = false,
                                       .sprint = false,
                                       .sneak = true});
    session.tick();
    float sneakSpeed = std::sqrt(player.velocity.x * player.velocity.x + player.velocity.z * player.velocity.z);
    assert(std::abs(sneakSpeed - 1.3f) < 0.05f);
    assert(player.isSneaking);
    std::cout << "Paso 3: Velocidad de agachado verificada en 1.3 m/s." << std::endl;

    // 4. Normalización de entrada diagonal
    // Sin normalización, (1, 0, 1) resultaría en 4.3 * sqrt(2) ≈ 6.08 m/s
    session.executeCommand(MoveCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                       .moveInput = Math::Vec3(1.0f, 0.0f, 1.0f), // Diagonal (adelante + derecha)
                                       .yaw = 0.0f,
                                       .pitch = 0.0f,
                                       .jump = false,
                                       .sprint = false,
                                       .sneak = false});
    session.tick();
    float diagSpeed = std::sqrt(player.velocity.x * player.velocity.x + player.velocity.z * player.velocity.z);
    assert(std::abs(diagSpeed - 4.3f) < 0.05f);
    std::cout << "Paso 4: Normalización de entrada diagonal comprobada (magnitud exacta de 4.3 m/s sin exceso sqrt(2))."
              << std::endl;

    // 5. Protección de borde al agacharse (Sneak Ledge Protection)
    // Crear un precipicio en X=10: quitar el suelo a partir de X=10
    for (int z = -5; z <= 20; ++z) {
        session.getWorld().setBlockState(10, 30, z, AIR_BLOCK_STATE);
    }

    // Colocar jugador en el borde en X=9.8
    player.position = Math::Vec3(9.8f, 31.0f, 8.0f);
    player.velocity = Math::Vec3(0.0f, 0.0f, 0.0f);
    player.isGrounded = true;

    session.executeCommand(MoveCommand{
        .entityId = LOCAL_PLAYER_ENTITY_ID,
        .moveInput = Math::Vec3(0.0f, 0.0f, 1.0f), // Mover hacia +X (precipicio)
        .yaw = 0.0f,
        .pitch = 0.0f,
        .jump = false,
        .sprint = false,
        .sneak = true // Agachado
    });
    session.tick();

    // El jugador no debe haber caído al vacío y no debe haber pasado de X=10.0
    assert(player.position.x < 10.0f);
    assert(player.position.y == 31.0f);
    std::cout << "Paso 5: Protección de caída en borde al agacharse verificada." << std::endl;

    // 6. Prevención de Tunneling a bajo framerate (15 FPS -> dt = 0.0667s)
    // Construir un muro sólido de 1 bloque de espesor en X=15 (de Y=31 a Y=33)
    for (int y = 31; y <= 33; ++y) {
        for (int z = 0; z <= 16; ++z) {
            session.getWorld().setBlockState(15, y, z, stone);
        }
    }

    // Colocar jugador corriendo a toda velocidad hacia el muro
    player.position = Math::Vec3(14.2f, 31.0f, 8.0f);
    player.velocity = Math::Vec3(12.0f, 0.0f, 0.0f); // Velocidad muy alta (12 m/s)
    player.isGrounded = true;

    // Simular un paso de colisión con substepping a 15 FPS (dt = 0.0667s)
    // Desplazamiento sin substepping sería 12 * 0.0667 = 0.8m, que atravesaría la pared
    Math::Vec3 vel = player.velocity;
    session.update(1.0 / 15.0);

    // Comprobar que el jugador colisionó y se detuvo delante del muro (X <= 15.0 - halfW)
    float halfW = player.width * 0.5f;
    assert(player.position.x <= 15.0f - halfW + 0.01f);
    std::cout << "Paso 6: Prevención de atravesar sólidos (tunneling) mediante substepping a 15 FPS verificada."
              << std::endl;

    std::cout << ">>> [F06 Test 3] APROBADO AL 100% <<<" << std::endl;
    return 0;
}
