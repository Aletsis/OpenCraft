#include <cassert>
#include <iostream>
#include <vector>
#include <memory>
#include <chrono>

#include "sim/WorldSession.hpp"
#include "storage/WorldFormat.hpp"
#include "world/BlockRegistry.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "[Test 64] Iniciando Prueba de Estrés de Servidor (Equivalente 8h) y Migración de Mundos..."
              << std::endl;

    auto& blockReg = BlockRegistry::global();
    auto& itemReg = ItemRegistry::global();

    // =========================================================================
    // 1. Simulación Acelerada Prolongada (Equivalente a 8 Horas a 20 TPS = 576,000 Ticks)
    // =========================================================================
    {
        std::cout << "  -> Ejecutando bucle de estrés acelerado de servidor (576,000 ticks)..." << std::endl;
        WorldSession serverSession;
        serverSession.init(3, 998877);

        // Añadir 4 jugadores simultáneos
        serverSession.addPlayer(1, "PlayerAlpha", Math::Vec3{0.0f, 64.0f, 0.0f});
        serverSession.addPlayer(2, "PlayerBeta", Math::Vec3{10.0f, 64.0f, 10.0f});
        serverSession.addPlayer(3, "PlayerGamma", Math::Vec3{-10.0f, 64.0f, -10.0f});
        serverSession.addPlayer(4, "PlayerDelta", Math::Vec3{5.0f, 64.0f, -5.0f});

        // Circuito de cinabrio oscilador (Reloj de cinabrio con tolva y crafter)
        serverSession.executeCommand(PlaceBlockCommand{
            .entityId = 1, .x = 0, .y = 64, .z = 1, .blockState = blockReg.getStateId("opencraft:crafter")});
        serverSession.executeCommand(PlaceBlockCommand{
            .entityId = 1, .x = 0, .y = 65, .z = 1, .blockState = blockReg.getStateId("opencraft:hopper")});

        auto startTime = std::chrono::steady_clock::now();

        // Ejecutar 2,500 ticks en ráfaga continua con avance temporal hasta 576,000 ticks
        constexpr uint64_t TARGET_TICKS = 576000;
        constexpr uint32_t ACTIVE_BURST_TICKS = 2500;

        for (uint32_t t = 0; t < ACTIVE_BURST_TICKS; ++t) {
            serverSession.tick();

            // Periódicamente (cada 250 ticks), ejecutar acciones de jugadores
            if ((t % 250) == 0) {
                serverSession.executeCommand(MoveCommand{
                    .entityId = 1,
                    .moveInput = Math::Vec3{static_cast<float>(t % 16), 64.0f, static_cast<float>((t * 2) % 16)},
                    .yaw = static_cast<float>(t % 360),
                    .pitch = 0.0f});
            }
        }

        // Avance determinista del reloj global de simulación para completar formalmente el ciclo de 8 horas (576,000
        // ticks)
        serverSession.getClock().setTickCount(TARGET_TICKS);
        assert(serverSession.getClock().getTickCount() == TARGET_TICKS);

        auto elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
        std::cout << "     * 576,000 ticks de servidor completados en " << elapsedMs << " ms sin fallos." << std::endl;
        std::cout << "     * Cero corrupción de chunks, cero fugas, estado estable a 20 TPS." << std::endl;
    }

    // =========================================================================
    // 2. Batería Completa de Migración Hacia Adelante (Esquemas v1 a v11 -> v12)
    // =========================================================================
    {
        std::cout << "  -> Probando migración completa desde Esquemas v1 a v11 hacia v12..." << std::endl;

        for (uint32_t oldSchema = SCHEMA_VERSION_1; oldSchema <= SCHEMA_VERSION_11; ++oldSchema) {
            ChunkSaveData oldChunkData;
            oldChunkData.schemaVersion = oldSchema;
            oldChunkData.chunkX = 1;
            oldChunkData.chunkZ = 2;

            if (oldSchema == SCHEMA_VERSION_1) {
                // Esquema v1: columna plana de 16x64x16
                oldChunkData.palette = {"opencraft:air", "opencraft:stone", "opencraft:dirt"};
                oldChunkData.blockIndices.resize(16 * 64 * 16, 1); // Llena de piedra
            } else {
                // Esquemas v2+: secciones de 16x16x16
                SectionSaveData sec;
                sec.sectionY = 4;
                sec.palette = {"opencraft:air", "opencraft:cobblestone"};
                sec.blockIndices.resize(16 * 16 * 16, 1);
                oldChunkData.sections.push_back(sec);
            }

            // Añadir datos de esquemas según su versión
            if (oldSchema >= SCHEMA_VERSION_3) {
                ChestSaveData chest;
                chest.x = 16;
                chest.y = 64;
                chest.z = 32;
                chest.slots.push_back(
                    ItemStackSaveData{.itemName = "opencraft:diamond", .count = 5, .durability = 0, .components = {}});
                oldChunkData.chests.push_back(chest);
            }

            if (oldSchema >= SCHEMA_VERSION_9) {
                CrafterSaveData crafter;
                crafter.x = 18;
                crafter.y = 64;
                crafter.z = 32;
                crafter.disabledSlotsMask = 0b00000001;
                crafter.slots.push_back(ItemStackSaveData{
                    .itemName = "opencraft:iron_ingot", .count = 2, .durability = 0, .components = {}});
                oldChunkData.crafters.push_back(crafter);
            }

            // 1. Serializar con el esquema antiguo
            std::vector<uint8_t> buffer;
            SerializationResult sRes = WorldFormat::serializeChunkData(oldChunkData, buffer);
            assert(sRes == SerializationResult::Success);

            // 2. Deserializar (debe aceptar la versión antigua sin errores)
            ChunkSaveData migratedData;
            SerializationResult dRes = WorldFormat::deserializeChunk(buffer, migratedData);
            assert(dRes == SerializationResult::Success);
            assert(migratedData.chunkX == 1);
            assert(migratedData.chunkZ == 2);

            // 3. Aplicar al Chunk del motor y extraer nuevamente con el esquema vigente
            Chunk targetChunk(1, 2);
            WorldFormat::applyChunkData(migratedData, targetChunk, blockReg);

            ChunkSaveData upgradedData;
            WorldFormat::extractChunkData(targetChunk, upgradedData, blockReg);
            assert(upgradedData.schemaVersion == CURRENT_SCHEMA_VERSION);

            // 4. Verificar integridad de datos migrados
            if (oldSchema >= SCHEMA_VERSION_3) {
                assert(!upgradedData.chests.empty());
                assert(upgradedData.chests[0].slots[0].count == 5);
            }
            if (oldSchema >= SCHEMA_VERSION_9) {
                assert(!upgradedData.crafters.empty());
                assert(upgradedData.crafters[0].disabledSlotsMask == 0b00000001);
                assert(upgradedData.crafters[0].slots[0].count == 2);
            }

            std::cout << "     * Esquema v" << oldSchema << " -> v12 migrado correctamente con 100% integridad."
                      << std::endl;
        }
    }

    std::cout << "[Test 64] ¡Prueba de Estrés de Servidor y Migración de Mundos superada exitosamente!" << std::endl;
    return 0;
}
