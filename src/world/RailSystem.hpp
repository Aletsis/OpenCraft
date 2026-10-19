#pragma once

#include "world/Block.hpp"
#include "world/BlockEntity.hpp"

namespace OpenCraft {

enum class RailShape : uint8_t {
    NorthSouth = 0,
    EastWest = 1,
    AscendingEast = 2,
    AscendingWest = 3,
    AscendingNorth = 4,
    AscendingSouth = 5,
    SouthEast = 6,
    SouthWest = 7,
    NorthWest = 8,
    NorthEast = 9
};

class RailSystem {
public:
    static bool isRail(BlockID id) noexcept {
        return id == BlockID::Rail || id == BlockID::PoweredRail || id == BlockID::DetectorRail ||
               id == BlockID::ActivatorRail;
    }

    static bool isPoweredRail(BlockID id) noexcept { return id == BlockID::PoweredRail; }

    static bool isDetectorRail(BlockID id) noexcept { return id == BlockID::DetectorRail; }
};

} // namespace OpenCraft
