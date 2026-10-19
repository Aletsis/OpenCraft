#include "network/NetworkManager.hpp"
#include "network/PacketValidator.hpp"
#include "sim/WorldSession.hpp"
#include "world/World.hpp"
#include "core/Log.hpp"

#include <chrono>

namespace OpenCraft {

NetworkManager::NetworkManager() = default;

NetworkManager::~NetworkManager() { disconnect(); }

bool NetworkManager::startHost(uint16_t port, const std::string& serverName) {
    disconnect();

    m_server = std::make_unique<Net::NetworkServer>();
    if (!m_server->start(port, serverName)) {
        m_server.reset();
        return false;
    }

    m_role = NetworkRole::Host;
    m_localPlayerID = 0;
    m_discovery.startBroadcasting(port, serverName);
    Log::info("NetworkManager: Host iniciado en puerto {}", port);
    return true;
}

bool NetworkManager::connectToHost(const std::string& hostIp, uint16_t port, const std::string& playerName) {
    disconnect();

    m_client = std::make_unique<Net::NetworkClient>();
    if (!m_client->connect(hostIp, port, playerName)) {
        m_client.reset();
        return false;
    }

    m_role = NetworkRole::Client;
    Log::info("NetworkManager: Conectando a {}:{} como '{}'", hostIp, port, playerName);
    return true;
}

void NetworkManager::disconnect() {
    m_discovery.stop();
    if (m_server) {
        m_server->stop();
        m_server.reset();
    }
    if (m_client) {
        m_client->disconnect();
        m_client.reset();
    }
    m_remotePlayers.clear();
    m_role = NetworkRole::Singleplayer;
}

void NetworkManager::sendBlockModify(int x, int y, int z, uint8_t blockID, uint8_t action) {
    if (m_role == NetworkRole::Singleplayer) return;

    if (m_role == NetworkRole::Host && m_server) {
        BlockStateId state =
            (action == 0) ? AIR_BLOCK_STATE : BlockRegistry::global().fromLegacyId(static_cast<BlockID>(blockID));
        m_server->broadcastBlockChange(x, y, z, state, 0);
    } else if (m_role == NetworkRole::Client && m_client) {
        Net::PlayerActionData act;
        act.action = (action == 0) ? Net::PlayerActionType::BreakBlock : Net::PlayerActionType::PlaceBlock;
        act.x = x;
        act.y = y;
        act.z = z;
        act.blockState = BlockRegistry::global().fromLegacyId(static_cast<BlockID>(blockID));
        uint64_t nowMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count());
        m_client->sendAction(act, nowMs);
    }
}

void NetworkManager::update(WorldSession& session) {
    if (!m_enabled) return;
    m_discovery.update();

    uint64_t nowMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());

    if (m_role == NetworkRole::Host && m_server) {
        m_server->update(session, nowMs);
    } else if (m_role == NetworkRole::Client && m_client) {
        m_client->update(session, nowMs);
    }
}

} // namespace OpenCraft
