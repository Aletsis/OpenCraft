#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "network/Socket.hpp"

#include "network/NetworkProtocol.hpp"
#include "network/ReliableChannel.hpp"
#include "sim/WorldSession.hpp"
#include "storage/WorldStorage.hpp"

namespace OpenCraft::Net {

struct ServerClientSession {
    uint32_t sessionId = 0;
    uint32_t entityId = 0;
    std::string name;
    sockaddr_in address{};
    uint64_t lastHeartbeatMs = 0;
    uint64_t lastMoveMs = 0;    // última telemetría de movimiento aceptada
    uint32_t rejectedMoves = 0; // movimientos imposibles rechazados (diagnóstico anti-trampas)
    bool isConnected = false;
    bool isOperator = false;
    ReliableChannel channel;
};

class NetworkServer {
public:
    static constexpr uint64_t TIMEOUT_MS = 10000; // 10 segundos de inactividad

    NetworkServer();
    ~NetworkServer();

    bool start(uint16_t port, const std::string& serverName = "OpenCraft Server", uint32_t maxPlayers = 16);
    void stop();

    void update(WorldSession& session, uint64_t currentSimTimeMs);

    // Métodos de administración
    void setWhitelistEnabled(bool enabled) noexcept { m_whitelistEnabled = enabled; }
    bool isWhitelistEnabled() const noexcept { return m_whitelistEnabled; }
    void addWhitelist(const std::string& name) { m_whitelist.insert(name); }
    void removeWhitelist(const std::string& name) { m_whitelist.erase(name); }
    bool isWhitelisted(const std::string& name) const;

    void addOp(const std::string& name) { m_operators.insert(name); }
    void removeOp(const std::string& name) { m_operators.erase(name); }
    bool isOp(const std::string& name) const { return m_operators.find(name) != m_operators.end(); }

    bool kickPlayer(const std::string& name, const std::string& reason);
    void broadcastChat(const std::string& senderName, const std::string& text, bool isSystem = false);

    bool isRunning() const noexcept { return m_running; }
    uint16_t getPort() const noexcept { return m_port; }
    size_t getConnectedCount() const noexcept { return m_clients.size(); }
    const std::unordered_map<uint32_t, ServerClientSession>& getClients() const noexcept { return m_clients; }

    // Enviar deltas explícitos
    void broadcastBlockChange(int x, int y, int z, BlockStateId state, EntityId causeId = 0);
    void broadcastEntitySpawn(const EntitySpawnData& sp);
    void broadcastEntityDestroy(uint32_t entityId);

    // Integración de persistencia de jugador
    void savePlayer(WorldStorage& storage, uint32_t entityId);
    void savePlayer(WorldSession& session, WorldStorage& storage, uint32_t entityId);

private:
    bool m_running = false;
    SocketHandle m_socket = INVALID_SOCKET_HANDLE;
    uint16_t m_port = DEFAULT_PORT;
    std::string m_serverName;
    uint32_t m_maxPlayers = 16;
    uint32_t m_nextSessionId = 1;
    uint32_t m_nextPlayerEntityId = 10;
    uint32_t m_sequence = 0;

    bool m_whitelistEnabled = false;
    std::unordered_set<std::string> m_whitelist;
    std::unordered_set<std::string> m_operators;

    std::unordered_map<uint32_t, ServerClientSession> m_clients; // clave: sessionId
    std::unordered_map<std::string, uint32_t> m_playerNameToEntityId;
    WorldSession* m_activeSession = nullptr;

    ServerClientSession* findClientByAddr(const sockaddr_in& addr);
    ServerClientSession* findClientByName(const std::string& name);

    void sendRaw(const void* data, size_t size, const sockaddr_in& dest);
    void handleIncomingPacket(const uint8_t* buffer, size_t size, const sockaddr_in& senderAddr, WorldSession& session,
                              uint64_t nowMs);
    void executeAdminCommand(ServerClientSession* client, const std::string& cmd, WorldSession& session);
};

} // namespace OpenCraft::Net
