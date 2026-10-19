#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace OpenCraft {

struct ProgressionNode {
    std::string id;
    std::string description;
    std::vector<std::string> dependencies; // Nodos de los que depende directamente
};

class ProgressionGraph {
public:
    ProgressionGraph();
    ~ProgressionGraph() = default;

    static ProgressionGraph& canonical();

    void addNode(std::string id, std::string description, std::vector<std::string> dependencies);

    bool hasNode(const std::string& id) const;
    const ProgressionNode* getNode(const std::string& id) const;

    // Validación formal de DAG (Grafo Dirigido Acíclico sin dependencias circulares)
    bool validateNoCircularDependencies() const;

    // Comprobación de alcanzabilidad desde recursos iniciales del Overworld
    bool isReachableFromStart(const std::string& targetId) const;

    // Devuelve la lista ordenada topológicamente de prerrequisitos para un hito
    std::vector<std::string> getPrerequisites(const std::string& targetId) const;

    // Auditoría exhaustiva de toda la ruta de victoria y fin de juego
    bool auditAllGameProgression() const;

    size_t getNodeCount() const noexcept { return m_nodes.size(); }

private:
    std::unordered_map<std::string, ProgressionNode> m_nodes;

    void registerCanonicalProgression();
};

} // namespace OpenCraft
