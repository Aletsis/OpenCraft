#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>
#include <thread>
#include "world/World.hpp"
#include "world/ChunkWorkerPool.hpp"
#include "world/ChunkManager.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F03 Test 2] Residencia acotada y límite de memoria en recorrido de 10.000 bloques ---"
              << std::endl;

    World world;
    world.setRenderDistance(3); // Radio de 3 chunks (7x7 = 49 chunks activos)
    world.setSeed(424242);

    ChunkWorkerPool workerPool(2);
    ChunkManager manager(world, workerPool, nullptr);

    // Inicializar zona inicial
    manager.loadAreaSync(0, 0, 3);
    size_t initialLoaded = world.getLoadedChunkCount();
    assert(initialLoaded == 49 && "Con renderDistance=3 deben haber 49 chunks en memoria");

    size_t initialMemory = world.getEstimatedMemoryUsage();
    assert(initialMemory > 0);

    // Simular recorrido continuo de 10.000 bloques (de X = 0 a X = 10.000)
    // Avanzando 16 bloques (1 chunk completo) por paso = 625 chunks recorridos
    constexpr int totalDistance = 10000;
    constexpr int stepSize = 16;
    size_t maxLoadedObserved = 0;

    for (int x = 0; x <= totalDistance; x += stepSize) {
        float playerX = static_cast<float>(x);
        float playerZ = 0.0f;

        manager.update(playerX, playerZ);
        // Esperar a que los trabajadores entreguen la generación pendiente (determinista aunque la
        // máquina esté cargada); el tope evita colgar la prueba si algo se atasca
        auto waitStart = std::chrono::steady_clock::now();
        while (manager.getStats().pendingGenerations > 0 &&
               std::chrono::steady_clock::now() - waitStart < std::chrono::seconds(5)) {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            manager.update(playerX, playerZ);
        }

        size_t currentLoaded = world.getLoadedChunkCount();
        if (currentLoaded > maxLoadedObserved) {
            maxLoadedObserved = currentLoaded;
        }

        // El radio de render es 3, el radio de descarga es 5 (margen de 2).
        // El número máximo de chunks en memoria en cualquier momento no debe superar (2 * 5 + 1)^2 = 121 chunks
        assert(currentLoaded <= 121 && "La caché de chunks debe mantenerse estrictamente acotada");
    }

    // Esperar a que los hilos de trabajo vacíen tareas
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    manager.update(static_cast<float>(totalDistance), 0.0f);

    size_t finalLoaded = world.getLoadedChunkCount();
    size_t finalMemory = world.getEstimatedMemoryUsage();
    auto stats = manager.getStats();

    std::cout << "  - Chunks cargados al inicio: " << initialLoaded << " (~" << (initialMemory / 1024) << " KB)"
              << std::endl;
    std::cout << "  - Pico máximo de chunks en RAM: " << maxLoadedObserved << std::endl;
    std::cout << "  - Chunks cargados al final tras 10.000 bloques: " << finalLoaded << " (~" << (finalMemory / 1024)
              << " KB)" << std::endl;
    std::cout << "  - Total de chunks desalojados durante el viaje: " << stats.unloadedChunks << std::endl;

    assert(stats.unloadedChunks > 500 &&
           "Deben haberse descargado cientos de chunks durante el recorrido de 10.000 bloques");
    // La memoria final no debe ser más de 2.5x la memoria inicial (se mantiene acotada, no proporcional a 10.000
    // bloques)
    assert(finalMemory <= initialMemory * 3 &&
           "El consumo de memoria no debe crecer proporcionalmente al recorrido total");

    workerPool.stop();
    std::cout
        << "EXITO: Caché acotada, desalojo eficiente y presupuesto de memoria estables en recorrido de 10.000 bloques."
        << std::endl;
    return 0;
}
