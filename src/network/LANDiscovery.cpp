#include "network/LANDiscovery.hpp"
#include "network/NetworkProtocol.hpp"
#include "core/Log.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sstream>

namespace OpenCraft::Net {

constexpr uint16_t DISCOVERY_PORT = 35566;

LANDiscovery::LANDiscovery() = default;

LANDiscovery::~LANDiscovery() { stop(); }

static bool setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

bool LANDiscovery::startBroadcasting(uint16_t gamePort, const std::string& serverName) {
    m_gamePort = gamePort;
    m_serverName = serverName;

    m_broadcastSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_broadcastSocket < 0) {
        Log::error("Fallo al crear socket para broadcast LAN");
        return false;
    }

    int broadcastEnable = 1;
    if (setsockopt(m_broadcastSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        Log::error("Fallo al configurar SO_BROADCAST");
        close(m_broadcastSocket);
        m_broadcastSocket = -1;
        return false;
    }

    setNonBlocking(m_broadcastSocket);
    m_isBroadcasting = true;
    m_lastBroadcastTime = std::chrono::steady_clock::now();
    Log::info("Anuncio de servidor en red local (LAN Broadcast) iniciado para: '{}'", m_serverName);
    return true;
}

bool LANDiscovery::startListening() {
    m_listenSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_listenSocket < 0) {
        Log::error("Fallo al crear socket de escucha LAN");
        return false;
    }

    int reuse = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DISCOVERY_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        Log::warn("Puerto de escucha LAN ocupado o restringido");
        close(m_listenSocket);
        m_listenSocket = -1;
        return false;
    }

    setNonBlocking(m_listenSocket);
    m_isListening = true;
    Log::info("Búsqueda de partidas locales (LAN Discovery) activa en puerto {}", DISCOVERY_PORT);
    return true;
}

void LANDiscovery::stop() {
    if (m_broadcastSocket >= 0) {
        close(m_broadcastSocket);
        m_broadcastSocket = -1;
    }
    if (m_listenSocket >= 0) {
        close(m_listenSocket);
        m_listenSocket = -1;
    }
    m_isBroadcasting = false;
    m_isListening = false;
}

void LANDiscovery::sendBroadcastPacket() {
    if (m_broadcastSocket < 0) return;

    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(DISCOVERY_PORT);
    destAddr.sin_addr.s_addr = INADDR_BROADCAST;

    // Formato de payload: MAGIC;NAME;PORT
    std::string payload = std::string(DISCOVERY_MAGIC) + ";" + m_serverName + ";" + std::to_string(m_gamePort);

    sendto(m_broadcastSocket, payload.c_str(), payload.length(), 0, reinterpret_cast<sockaddr*>(&destAddr),
           sizeof(destAddr));
}

void LANDiscovery::pollListenPackets() {
    if (m_listenSocket < 0) return;

    char buffer[512];
    sockaddr_in senderAddr{};
    socklen_t addrLen = sizeof(senderAddr);

    while (true) {
        ssize_t bytes =
            recvfrom(m_listenSocket, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&senderAddr), &addrLen);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        std::string msg(buffer);

        std::stringstream ss(msg);
        std::string magic, name, portStr;
        if (std::getline(ss, magic, ';') && std::getline(ss, name, ';') && std::getline(ss, portStr, ';')) {
            if (magic == DISCOVERY_MAGIC) {
                std::string ip = inet_ntoa(senderAddr.sin_addr);
                uint16_t port = static_cast<uint16_t>(std::stoi(portStr));

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
