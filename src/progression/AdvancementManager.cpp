#include "progression/AdvancementManager.hpp"

namespace OpenCraft {

AdvancementManager::AdvancementManager() { initDefaults(); }

void AdvancementManager::initDefaults() {
    registerAdvancement(Advancement{.id = "opencraft:story/root",
                                    .parent = "",
                                    .title = "OpenCraft",
                                    .description = "El corazón y la historia del juego",
                                    .iconItem = "opencraft:crafting_table",
                                    .frame = AdvancementFrame::Task,
                                    .criteria = {"join_world"}});

    registerAdvancement(Advancement{.id = "opencraft:story/mine_stone",
                                    .parent = "opencraft:story/root",
                                    .title = "Edad de Piedra",
                                    .description = "Pica piedra con tu nuevo pico",
                                    .iconItem = "opencraft:cobblestone",
                                    .frame = AdvancementFrame::Task,
                                    .criteria = {"mine_stone"}});

    registerAdvancement(Advancement{.id = "opencraft:story/upgrade_tools",
                                    .parent = "opencraft:story/mine_stone",
                                    .title = "Mejora",
                                    .description = "Fabrica un pico mejor",
                                    .iconItem = "opencraft:stone_pickaxe",
                                    .frame = AdvancementFrame::Task,
                                    .criteria = {"craft_pickaxe"}});

    registerAdvancement(Advancement{.id = "opencraft:story/smelt_iron",
                                    .parent = "opencraft:story/upgrade_tools",
                                    .title = "Adquiriendo hardware",
                                    .description = "Funde un lingote de hierro",
                                    .iconItem = "opencraft:iron_ingot",
                                    .frame = AdvancementFrame::Task,
                                    .criteria = {"smelt_iron"}});

    registerAdvancement(Advancement{.id = "opencraft:nether/root",
                                    .parent = "opencraft:story/root",
                                    .title = "Abismo",
                                    .description = "Entra en el reino ardiente",
                                    .iconItem = "opencraft:abyssal_rock",
                                    .frame = AdvancementFrame::Task,
                                    .criteria = {"enter_nether"}});

    registerAdvancement(Advancement{.id = "opencraft:end/root",
                                    .parent = "opencraft:story/root",
                                    .title = "El Confín",
                                    .description = "¿O es el principio?",
                                    .iconItem = "opencraft:rim_stone",
                                    .frame = AdvancementFrame::Task,
                                    .criteria = {"enter_end"}});

    registerAdvancement(Advancement{.id = "opencraft:adventure/kill_mob",
                                    .parent = "opencraft:story/root",
                                    .title = "Cazador de monstruos",
                                    .description = "Ataca y destruye a un monstruo hostil",
                                    .iconItem = "opencraft:iron_sword",
                                    .frame = AdvancementFrame::Task,
                                    .criteria = {"kill_mob"}});
}

void AdvancementManager::registerAdvancement(Advancement adv) {
    std::string id = adv.id;
    m_advancements[id] = std::move(adv);
}

bool AdvancementManager::grantAdvancement(const std::string& id) {
    if (m_advancements.find(id) == m_advancements.end()) {
        return false;
    }
    return m_completedAdvancements.insert(id).second;
}

bool AdvancementManager::revokeAdvancement(const std::string& id) { return m_completedAdvancements.erase(id) > 0; }

bool AdvancementManager::hasAdvancement(const std::string& id) const {
    return m_completedAdvancements.find(id) != m_completedAdvancements.end();
}

void AdvancementManager::loadCompletedAdvancements(const std::vector<std::string>& list) {
    m_completedAdvancements.clear();
    for (const auto& adv : list) {
        if (m_advancements.find(adv) != m_advancements.end()) {
            m_completedAdvancements.insert(adv);
        }
    }
}

std::vector<std::string> AdvancementManager::exportCompletedAdvancements() const {
    return std::vector<std::string>(m_completedAdvancements.begin(), m_completedAdvancements.end());
}

void AdvancementManager::onPlayerJoin() { grantAdvancement("opencraft:story/root"); }

void AdvancementManager::onBlockMined(const std::string& blockId) {
    if (blockId == "opencraft:stone" || blockId == "opencraft:cobblestone") {
        grantAdvancement("opencraft:story/mine_stone");
    }
}

void AdvancementManager::onItemCrafted(const std::string& itemId) {
    if (itemId == "opencraft:stone_pickaxe" || itemId == "opencraft:iron_pickaxe" ||
        itemId == "opencraft:diamond_pickaxe") {
        grantAdvancement("opencraft:story/upgrade_tools");
    }
    if (itemId == "opencraft:iron_ingot") {
        grantAdvancement("opencraft:story/smelt_iron");
    }
}

void AdvancementManager::onDimensionChanged(uint8_t dimension) {
    if (dimension == 1) {
        grantAdvancement("opencraft:nether/root");
    } else if (dimension == 2) {
        grantAdvancement("opencraft:end/root");
    }
}

void AdvancementManager::onMobKilled(const std::string& /*mobType*/) {
    grantAdvancement("opencraft:adventure/kill_mob");
}

} // namespace OpenCraft
