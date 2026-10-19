#pragma once

#include <cstdint>
#include <vector>
#include <deque>
#include <unordered_set>
#include <map>
#include <functional>
#include <random>
#include <netinet/in.h>
#include "network/NetworkProtocol.hpp"

namespace OpenCraft::Net {

struct OutgoingReliablePacket {
    uint32_t seq = 0;
    std::vector<uint8_t> data;
    uint64_t sendTimeMs = 0;
    uint32_t retries = 0;
};

class ReliableChannel {
public:
    static constexpr uint64_t DEFAULT_RTO_MS = 150;
    static constexpr uint32_t MAX_RETRIES = 10;

    ReliableChannel();
    ~ReliableChannel() = default;

    void reset();

    // Emisión fiable
    uint32_t prepareReliablePacket(std::vector<uint8_t>& outPayload, const void* payloadData, size_t payloadSize,
                                   uint64_t nowMs);
    void onAckReceived(uint32_t ackSeq);
    std::vector<std::vector<uint8_t>> checkRetransmissions(uint64_t nowMs);

    // Recepción fiable
    // Retorna true si es un paquete fiable nuevo que debe procesarse o fue encolado para ordenamiento
    bool onPacketReceived(uint32_t seq, const uint8_t* payload, size_t size,
                          std::vector<std::vector<uint8_t>>& outReadyPackets);

    // Telemetría no fiable (descarte de paquetes obsoletos)
    bool checkUnreliableSequence(uint32_t seq);

    bool isDisconnected() const noexcept { return m_disconnected; }
    size_t getUnackedCount() const noexcept { return m_unackedQueue.size(); }
    uint32_t getNextSendSeq() const noexcept { return m_nextSendSeq; }

private:
    uint32_t m_nextSendSeq = 1;
    uint32_t m_nextExpectedRecvSeq = 1;
    uint32_t m_highestUnreliableSeq = 0;
    bool m_disconnected = false;

    // Paquetes pendientes de confirmación
    std::deque<OutgoingReliablePacket> m_unackedQueue;

    // Paquetes recibidos fuera de orden esperando secuencia
    std::map<uint32_t, std::vector<uint8_t>> m_outOfOrderBuffer;

    // Historial de secuencias recibidas para deduplicación
    std::unordered_set<uint32_t> m_receivedSeqs;
};

// ==============================================================================
// Simulador de Red Degradada (RTT, pérdida, duplicados, reordenamiento)
// ==============================================================================

struct SimulatedPacket {
    std::vector<uint8_t> data;
    sockaddr_in destAddr{};
    uint64_t deliverTimeMs = 0;
};

class NetworkDegradationSimulator {
public:
    NetworkDegradationSimulator(uint32_t seed = 42);

    void setLatencyMs(uint32_t oneWayLatencyMs) noexcept { m_oneWayLatencyMs = oneWayLatencyMs; }
    void setPacketLossPercent(float percent) noexcept { m_packetLossPercent = percent; }
    void setDuplicationPercent(float percent) noexcept { m_duplicationPercent = percent; }
    void setReorderPercent(float percent) noexcept { m_reorderPercent = percent; }

    // Enviar paquete a través del simulador
    void enqueuePacket(const void* data, size_t size, const sockaddr_in& dest, uint64_t currentSimTimeMs);

    // Entregar los paquetes cuya hora de entrega ha llegado
    void update(uint64_t currentSimTimeMs,
                const std::function<void(const std::vector<uint8_t>&, const sockaddr_in&)>& deliverFunc);

    size_t getPendingCount() const noexcept { return m_queue.size(); }
    void clear() { m_queue.clear(); }

private:
    uint32_t m_oneWayLatencyMs = 0;   // 50ms para 100ms RTT
    float m_packetLossPercent = 0.0f; // 0.0 a 100.0
    float m_duplicationPercent = 0.0f;
    float m_reorderPercent = 0.0f;

    std::mt19937 m_rng;
    std::deque<SimulatedPacket> m_queue;
};

} // namespace OpenCraft::Net
