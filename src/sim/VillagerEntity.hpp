#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "item/ItemStack.hpp"
#include "world/BlockEntity.hpp"

namespace OpenCraft {

struct PlayerEntity;

enum class VillagerProfession : uint8_t {
    Unemployed = 0,
    Farmer,
    Librarian,
    Armorer
};

enum class TradeResult : uint8_t {
    Success = 0,
    OutOfStock,
    InsufficientItems,
    InvalidOffer
};

struct TradeOffer {
    ItemStack costA;
    ItemStack costB;
    ItemStack result;
    uint32_t uses = 0;
    uint32_t maxUses = 12;

    bool isLocked() const noexcept { return uses >= maxUses; }
};

struct VillagerData {
    VillagerProfession profession = VillagerProfession::Unemployed;
    BlockPos workstationPos{0, 0, 0};
    bool hasWorkstation = false;
    BlockPos bedPos{0, 0, 0};
    bool hasBed = false;
    std::vector<TradeOffer> offers;
    uint32_t restocksToday = 0;
    uint64_t lastRestockTick = 0;

    void assignProfession(VillagerProfession prof);
    TradeResult executeTrade(PlayerEntity& player, size_t offerIndex);
    void restock();
};

} // namespace OpenCraft
