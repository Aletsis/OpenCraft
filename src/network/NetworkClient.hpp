#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "network/Socket.hpp"

#include "network/NetworkProtocol.hpp"
#include "network/ReliableChannel.hpp"
#include "sim/WorldSession.hpp"

namespace OpenCraft::Net {

enum class ClientConnectionState : uint8_t {
    Disconnected = 0,
    Connecting,
    Connected,
    ConnectionFailed
};

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    bool connect(const std::string& hostIp, uint16_t port, const std::string& playerName);
    void disconnect();

    void update(WorldSession& session, uint64_t currentSimTimeMs);

    // Envío de intenciones autoritativas
    void sendAction(const PlayerActionData& action, uint64_t nowMs);
    void sendInventoryTransaction(const InventoryTransactionData& tx, uint64_t nowMs);
    void sendChat(const std::string& text);

    ClientConnectionState getState() const noexcept { return m_state; }
    bool isConnected() const noexcept { return m_state == ClientConnectionState::Connected; }
    uint32_t getAssignedPlayerId() const noexcept { return m_assignedPlayerId; }
    uint32_t getSessionId() const noexcept { return m_sessionId; }
    const std::string& getDisconnectReason() const noexcept { return m_disconnectReason; }
    uint32_t getRttMs() const noexcept { return m_rttMs; }

    const std::vector<ChatMessageData>& getChatHistory() const noexcept { return m_chatHistory; }

private:
    SocketHandle m_socket = INVALID_SOCKET_HANDLE;
    sockaddr_in m_serverAddr{};
    std::string m_playerName;
    ClientConnectionState m_state = ClientConnectionState::Disconnected;
    std::string m_disconnectReason;

    uint32_t m_assignedPlayerId = 0;
    uint32_t m_sessionId = 0;
    uint32_t m_sequence = 0;
    uint32_t m_rttMs = 0;
    uint64_t m_lastPingSentMs = 0;
    uint64_t m_lastServerPacketMs = 0;
    uint64_t m_lastTeleTime = 0;

    ReliableChannel m_reliable;
    std::vector<ChatMessageData> m_chatHistory;

    void sendRaw(const void* data, size_t size);
    void handleIncomingPacket(const uint8_t* buffer, size_t size, WorldSession& session, uint64_t nowMs);
};

} // namespace OpenCraft::Net
