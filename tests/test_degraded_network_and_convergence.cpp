#include <iostream>
#include <cassert>
#include <vector>
#include "network/ReliableChannel.hpp"
#include "network/NetworkProtocol.hpp"
#include "sim/WorldSession.hpp"

using namespace OpenCraft;
using namespace OpenCraft::Net;

int main() {
    std::cout
        << "--- [F09 Test 4] Red Degradada (100 ms RTT, 2% Pérdida, Duplicación y Reordenamiento) y Convergencia ---"
        << std::endl;

    // 1. Configurar simulador de red degradada
    NetworkDegradationSimulator clientToServerNet(12345);
    NetworkDegradationSimulator serverToClientNet(54321);

    // 50 ms unidireccional = 100 ms RTT
    clientToServerNet.setLatencyMs(50);
    clientToServerNet.setPacketLossPercent(2.0f);  // 2% de pérdida
    clientToServerNet.setDuplicationPercent(2.0f); // 2% de duplicados
    clientToServerNet.setReorderPercent(5.0f);     // 5% de reordenamiento

    serverToClientNet.setLatencyMs(50);
    serverToClientNet.setPacketLossPercent(2.0f);
    serverToClientNet.setDuplicationPercent(2.0f);
    serverToClientNet.setReorderPercent(5.0f);

    ReliableChannel clientChannel;
    ReliableChannel serverChannel;

    sockaddr_in fakeServerAddr{};
    fakeServerAddr.sin_port = htons(35565);
    sockaddr_in fakeClientAddr{};
    fakeClientAddr.sin_port = htons(35566);

    uint64_t simTimeMs = 1000;
    constexpr uint32_t totalMessages = 100;
    std::vector<uint32_t> serverDeliveredSeqs;
    std::vector<uint32_t> clientDeliveredAcks;

    // 2. Transmitir ráfagas de 100 mensajes fiables consecutivos a través del enlace degradado
    for (uint32_t msgIdx = 1; msgIdx <= totalMessages; ++msgIdx) {
        ByteWriter w;
        w.writeUint32(msgIdx);
        w.writeString("CriticalPayload_" + std::to_string(msgIdx));

        std::vector<uint8_t> pktPayload;
        uint32_t seq = clientChannel.prepareReliablePacket(pktPayload, w.data(), w.size(), simTimeMs);

        // Enviar a través del simulador de degradación
        clientToServerNet.enqueuePacket(pktPayload.data(), pktPayload.size(), fakeServerAddr, simTimeMs);

        // Avanzar el tiempo y actualizar retransmisiones y entregas
        for (int step = 0; step < 5; ++step) {
            simTimeMs += 10;

            // Retransmisiones del cliente si el RTO expiró
            auto retransmissions = clientChannel.checkRetransmissions(simTimeMs);
            for (const auto& repkt : retransmissions) {
                clientToServerNet.enqueuePacket(repkt.data(), repkt.size(), fakeServerAddr, simTimeMs);
            }

            // Entregar paquetes que hayan cumplido su latencia desde Cliente hacia Servidor
            clientToServerNet.update(simTimeMs, [&](const std::vector<uint8_t>& data, const sockaddr_in&) {
                ByteReader r(data.data(), data.size());
                uint32_t readSeq = 0;
                r.readUint32(readSeq);

                std::vector<std::vector<uint8_t>> ready;
                bool ok = serverChannel.onPacketReceived(readSeq, data.data(), data.size(), ready);
                if (ok) {
                    for (const auto& p : ready) {
                        ByteReader pr(p.data(), p.size());
                        uint32_t s = 0;
                        pr.readUint32(s);
                        serverDeliveredSeqs.push_back(s);
                    }
                }

                // El servidor envía ACK al cliente a través del enlace degradado
                ByteWriter ackW;
                ackW.writeUint32(readSeq);
                serverToClientNet.enqueuePacket(ackW.data(), ackW.size(), fakeClientAddr, simTimeMs);
            });

            // Entregar ACKs desde Servidor hacia Cliente
            serverToClientNet.update(simTimeMs, [&](const std::vector<uint8_t>& data, const sockaddr_in&) {
                ByteReader r(data.data(), data.size());
                uint32_t ackSeq = 0;
                if (r.readUint32(ackSeq)) {
                    clientChannel.onAckReceived(ackSeq);
                    clientDeliveredAcks.push_back(ackSeq);
                }
            });
        }
    }

    // 3. Fase de convergencia y estabilización (hasta 10 segundos adicionales de tiempo simulado)
    while ((serverDeliveredSeqs.size() < totalMessages || clientChannel.getUnackedCount() > 0) && simTimeMs < 30000) {
        simTimeMs += 20;

        auto retransmissions = clientChannel.checkRetransmissions(simTimeMs);
        for (const auto& repkt : retransmissions) {
            clientToServerNet.enqueuePacket(repkt.data(), repkt.size(), fakeServerAddr, simTimeMs);
        }

        clientToServerNet.update(simTimeMs, [&](const std::vector<uint8_t>& data, const sockaddr_in&) {
            ByteReader r(data.data(), data.size());
            uint32_t readSeq = 0;
            r.readUint32(readSeq);

            std::vector<std::vector<uint8_t>> ready;
            bool ok = serverChannel.onPacketReceived(readSeq, data.data(), data.size(), ready);
            if (ok) {
                for (const auto& p : ready) {
                    ByteReader pr(p.data(), p.size());
                    uint32_t s = 0;
                    pr.readUint32(s);
                    serverDeliveredSeqs.push_back(s);
                }
            }

            ByteWriter ackW;
            ackW.writeUint32(readSeq);
            serverToClientNet.enqueuePacket(ackW.data(), ackW.size(), fakeClientAddr, simTimeMs);
        });

        serverToClientNet.update(simTimeMs, [&](const std::vector<uint8_t>& data, const sockaddr_in&) {
            ByteReader r(data.data(), data.size());
            uint32_t ackSeq = 0;
            if (r.readUint32(ackSeq)) {
                clientChannel.onAckReceived(ackSeq);
            }
        });
    }

    // Comprobar que todos los 100 mensajes fueron entregados
    assert(serverDeliveredSeqs.size() == totalMessages &&
           "Todos los mensajes fiables deben llegar a pesar de pérdidas, duplicados y reordenamiento");

    // Comprobar orden estricto de entrega: 1, 2, 3, ..., 100
    for (size_t i = 0; i < totalMessages; ++i) {
        assert(serverDeliveredSeqs[i] == (i + 1) && "Los mensajes deben entregarse en orden secuencial estricto");
    }

    // Comprobar que no quedan paquetes sin confirmar
    assert(clientChannel.getUnackedCount() == 0 && "Todos los paquetes fiables deben quedar confirmados por ACK");
    std::cout << "  [OK] 100/100 mensajes recuperados en orden secuencial exacto bajo red degradada." << std::endl;

    // 4. Convergencia de estado (`computeStateHash()`) entre servidor y cliente
    {
        WorldSession serverSession;
        serverSession.init(2, 777);

        WorldSession clientSession;
        clientSession.init(2, 777);

        // Aplicar mutaciones deterministas a ambos extremos
        for (int i = 0; i < 20; ++i) {
            serverSession.executeCommand(
                PlaceBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                  .x = 8 + (i % 5),
                                  .y = 66,
                                  .z = 8 + (i / 5),
                                  .blockState = BlockRegistry::global().fromLegacyId(BlockID::Cobblestone)});

            clientSession.executeCommand(
                PlaceBlockCommand{.entityId = LOCAL_PLAYER_ENTITY_ID,
                                  .x = 8 + (i % 5),
                                  .y = 66,
                                  .z = 8 + (i / 5),
                                  .blockState = BlockRegistry::global().fromLegacyId(BlockID::Cobblestone)});
        }

        uint64_t serverHash = serverSession.computeStateHash();
        uint64_t clientHash = clientSession.computeStateHash();

        assert(serverHash == clientHash && "Los hashes de estado de simulación deben coincidir de forma determinista");
        std::cout << "  [OK] Convergencia de estado verificada: serverHash == clientHash (0x" << std::hex << serverHash
                  << std::dec << ")." << std::endl;
    }

    std::cout << "EXITO: Red degradada y convergencia de estado superadas sin divergencia permanente." << std::endl;
    return 0;
}
