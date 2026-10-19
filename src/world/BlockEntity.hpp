#pragma once

#include <cstdint>
#include <cstddef>

namespace OpenCraft {

struct BlockPos {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const BlockPos& o) const noexcept { return x == o.x && y == o.y && z == o.z; }
};

struct BlockPosHash {
    size_t operator()(const BlockPos& p) const noexcept {
        return (static_cast<size_t>(p.x) * 73856093) ^ (static_cast<size_t>(p.y) * 19349663) ^
               (static_cast<size_t>(p.z) * 83492791);
    }
};

enum class BlockEntityType : uint8_t {
    Unknown = 0,
    Chest = 1,
    Furnace = 2,
    Hopper = 3,
    Dispenser = 4,
    Dropper = 5,
    Crafter = 6,
    Piston = 7,
    BrewingStand = 8,
    Beacon = 9,
    CommandBlock = 10,
    TrialSpawner = 11,
    Vault = 12
};

class BlockEntity {
public:
    virtual ~BlockEntity() = default;

    virtual BlockEntityType getType() const noexcept = 0;
    const BlockPos& getPos() const noexcept { return m_pos; }
    void setPos(const BlockPos& pos) noexcept { m_pos = pos; }

protected:
    explicit BlockEntity(BlockPos pos) : m_pos(pos) {}
    BlockPos m_pos;
};

} // namespace OpenCraft
