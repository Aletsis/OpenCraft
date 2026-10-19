#include "network/ReliableChannel.hpp"
#include <algorithm>

namespace OpenCraft::Net {

ReliableChannel::ReliableChannel() = default;

void ReliableChannel::reset() {
    m_nextSendSeq = 1;
    m_nextExpectedRecvSeq = 1;
    m_highestUnreliableSeq = 0;
    m_disconnected = false;
    m_unackedQueue.clear();
    m_outOfOrderBuffer.clear();
    m_receivedSeqs.clear();
}

uint32_t ReliableChannel::prepareReliablePacket(std::vector<uint8_t>& outPayload, const void* payloadData,
                                                size_t payloadSize, uint64_t nowMs) {
    uint32_t seq = m_nextSendSeq++;
    outPayload.assign(reinterpret_cast<const uint8_t*>(payloadData),
                      reinterpret_cast<const uint8_t*>(payloadData) + payloadSize);

    OutgoingReliablePacket pkt;
    pkt.seq = seq;
    pkt.data = outPayload;
    pkt.sendTimeMs = nowMs;
    pkt.retries = 0;
    m_unackedQueue.push_back(std::move(pkt));

    return seq;
}

void ReliableChannel::onAckReceived(uint32_t ackSeq) {
    for (auto it = m_unackedQueue.begin(); it != m_unackedQueue.end(); ++it) {
        if (it->seq == ackSeq) {
            m_unackedQueue.erase(it);
            break;
        }
    }
}

std::vector<std::vector<uint8_t>> ReliableChannel::checkRetransmissions(uint64_t nowMs) {
    std::vector<std::vector<uint8_t>> toRetransmit;
    for (auto& pkt : m_unackedQueue) {
        if (nowMs >= pkt.sendTimeMs + DEFAULT_RTO_MS) {
            pkt.retries++;
            if (pkt.retries > MAX_RETRIES) {
                m_disconnected = true;
            } else {
                pkt.sendTimeMs = nowMs;
                toRetransmit.push_back(pkt.data);
            }
        }
    }
    return toRetransmit;
}

bool ReliableChannel::onPacketReceived(uint32_t seq, const uint8_t* payload, size_t size,
                                       std::vector<std::vector<uint8_t>>& outReadyPackets) {
    // Si ya lo procesamos antes, es un duplicado
    if (seq < m_nextExpectedRecvSeq || m_receivedSeqs.find(seq) != m_receivedSeqs.end()) {
        return false;
    }

    m_receivedSeqs.insert(seq);

    if (seq == m_nextExpectedRecvSeq) {
        outReadyPackets.emplace_back(payload, payload + size);
        m_nextExpectedRecvSeq++;

        // Despachar los que estaban en buffer fuera de orden si ahora están contiguos
        while (true) {
            auto it = m_outOfOrderBuffer.find(m_nextExpectedRecvSeq);
            if (it == m_outOfOrderBuffer.end()) break;
            outReadyPackets.push_back(std::move(it->second));
            m_outOfOrderBuffer.erase(it);
            m_nextExpectedRecvSeq++;
        }
        return true;
    }

    if (seq > m_nextExpectedRecvSeq) {
        // Paquete adelantado: encolar en el buffer ordenado
        m_outOfOrderBuffer[seq] = std::vector<uint8_t>(payload, payload + size);
        return true;
    }

    return false;
}

bool ReliableChannel::checkUnreliableSequence(uint32_t seq) {
    if (seq > m_highestUnreliableSeq || (m_highestUnreliableSeq > 0xFFFFFF00 && seq < 0xFF)) {
        m_highestUnreliableSeq = seq;
        return true;
    }
    return false; // Paquete de telemetría desactualizado, descartar
}

// ==============================================================================
// Simulador de Red Degradada
// ==============================================================================

NetworkDegradationSimulator::NetworkDegradationSimulator(uint32_t seed) : m_rng(seed) {}

void NetworkDegradationSimulator::enqueuePacket(const void* data, size_t size, const sockaddr_in& dest,
                                                uint64_t currentSimTimeMs) {
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);

    // 1. Simulación de pérdida de paquetes
    if (m_packetLossPercent > 0.0f && dist(m_rng) < m_packetLossPercent) {
        return; // Paquete perdido intencionalmente
    }

    uint64_t delay = m_oneWayLatencyMs;

    // 2. Simulación de reordenamiento / jitter
    if (m_reorderPercent > 0.0f && dist(m_rng) < m_reorderPercent) {
        delay += static_cast<uint64_t>(dist(m_rng) * 0.5f); // Añade retraso variable
    }

    SimulatedPacket pkt;
    pkt.data.assign(reinterpret_cast<const uint8_t*>(data), reinterpret_cast<const uint8_t*>(data) + size);
    pkt.destAddr = dest;
    pkt.deliverTimeMs = currentSimTimeMs + delay;
    m_queue.push_back(pkt);

    // 3. Simulación de duplicación de paquetes
    if (m_duplicationPercent > 0.0f && dist(m_rng) < m_duplicationPercent) {
        SimulatedPacket dup = pkt;
        dup.deliverTimeMs += 5; // Llega 5ms más tarde
        m_queue.push_back(std::move(dup));
    }
}

void NetworkDegradationSimulator::update(
    uint64_t currentSimTimeMs,
    const std::function<void(const std::vector<uint8_t>&, const sockaddr_in&)>& deliverFunc) {
    for (auto it = m_queue.begin(); it != m_queue.end();) {
        if (currentSimTimeMs >= it->deliverTimeMs) {
            deliverFunc(it->data, it->destAddr);
            it = m_queue.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace OpenCraft::Net
