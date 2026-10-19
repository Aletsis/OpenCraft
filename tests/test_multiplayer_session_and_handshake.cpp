#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <filesystem>
#include "storage/WorldStorage.hpp"
#include "network/NetworkServer.hpp"
#include "network/NetworkClient.hpp"
#include "network/PacketValidator.hpp"
#include "sim/WorldSession.hpp"

using namespace OpenCraft;
using namespace OpenCraft::Net;

int main() {
    std::cout << "--- [F09 Test 1] Sesión Multijugador, Handshake y Comandos de Administración ---" << std::endl;

    WorldSession serverSession;
    serverSession.init(2, 42);

    NetworkServer server;
    constexpr uint16_t testPort = 35601;
    bool serverStarted = server.start(testPort, "Test Handshake Server", 4);
    assert(serverStarted && "El servidor debe iniciar en el puerto de pruebas");
    (void)serverStarted;

    uint64_t simTimeMs = 1000;

    // 1. Handshake exitoso con versión canónica de protocolo
    {
        NetworkClient clientA;
        bool ok = clientA.connect("127.0.0.1", testPort, "Alice");
        assert(ok && "El cliente debe enviar handshake");
        (void)ok;

        // Simular ticks de red en loopback
        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            clientA.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        assert(clientA.isConnected() && "Alice debe conectarse exitosamente");
        assert(clientA.getAssignedPlayerId() >= 10 && "Debe asignarse un EntityId válido");
        assert(clientA.getSessionId() > 0 && "Debe asignarse un SessionId único");
        assert(server.getConnectedCount() == 1 && "El servidor debe registrar 1 cliente");

        // Ping / Pong
        simTimeMs += 50;
        server.update(serverSession, simTimeMs);
        clientA.update(serverSession, simTimeMs);

        // Desconexión limpia
        clientA.disconnect();
        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        assert(server.getConnectedCount() == 0 && "El servidor debe remover al cliente tras desconexión");
    }

    // 2. Rechazo explícito por versión incompatible de protocolo
    {
        // Enviar paquete Handshake con versión antigua (p. ej. 99)
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        assert(sock >= 0);

        sockaddr_in saddr{};
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(testPort);
        inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr);

        ByteWriter w;
        serializeHandshake(w, 1, "OldBob", 99); // Versión incompatible
        sendto(sock, w.data(), w.size(), 0, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr));

        simTimeMs += 10;
        server.update(serverSession, simTimeMs);

        // Recibir respuesta de rechazo
        uint8_t recvBuf[512];
        socklen_t slen = sizeof(saddr);
        ssize_t n = recvfrom(sock, recvBuf, sizeof(recvBuf), 0, reinterpret_cast<sockaddr*>(&saddr), &slen);
        assert(n > 0 && "El servidor debe responder al cliente incompatible");

        ByteReader r(recvBuf, static_cast<size_t>(n));
        PacketType ptype = PacketType::Handshake;
        uint32_t pseq = 0;
        bool hdrOk = deserializeHeader(r, ptype, pseq);
        assert(hdrOk && ptype == PacketType::HandshakeReject);
        (void)hdrOk;

        HandshakeRejectData rej;
        bool rejOk = deserializeHandshakeReject(r, rej);
        assert(rejOk);
        (void)rejOk;
        assert(rej.reason == RejectReason::IncompatibleVersion);
        assert(!rej.message.empty());

        close(sock);
        std::cout << "  [OK] Rechazo por versión incompatible verificado: '" << rej.message << "'" << std::endl;
    }

    // 3. Control de Lista Blanca (Whitelist) y Administración (/kick, /op)
    {
        server.setWhitelistEnabled(true);
        server.addWhitelist("WhitelistedPlayer");

        NetworkClient intruder;
        intruder.connect("127.0.0.1", testPort, "Intruder");

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            intruder.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        assert(!intruder.isConnected() && "Jugador no en whitelist debe ser rechazado");
        assert(intruder.getState() == ClientConnectionState::ConnectionFailed);
        assert(intruder.getDisconnectReason().find("blanca") != std::string::npos);

        // Conectar jugador permitido
        NetworkClient allowed;
        allowed.connect("127.0.0.1", testPort, "WhitelistedPlayer");

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            allowed.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        assert(allowed.isConnected() && "Jugador en whitelist debe conectarse con éxito");

        // Expulsión administrativa (Kick)
        bool kicked = server.kickPlayer("WhitelistedPlayer", "Violacion de reglas");
        assert(kicked && "Kick debe reportar éxito");
        (void)kicked;

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            allowed.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        assert(!allowed.isConnected() && "El cliente expulsado debe quedar desconectado");

        server.setWhitelistEnabled(false);
    }

    // 4. Timeout por inactividad prolongada (>10s)
    {
        NetworkClient ghost;
        ghost.connect("127.0.0.1", testPort, "Ghost");

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            ghost.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        assert(ghost.isConnected());
        assert(server.getConnectedCount() == 1);

        // Avanzar el tiempo simulado en 11 segundos sin que el cliente envíe paquetes
        simTimeMs += 11000;
        server.update(serverSession, simTimeMs);

        assert(server.getConnectedCount() == 0 && "El servidor debe expulsar clientes tras 10s de silencio");
    }

    // 5. Persistencia de perfiles remotos y reconexión con inventario preservado (A03 / F09)
    {
        std::filesystem::path tempWorldPath = "/tmp/opencraft-test-multiplayer-storage";
        std::filesystem::remove_all(tempWorldPath);
        WorldStorage storage(tempWorldPath);
        assert(storage.initializeWorld("MultiplayerTest", 123) == StorageError::None);

        NetworkClient clientAlice;
        clientAlice.connect("127.0.0.1", testPort, "Alice");

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            clientAlice.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        assert(clientAlice.isConnected() && "Alice debe conectarse");
        uint32_t aliceEid = clientAlice.getAssignedPlayerId();
        assert(aliceEid >= 10);

        PlayerEntity* aliceEntity = serverSession.getPlayerById(aliceEid);
        assert(aliceEntity != nullptr);

        // Otorgar 17 diamantes al jugador remoto
        ItemId diamondId = ItemRegistry::global().getId("opencraft:diamond");
        aliceEntity->inventory.setSlot(0, ItemStack(diamondId, 17));
        assert(aliceEntity->inventory.getSlot(0).count == 17);

        // Guardar explícitamente el jugador en almacenamiento persistente
        server.savePlayer(serverSession, storage, aliceEid);
        assert(std::filesystem::exists(storage.getPlayerPath(aliceEid)) && "El archivo player_<id>.dat debe existir");

        // Guardar la sesión completa
        assert(serverSession.saveToStorage(storage) == StorageError::None);

        // Desconexión de Alice
        clientAlice.disconnect();
        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        // Reconexión con el mismo nombre "Alice": debe conservar la misma identidad estable y su inventario
        NetworkClient clientAliceReconnected;
        clientAliceReconnected.connect("127.0.0.1", testPort, "Alice");

        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            clientAliceReconnected.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        assert(clientAliceReconnected.isConnected() && "Alice debe reconectarse con éxito");
        assert(clientAliceReconnected.getAssignedPlayerId() == aliceEid &&
               "Debe reasignarse la misma identidad estable EntityId");

        PlayerEntity* reconnectedEntity = serverSession.getPlayerById(aliceEid);
        assert(reconnectedEntity != nullptr);
        assert(reconnectedEntity->inventory.getSlot(0).count == 17 &&
               "El inventario de 17 diamantes debe haberse preservado al reconectar");

        clientAliceReconnected.disconnect();
        for (int i = 0; i < 5; ++i) {
            simTimeMs += 10;
            server.update(serverSession, simTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }

        // Cargar en una WorldSession totalmente nueva desde disco
        WorldSession restoredSession;
        restoredSession.init(0, 123);
        assert(restoredSession.loadFromStorage(storage) == StorageError::None);

        PlayerEntity* loadedAlice = restoredSession.getPlayerById(aliceEid);
        assert(loadedAlice != nullptr && "El jugador 42/Alice debe restaurarse al cargar el mundo");
        assert(loadedAlice->inventory.getSlot(0).count == 17 &&
               "Los 17 diamantes deben recuperarse del archivo player_<id>.dat");
        assert(restoredSession.getPlayerName(aliceEid) == "Alice" && "El nombre del jugador debe coincidir");

        std::filesystem::remove_all(tempWorldPath);
        std::cout << "  [OK] Persistencia multijugador y reconexión con inventario preservado verificado." << std::endl;
    }

    server.stop();
    std::cout << "EXITO: Sesión multijugador, handshake, rechazos y administración verificados limpiamente."
              << std::endl;
    return 0;
}
