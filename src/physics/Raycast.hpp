#pragma once

#include "math/Math.hpp"
#include "world/Block.hpp"
#include "world/BlockRegistry.hpp"

namespace OpenCraft {

class World;

struct RaycastHit {
    bool hit = false;
    int blockX = 0;
    int blockY = 0;
    int blockZ = 0;
    Math::Vec3 normal{0.0f, 0.0f, 0.0f};
    BlockID blockID = BlockID::Air;
    BlockStateId blockState = AIR_BLOCK_STATE;
};

class VoxelRaycast {
public:
    static RaycastHit cast(const World& world, const Math::Vec3& origin, const Math::Vec3& direction,
                           float maxDistance = 6.0f);
};

} // namespace OpenCraft
