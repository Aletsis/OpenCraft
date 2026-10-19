#include "world/TrialSpawnerBlockEntity.hpp"
#include "item/Item.hpp"

namespace OpenCraft {

std::vector<ItemStack> TrialSpawnerBlockEntity::generateRewards() const {
    std::vector<ItemStack> rewards;
    auto& itemReg = ItemRegistry::global();

    if (m_isOminous) {
        ItemId keyId = itemReg.getId("opencraft:ominous_trial_key");
        if (keyId != EMPTY_ITEM_ID) {
            rewards.emplace_back(keyId, 1);
        }
        ItemId diamondId = itemReg.getId("opencraft:diamond");
        if (diamondId != EMPTY_ITEM_ID) {
            rewards.emplace_back(diamondId, 2);
        }
    } else {
        ItemId keyId = itemReg.getId("opencraft:trial_key");
        if (keyId != EMPTY_ITEM_ID) {
            rewards.emplace_back(keyId, 1);
        }
        ItemId windChargeId = itemReg.getId("opencraft:wind_charge");
        if (windChargeId != EMPTY_ITEM_ID) {
            rewards.emplace_back(windChargeId, 4);
        }
        ItemId emeraldId = itemReg.getId("opencraft:emerald");
        if (emeraldId != EMPTY_ITEM_ID) {
            rewards.emplace_back(emeraldId, 3);
        }
    }

    return rewards;
}

} // namespace OpenCraft
