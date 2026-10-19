#include "world/IdAliases.hpp"
#include <unordered_map>

namespace OpenCraft::IdAliases {

const std::vector<std::pair<std::string_view, std::string_view>>& table() {
    static const std::vector<std::pair<std::string_view, std::string_view>> aliases = {
        {"opencraft:creeper", "opencraft:lurker"},
        {"opencraft:breeze", "opencraft:whirl"},
        {"opencraft:bogged", "opencraft:swamp_skeleton"},
        {"opencraft:redstone", "opencraft:cinnabar"},
        {"opencraft:redstone_block", "opencraft:cinnabar_block"},
        {"opencraft:redstone_torch", "opencraft:cinnabar_torch"},
        {"opencraft:unlit_redstone_torch", "opencraft:unlit_cinnabar_torch"},
        {"opencraft:redstone_lamp", "opencraft:cinnabar_lamp"},
        {"opencraft:lit_redstone_lamp", "opencraft:lit_cinnabar_lamp"},
        {"opencraft:redstone_wire", "opencraft:cinnabar_wire"},
        {"opencraft:the_nether", "opencraft:abyss"},
        {"opencraft:netherrack", "opencraft:abyssal_rock"},
        {"opencraft:nether_portal", "opencraft:abyss_portal"},
        {"opencraft:nether_quartz_ore", "opencraft:abyssal_quartz_ore"},
        {"opencraft:nether_wart", "opencraft:crimson_fungus"},
        {"opencraft:nether_star", "opencraft:abyss_star"},
        {"opencraft:netherite_ingot", "opencraft:orichalcum_ingot"},
        {"opencraft:netherite_scrap", "opencraft:orichalcum_scrap"},
        {"opencraft:netherite_sword", "opencraft:orichalcum_sword"},
        {"opencraft:netherite_pickaxe", "opencraft:orichalcum_pickaxe"},
        {"opencraft:netherite_chestplate", "opencraft:orichalcum_chestplate"},
        {"opencraft:netherite_upgrade_smithing_template", "opencraft:orichalcum_upgrade_smithing_template"},
        {"opencraft:ancient_debris", "opencraft:orichalcum_vein"},
        {"opencraft:the_end", "opencraft:rim"},
        {"opencraft:end_stone", "opencraft:rim_stone"},
        {"opencraft:end_portal", "opencraft:rim_portal"},
        {"opencraft:end_portal_frame", "opencraft:rim_portal_frame"},
        {"opencraft:ender_pearl", "opencraft:rim_pearl"},
        {"opencraft:eye_of_ender", "opencraft:rim_eye"},
        {"opencraft:ender_dragon", "opencraft:rim_dragon"},
        {"opencraft:wither", "opencraft:bone_hydra"},
        {"opencraft:blaze_rod", "opencraft:ember_rod"},
        {"opencraft:blaze_powder", "opencraft:ember_powder"},
        {"opencraft:ghast_tear", "opencraft:wraith_tear"},
        {"opencraft:glowstone", "opencraft:glow_rock"},
        {"opencraft:glowstone_dust", "opencraft:glow_rock_dust"},
        {"opencraft:breeze_rod", "opencraft:whirl_rod"},
    };
    return aliases;
}

std::string canonical(std::string_view id) {
    static const std::unordered_map<std::string_view, std::string_view> map = [] {
        std::unordered_map<std::string_view, std::string_view> m;
        for (const auto& [from, to] : table()) m.emplace(from, to);
        return m;
    }();
    auto it = map.find(id);
    return std::string(it != map.end() ? it->second : id);
}

} // namespace OpenCraft::IdAliases
