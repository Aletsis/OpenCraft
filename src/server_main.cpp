#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <csignal>

#include "core/Log.hpp"
#include "sim/WorldSession.hpp"
#include "storage/WorldManager.hpp"
#include "storage/WorldStorage.hpp"
#include "network/NetworkServer.hpp"

namespace {
std::atomic<bool> g_running{true};

void handleSignal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_running = false;
    }
}
} // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    uint16_t port = OpenCraft::Net::DEFAULT_PORT;
    std::string worldName = "dedicated_server_world";
    uint64_t seed = 123456789ULL;
    uint32_t maxPlayers = 16;
    bool enableWhitelist = false;
    uint32_t saveIntervalSec = 30;
    uint64_t maxTicks = 0; // 0 = sin límite

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout
                << "OpenCraft Dedicated Server\n"
                << "Uso: " << argv[0] << " [opciones]\n"
                << "Opciones:\n"
                << "  --port <num>           Puerto UDP de escucha (por defecto: 35565)\n"
                << "  --world <nombre>       Nombre del mundo en saves/ (por defecto: dedicated_server_world)\n"
                << "  --seed <num>           Semilla de generación si el mundo es nuevo (por defecto: 123456789)\n"
                << "  --max-players <num>    Límite máximo de jugadores conectados (por defecto: 16)\n"
                << "  --whitelist            Activar lista blanca de acceso\n"
                << "  --save-interval <sec>  Intervalo de autoguardado en segundos (por defecto: 30)\n"
                << "  --ticks <num>          Simular solo ese número de ticks, guardar y salir (pruebas)\n"
                << "  --help                 Mostrar esta ayuda\n";
            return 0;
        } else if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--world" && i + 1 < argc) {
            worldName = argv[++i];
        } else if (arg == "--seed" && i + 1 < argc) {
            seed = std::stoull(argv[++i]);
        } else if (arg == "--max-players" && i + 1 < argc) {
            maxPlayers = static_cast<uint32_t>(std::stoi(argv[++i]));
        } else if (arg == "--whitelist") {
            enableWhitelist = true;
        } else if (arg == "--save-interval" && i + 1 < argc) {
            saveIntervalSec = static_cast<uint32_t>(std::stoi(argv[++i]));
        } else if (arg == "--ticks" && i + 1 < argc) {
            maxTicks = std::stoull(argv[++i]);
        }
    }

    OpenCraft::Log::info("=== Iniciando OpenCraft Dedicated Server ===");
    OpenCraft::Log::info("Puerto: {}, Mundo: '{}', Semilla: {}, Máx Jugadores: {}", port, worldName, seed, maxPlayers);

    OpenCraft::WorldManager worldManager("saves", "backups");
    OpenCraft::StorageError storageErr = OpenCraft::StorageError::None;
    auto storage = worldManager.openWorld(worldName, storageErr);
    if (!storage) {
        OpenCraft::Log::info("Creando nuevo mundo persistente '{}'...", worldName);
        std::string createdId = worldManager.createWorld(worldName, seed, 0);
        if (createdId.empty()) {
            OpenCraft::Log::error("Fallo al crear el mundo '{}'", worldName);
            return 1;
        }
        storage = worldManager.openWorld(createdId, storageErr);
        worldName = createdId;
    }

    OpenCraft::WorldSession session;
    session.init(3, seed);

    if (storage) {
        session.loadFromStorage(*storage);
        session.attachStorage(storage.get());
    }

    OpenCraft::Net::NetworkServer server;
    server.setWhitelistEnabled(enableWhitelist);
    if (!server.start(port, "OpenCraft Dedicated Server", maxPlayers)) {
        OpenCraft::Log::error("No se pudo iniciar el servidor en el puerto {}", port);
        return 1;
    }

    OpenCraft::Log::info("Servidor listo y escuchando conexiones a 20 Hz (Ctrl+C para salir limpiamente)...");

    auto lastTick = std::chrono::steady_clock::now();
    auto lastSave = std::chrono::steady_clock::now();
    constexpr auto tickDuration = std::chrono::milliseconds(50); // 20 Hz fijo

    uint64_t ticksRun = 0;
    while (g_running && (maxTicks == 0 || ticksRun < maxTicks)) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick);

        if (elapsed >= tickDuration) {
            lastTick = now;
            uint64_t nowMs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());

            session.update(0.05);
            server.update(session, nowMs);
            ++ticksRun;

            // Autoguardado periódico
            auto saveElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastSave);
            if (saveElapsed.count() >= saveIntervalSec) {
                lastSave = now;
                if (storage) {
                    session.saveToStorage(*storage);
                    OpenCraft::Log::info("Autoguardado completado en 'saves/{}'.", worldName);
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    OpenCraft::Log::info("Apagando servidor y guardando mundo...");
    server.stop();
    if (storage) {
        session.saveToStorage(*storage);
        worldManager.closeWorld(worldName);
    }
    OpenCraft::Log::info("Servidor dedicado cerrado correctamente ({} ticks).", ticksRun);
    return 0;
}
