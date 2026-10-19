#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "network/Socket.hpp"
#include "network/NetworkProtocol.hpp"
#include "network/LANDiscovery.hpp"
#include "network/NetworkServer.hpp"
#include "network/NetworkClient.hpp"
#include "math/Math.hpp"

namespace OpenCraft {

class World;
class WorldSession;

struct RemotePlayer {
    uint32_t id = 0;
    std::string name;
    Math::Vec3 position{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    sockaddr_in address{};
};

enum class NetworkRole {
    Singleplayer,
    Host,
    Client
};

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool startHost(uint16_t port, const std::string& serverName);
    bool connectToHost(const std::string& hostIp, uint16_t port, const std::string& playerName);
    void disconnect();
    void update(WorldSession& session);
    void sendBlockModify(int x, int y, int z, uint8_t blockID, uint8_t action);

    bool isEnabled() const noexcept { return m_enabled; }
    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }

    NetworkRole getRole() const noexcept { return m_role; }
    Net::LANDiscovery& getDiscovery() { return m_discovery; }

    const std::unordered_map<uint32_t, RemotePlayer>& getRemotePlayers() const noexcept { return m_remotePlayers; }

    Net::NetworkServer* getServer() noexcept { return m_server.get(); }
    Net::NetworkClient* getClient() noexcept { return m_client.get(); }

private:
    bool m_enabled = true; // Habilitado en Fase 09
    NetworkRole m_role = NetworkRole::Singleplayer;
    Net::SocketHandle m_socket = Net::INVALID_SOCKET_HANDLE;
    sockaddr_in m_serverAddr{};
    uint32_t m_localPlayerID = 0;
    uint32_t m_nextPlayerID = 1;
    uint32_t m_sequence = 0;

    Net::LANDiscovery m_discovery;
    std::unordered_map<uint32_t, RemotePlayer> m_remotePlayers;

    std::unique_ptr<Net::NetworkServer> m_server;
    std::unique_ptr<Net::NetworkClient> m_client;
    void updateHost(WorldSession& session);
    void updateClient(WorldSession& session);
};

} // namespace OpenCraft
