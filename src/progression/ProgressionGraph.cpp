#include "progression/ProgressionGraph.hpp"
#include <queue>
#include <algorithm>
#include <iostream>

namespace OpenCraft {

ProgressionGraph::ProgressionGraph() { registerCanonicalProgression(); }

ProgressionGraph& ProgressionGraph::canonical() {
    static ProgressionGraph s_instance;
    return s_instance;
}

void ProgressionGraph::addNode(std::string id, std::string description, std::vector<std::string> dependencies) {
    std::string key = id;
    m_nodes[key] = ProgressionNode{
        .id = std::move(id), .description = std::move(description), .dependencies = std::move(dependencies)};
}

bool ProgressionGraph::hasNode(const std::string& id) const { return m_nodes.find(id) != m_nodes.end(); }

const ProgressionNode* ProgressionGraph::getNode(const std::string& id) const {
    auto it = m_nodes.find(id);
    if (it != m_nodes.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ProgressionGraph::validateNoCircularDependencies() const {
    // Algoritmo de Kahn para detección de ciclos en grafos dirigidos
    std::unordered_map<std::string, int> inDegree;
    std::unordered_map<std::string, std::vector<std::string>> adjacency;

    for (const auto& [id, node] : m_nodes) {
        if (!inDegree.contains(id)) inDegree[id] = 0;
        for (const auto& dep : node.dependencies) {
            adjacency[dep].push_back(id);
            inDegree[id]++;
        }
    }

    std::queue<std::string> q;
    for (const auto& [id, deg] : inDegree) {
        if (deg == 0) {
            q.push(id);
        }
    }

    size_t visitedCount = 0;
    while (!q.empty()) {
        std::string current = q.front();
        q.pop();
        visitedCount++;

        auto it = adjacency.find(current);
        if (it != adjacency.end()) {
            for (const auto& neighbor : it->second) {
                if (--inDegree[neighbor] == 0) {
                    q.push(neighbor);
                }
            }
        }
    }

    return visitedCount == m_nodes.size();
}

bool ProgressionGraph::isReachableFromStart(const std::string& targetId) const {
    if (!hasNode(targetId)) return false;

    // Los nodos raíz son aquellos sin dependencias (ej. "raw_wood")
    std::unordered_set<std::string> resolved;
    bool progress = true;

    while (progress) {
        progress = false;
        for (const auto& [id, node] : m_nodes) {
            if (resolved.contains(id)) continue;

            bool allDepsMet = true;
            for (const auto& dep : node.dependencies) {
                if (!resolved.contains(dep)) {
                    allDepsMet = false;
                    break;
                }
            }

            if (allDepsMet) {
                resolved.insert(id);
                progress = true;
                if (id == targetId) return true;
            }
        }
    }

    return resolved.contains(targetId);
}

std::vector<std::string> ProgressionGraph::getPrerequisites(const std::string& targetId) const {
    std::vector<std::string> result;
    std::unordered_set<std::string> visited;

    auto dfs = [&](auto& self, const std::string& current) -> void {
        auto it = m_nodes.find(current);
        if (it == m_nodes.end()) return;

        for (const auto& dep : it->second.dependencies) {
            if (!visited.contains(dep)) {
                visited.insert(dep);
                self(self, dep);
                result.push_back(dep);
            }
        }
    };

    dfs(dfs, targetId);
    return result;
}

bool ProgressionGraph::auditAllGameProgression() const {
    // 1. Grafo sin ciclos
    if (!validateNoCircularDependencies()) return false;

    // 2. Todos los hitos clave alcanzables
    static const std::vector<std::string> keyMilestones = {"wooden_pickaxe",
                                                           "stone_pickaxe",
                                                           "furnace",
                                                           "iron_ingot",
                                                           "iron_pickaxe",
                                                           "diamond_pickaxe",
                                                           "abyss_portal",
                                                           "abyss_dimension",
                                                           "ember_rod",
                                                           "brewing_stand",
                                                           "potions",
                                                           "rim_eye",
                                                           "rim_portal",
                                                           "rim_dimension",
                                                           "ender_dragon",
                                                           "dragon_egg",
                                                           "end_gateway_and_elytra",
                                                           "bone_hydra_boss",
                                                           "abyss_star",
                                                           "beacon",
                                                           "orichalcum_vein",
                                                           "orichalcum_gear",
                                                           "enchanting_table",
                                                           "anvil"};

    for (const auto& milestone : keyMilestones) {
        if (!isReachableFromStart(milestone)) {
            return false;
        }
    }

    return true;
}

void ProgressionGraph::registerCanonicalProgression() {
    m_nodes.clear();

    // Nivel 0: Overworld básico
    addNode("raw_wood", "Obtención de madera golpeando árboles", {});
    addNode("crafting_table", "Mesa de trabajo 3x3", {"raw_wood"});
    addNode("wooden_pickaxe", "Pico de madera para extraer piedra", {"crafting_table", "raw_wood"});
    addNode("cobblestone", "Extracción de roca", {"wooden_pickaxe"});
    addNode("furnace", "Horno para fundición", {"cobblestone", "crafting_table"});
    addNode("stone_pickaxe", "Pico de piedra", {"cobblestone", "raw_wood", "crafting_table"});

    // Nivel 1: Era del Hierro
    addNode("iron_ore", "Minería de mena de hierro en cuevas", {"stone_pickaxe"});
    addNode("iron_ingot", "Fundición de lingote de hierro en horno", {"iron_ore", "furnace"});
    addNode("iron_pickaxe", "Pico de hierro para diamantes y oro", {"iron_ingot", "raw_wood", "crafting_table"});
    addNode("gold_ore", "Minería de mena de oro", {"iron_pickaxe"});
    addNode("gold_ingot", "Fundición de lingote de oro", {"gold_ore", "furnace"});
    addNode("flint_and_steel", "Mechero para encendido de fuego", {"iron_ingot"});
    addNode("anvil", "Yunque para forja y reparación", {"iron_ingot", "crafting_table"});

    // Nivel 2: Era del Diamante y Encantamientos
    addNode("diamond_ore", "Minería profunda de diamantes (Y < 0)", {"iron_pickaxe"});
    addNode("diamond", "Diamantes puros", {"diamond_ore"});
    addNode("diamond_pickaxe", "Pico de diamante para minar obsidiana", {"diamond", "raw_wood", "crafting_table"});
    addNode("obsidian", "Enfriamiento de lava con agua y minado con diamante", {"diamond_pickaxe"});
    addNode("enchanting_table", "Mesa de encantamientos con libros y obsidiana",
            {"obsidian", "diamond", "raw_wood", "crafting_table"});

    // Nivel 3: El Abismo
    addNode("abyss_portal", "Construcción y activación de portal de obsidiana", {"obsidian", "flint_and_steel"});
    addNode("abyss_dimension", "Acceso a la dimensión infernal", {"abyss_portal"});
    addNode("ember_rod", "Derrota de Ígneos en fortalezas del Abismo", {"abyss_dimension"});
    addNode("ember_powder", "Polvo de ígneo para ojos y combustible de alquimia", {"ember_rod"});
    addNode("crimson_fungus", "Cultivo de hongo carmesí en arena de almas", {"abyss_dimension"});
    addNode("soul_sand", "Arena de almas para cultivos y jefes", {"abyss_dimension"});

    // Alquimia
    addNode("brewing_stand", "Soporte de pociones", {"ember_rod", "cobblestone", "crafting_table"});
    addNode("potions", "Destilación de pociones con efectos de estado",
            {"brewing_stand", "ember_powder", "crimson_fungus"});

    // Nivel 4: El Confín y Dragón
    addNode("rim_pearl", "Perla del Confín de Errantes", {"iron_pickaxe"});
    addNode("rim_eye", "Ojo del Confín para localizar y activar el portal", {"rim_pearl", "ember_powder"});
    addNode("rim_portal", "Activación de marco de 12 ojos en fortaleza", {"rim_eye"});
    addNode("rim_dimension", "Acceso a la dimensión del Confín", {"rim_portal"});
    addNode("ender_dragon", "Batalla y derrota del Dragón del Confín", {"rim_dimension"});
    addNode("dragon_egg", "Trofeo único del huevo de dragón en la fuente", {"ender_dragon"});
    addNode("end_gateway_and_elytra", "Portal a islas exteriores y obtención de élitros", {"ender_dragon"});

    // Jefe Invocable (Hidra ósea) y Baliza
    addNode("hydra_skull", "Obtención de 3 calaveras de esqueleto de la hidra en fortalezas",
            {"abyss_dimension", "iron_pickaxe"});
    addNode("bone_hydra_boss", "Invocación y derrota de la Hidra ósea", {"soul_sand", "hydra_skull"});
    addNode("abyss_star", "Estrella del Abismo caída por la Hidra ósea", {"bone_hydra_boss"});
    addNode("beacon", "Construcción y activación de Baliza con pirámide", {"abyss_star", "obsidian", "crafting_table"});

    // Nivel 5: Oricalco
    addNode("orichalcum_vein", "Minería de escombros ancestrales en el Abismo profundo",
            {"abyss_dimension", "diamond_pickaxe"});
    addNode("orichalcum_scrap", "Fundición de escombros en horno", {"orichalcum_vein", "furnace"});
    addNode("orichalcum_ingot", "Aleación de 4 fragmentos y 4 lingotes de oro", {"orichalcum_scrap", "gold_ingot"});
    addNode("smithing_table", "Mesa de herrería para mejoras", {"iron_ingot", "raw_wood", "crafting_table"});
    addNode("orichalcum_gear", "Mejora de armadura y herramientas de diamante a oricalco",
            {"diamond_pickaxe", "orichalcum_ingot", "smithing_table"});

    // Hitos canónicos (Fase 12)
    addNode("wood_age", "Era de la madera", {"raw_wood"});
    addNode("stone_age", "Era de la piedra", {"cobblestone"});
    addNode("iron_age", "Era del hierro", {"iron_ingot"});
    addNode("diamond_age", "Era del diamante", {"diamond"});
    addNode("defeat_ender_dragon", "Derrota del Dragón del Confín", {"ender_dragon"});
    addNode("elytra", "Obtención de élitros en barcos del Confín", {"end_gateway_and_elytra"});
    addNode("summon_bone_hydra", "Invocación y batalla de la Hidra ósea", {"bone_hydra_boss"});
    addNode("beacon_tier_4", "Faro activado a nivel 4", {"beacon"});
    addNode("orichalcum_armor", "Armadura completa de oricalco", {"orichalcum_gear"});
}

} // namespace OpenCraft
