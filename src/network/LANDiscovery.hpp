#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace OpenCraft::Net {

struct DiscoveredServer {
    std::string ip;
    uint16_t port = 0;
    std::string serverName;
    std::chrono::steady_clock::time_point lastSeen;
};

class LANDiscovery {
public:
    LANDiscovery();
    ~LANDiscovery();

    bool startBroadcasting(uint16_t gamePort, const std::string& serverName);
    bool startListening();

    void update();
    void stop();

    const std::vector<DiscoveredServer>& getDiscoveredServers() const noexcept { return m_servers; }

private:
    int m_broadcastSocket = -1;
    int m_listenSocket = -1;
    bool m_isBroadcasting = false;
    bool m_isListening = false;
    uint16_t m_gamePort = 0;
    std::string m_serverName;
    std::chrono::steady_clock::time_point m_lastBroadcastTime;

    std::vector<DiscoveredServer> m_servers;

    void sendBroadcastPacket();
    void pollListenPackets();
};

} // namespace OpenCraft::Net
