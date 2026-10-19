#include "world/VaultBlockEntity.hpp"
#include "item/Item.hpp"

namespace OpenCraft {

bool VaultBlockEntity::tryUnlock(uint32_t playerId, const std::string& keyNamespacedId,
                                 std::vector<ItemStack>& outLoot) {
    if (hasPlayerUnlocked(playerId)) {
        return false; // Ya fue abierta por este jugador
    }

    if (m_isOminous) {
        if (keyNamespacedId != "opencraft:ominous_trial_key") {
            return false;
        }
    } else {
        if (keyNamespacedId != "opencraft:trial_key") {
            return false;
        }
    }

    // Desbloqueo exitoso: registrar jugador
    markPlayerUnlocked(playerId);

    auto& itemReg = ItemRegistry::global();
    if (m_isOminous) {
        ItemId heavyCoreId = itemReg.getId("opencraft:heavy_core");
        if (heavyCoreId != EMPTY_ITEM_ID) {
            outLoot.emplace_back(heavyCoreId, 1);
        }
        ItemId maceId = itemReg.getId("opencraft:mace");
        if (maceId != EMPTY_ITEM_ID) {
            outLoot.emplace_back(maceId, 1);
        }
        ItemId diamondId = itemReg.getId("opencraft:diamond");
        if (diamondId != EMPTY_ITEM_ID) {
            outLoot.emplace_back(diamondId, 3);
        }
    } else {
        ItemId whirlRodId = itemReg.getId("opencraft:whirl_rod");
        if (whirlRodId != EMPTY_ITEM_ID) {
            outLoot.emplace_back(whirlRodId, 2);
        }
        ItemId ironIngotId = itemReg.getId("opencraft:iron_ingot");
        if (ironIngotId != EMPTY_ITEM_ID) {
            outLoot.emplace_back(ironIngotId, 4);
        }
        ItemId emeraldId = itemReg.getId("opencraft:emerald");
        if (emeraldId != EMPTY_ITEM_ID) {
            outLoot.emplace_back(emeraldId, 5);
        }
    }

    return true;
}

} // namespace OpenCraft
