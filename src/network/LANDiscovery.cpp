#include "network/LANDiscovery.hpp"
#include "network/NetworkProtocol.hpp"
#include "core/Log.hpp"
#include "network/Socket.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace OpenCraft::Net {

constexpr uint16_t DISCOVERY_PORT = 35566;

LANDiscovery::LANDiscovery() = default;

LANDiscovery::~LANDiscovery() { stop(); }

bool LANDiscovery::startBroadcasting(uint16_t gamePort, const std::string& serverName) {
    m_gamePort = gamePort;
    m_serverName = serverName;

    m_broadcastSocket = openUdpSocket();
    if (!isValidSocket(m_broadcastSocket)) {
        Log::error("Fallo al crear socket para broadcast LAN");
        return false;
    }

    if (!setSocketOption(m_broadcastSocket, SOL_SOCKET, SO_BROADCAST, 1)) {
        Log::error("Fallo al configurar SO_BROADCAST");
        closeSocket(m_broadcastSocket);
        m_broadcastSocket = INVALID_SOCKET_HANDLE;
        return false;
    }

    m_isBroadcasting = true;
    m_lastBroadcastTime = std::chrono::steady_clock::now();
    Log::info("Anuncio de servidor en red local (LAN Broadcast) iniciado para: '{}'", m_serverName);
    return true;
}

bool LANDiscovery::startListening() {
    m_listenSocket = openUdpSocket();
    if (!isValidSocket(m_listenSocket)) {
        Log::error("Fallo al crear socket de escucha LAN");
        return false;
    }

    setSocketOption(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, 1);
#ifdef SO_REUSEPORT
    setSocketOption(m_listenSocket, SOL_SOCKET, SO_REUSEPORT, 1);
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (!bindSocket(m_listenSocket, addr)) {
        Log::warn("Puerto de escucha LAN ocupado o restringido");
        closeSocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET_HANDLE;
        return false;
    }

    m_isListening = true;
    Log::info("Búsqueda de partidas locales (LAN Discovery) activa en puerto {}", DISCOVERY_PORT);
    return true;
}

void LANDiscovery::stop() {
    closeSocket(m_broadcastSocket);
    m_broadcastSocket = INVALID_SOCKET_HANDLE;
    closeSocket(m_listenSocket);
    m_listenSocket = INVALID_SOCKET_HANDLE;
    m_isBroadcasting = false;
    m_isListening = false;
}

void LANDiscovery::sendBroadcastPacket() {
    if (!isValidSocket(m_broadcastSocket)) return;

    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(DISCOVERY_PORT);
    destAddr.sin_addr.s_addr = INADDR_BROADCAST;

    // Formato de payload: MAGIC;NAME;PORT
    std::string payload = std::string(DISCOVERY_MAGIC) + ";" + m_serverName + ";" + std::to_string(m_gamePort);

    sendTo(m_broadcastSocket, payload.data(), payload.size(), destAddr);
}

void LANDiscovery::pollListenPackets() {
    if (!isValidSocket(m_listenSocket)) return;

    char buffer[512];
    sockaddr_in senderAddr{};

    while (true) {
        long long bytes = recvFrom(m_listenSocket, buffer, sizeof(buffer) - 1, senderAddr);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        std::string msg(buffer);

        std::stringstream ss(msg);
        std::string magic, name, portStr;
        if (std::getline(ss, magic, ';') && std::getline(ss, name, ';') && std::getline(ss, portStr, ';')) {
            if (magic == DISCOVERY_MAGIC) {
                char ipBuf[INET_ADDRSTRLEN] = {};
                inet_ntop(AF_INET, &senderAddr.sin_addr, ipBuf, sizeof(ipBuf));
                std::string ip = ipBuf;
                int parsedPort = 0;
                try {
                    parsedPort = std::stoi(portStr);
                } catch (const std::exception&) {
                    continue; // anuncio malformado
                }
                if (parsedPort <= 0 || parsedPort > 65535) continue;
                uint16_t port = static_cast<uint16_t>(parsedPort);

                auto now = std::chrono::steady_clock::now();
                bool found = false;
                for (auto& s : m_servers) {
                    if (s.ip == ip && s.port == port) {
                        s.serverName = name;
                        s.lastSeen = now;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    m_servers.push_back({ip, port, name, now});
                    Log::info("¡Partida local descubierta!: '{}' en {}:{}", name, ip, port);
                }
            }
        }
    }

    // Purgar servidores que no envíen broadcast en más de 5 segundos
    auto now = std::chrono::steady_clock::now();
    for (auto it = m_servers.begin(); it != m_servers.end();) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->lastSeen).count();
        if (elapsed > 5) {
            Log::info("Partida local '{}' desconectada", it->serverName);
            it = m_servers.erase(it);
        } else {
            ++it;
        }
    }
}

void LANDiscovery::update() {
    auto now = std::chrono::steady_clock::now();

    if (m_isBroadcasting) {
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastBroadcastTime).count();
        if (elapsedMs >= 1500) { // Enviar cada 1.5s
            sendBroadcastPacket();
            m_lastBroadcastTime = now;
        }
    }

    if (m_isListening) {
        pollListenPackets();
    }
}

} // namespace OpenCraft::Net
