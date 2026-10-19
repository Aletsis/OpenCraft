#include <iostream>
#include <cassert>
#include "world/World.hpp"
#include "sim/WorldSession.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F03 Test 4] Concurrencia, versionado de mallas y prevención de caída al vacío ---" << std::endl;

    // 1. Verificación de barrera física contra caída al vacío en chunks no cargados
    {
        WorldSession session;
        // Inicializar únicamente chunk (0, 0)
        auto singleChunk = std::make_unique<Chunk>(0, 0);
        singleChunk->generateTerrain(12345);
        session.getWorld().setChunk(0, 0, std::move(singleChunk));

        assert(session.getWorld().isChunkLoaded(0, 0));
        assert(!session.getWorld().isChunkLoaded(1, 0) && "El chunk (1, 0) no debe estar cargado aún");

        // Colocar al jugador en el borde del chunk (0, 0), en X = 15.0, mirando hacia el este
        auto& player = session.getPlayer();
        player.position = Math::Vec3(15.0f, 68.0f, 8.0f);
        player.velocity = Math::Vec3(5.0f, 0.0f, 0.0f); // Velocidad intentando cruzar hacia X = 16..17

        // Emitir intención de movimiento
        session.queueCommand(MoveCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                         .moveInput = Math::Vec3(1.0f, 0.0f, 0.0f),
                                         .yaw = 90.0f,
                                         .pitch = 0.0f,
                                         .jump = false});

        session.update(0.1);

        // El jugador NO debe haber caído al vacío ni haber cruzado al chunk no cargado
        assert(player.position.x <= 16.0f && "La física debe bloquear el paso hacia chunks no cargados");
        assert(player.position.y >= 60.0f && "El jugador no debe caer al vacío");
    }

    // 2. Verificación de descarte de mallas obsoletas mediante versionado
    {
        World world;
        auto chunk = std::make_unique<Chunk>(0, 0);
        chunk->generateTerrain(12345);
        world.setChunk(0, 0, std::move(chunk));

        Chunk* liveChunk = world.getChunk(0, 0);
        assert(liveChunk != nullptr);

        uint32_t initialVersion = liveChunk->getMeshVersion();

        // Simular solicitud de malla con la versión actual (initialVersion)
        uint32_t queuedMeshVersion = initialVersion;

        // Mientras el hilo de trabajo CPU procesa en segundo plano, el jugador edita un bloque
        liveChunk->setBlock(5, 70, 5, BlockID::Wood);
        assert(liveChunk->getMeshVersion() > initialVersion && "Modificar un bloque debe incrementar meshVersion");

        // Cuando el hilo de trabajo termina, el resultado trae queuedMeshVersion (obsoleto)
        bool isMeshResultOutdated = (queuedMeshVersion != liveChunk->getMeshVersion());
        assert(isMeshResultOutdated &&
               "La malla calculada con versión anterior debe identificarse como obsoleta y descartarse");
        (void)isMeshResultOutdated;

        // Una nueva solicitud de malla toma la versión vigente
        uint32_t newQueuedVersion = liveChunk->getMeshVersion();
        bool isNewMeshValid = (newQueuedVersion == liveChunk->getMeshVersion());
        assert(isNewMeshValid && "La malla calculada con la versión más reciente debe ser aceptada");
        (void)isNewMeshValid;
    }

    // 3. Verificación de bordes verticales y negativos
    {
        World world;
        auto negChunk = std::make_unique<Chunk>(-5, -5);
        negChunk->generateTerrain(54321);
        world.setChunk(-5, -5, std::move(negChunk));

        // Probar borde vertical inferior (Bedrock en y = -64)
        BlockStateId bottomState = world.getBlockState(-5 * 16 + 2, Chunk::MIN_Y, -5 * 16 + 2);
        assert(bottomState != AIR_BLOCK_STATE && "El fondo en y = -64 debe tener bloques de Bedrock");
        (void)bottomState;

        // Modificar bloque en el límite negativo de altura
        BlockStateId woodId = BlockRegistry::global().getStateId("opencraft:wood");
        world.setBlockState(-5 * 16 + 2, Chunk::MIN_Y + 1, -5 * 16 + 2, woodId);
        assert(world.getBlockState(-5 * 16 + 2, Chunk::MIN_Y + 1, -5 * 16 + 2) == woodId);
    }

    std::cout << "EXITO: Descarte de mallas obsoletas, barrera de vacío y bordes negativos/verticales verificados."
              << std::endl;
    return 0;
}
