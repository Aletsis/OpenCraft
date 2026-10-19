#include "physics/Raycast.hpp"
#include "world/World.hpp"
#include <cmath>

namespace OpenCraft {

RaycastHit VoxelRaycast::cast(const World& world, const Math::Vec3& origin, const Math::Vec3& direction,
                              float maxDistance) {
    RaycastHit result;
    Math::Vec3 dir = direction.normalized();

    if (dir.length() < 0.0001f) {
        return result;
    }

    int x = static_cast<int>(std::floor(origin.x));
    int y = static_cast<int>(std::floor(origin.y));
    int z = static_cast<int>(std::floor(origin.z));

    int stepX = (dir.x > 0.0f) ? 1 : ((dir.x < 0.0f) ? -1 : 0);
    int stepY = (dir.y > 0.0f) ? 1 : ((dir.y < 0.0f) ? -1 : 0);
    int stepZ = (dir.z > 0.0f) ? 1 : ((dir.z < 0.0f) ? -1 : 0);

    float tDeltaX = (stepX != 0) ? std::abs(1.0f / dir.x) : 1e30f;
    float tDeltaY = (stepY != 0) ? std::abs(1.0f / dir.y) : 1e30f;
    float tDeltaZ = (stepZ != 0) ? std::abs(1.0f / dir.z) : 1e30f;

    float tMaxX = (stepX > 0) ? (x + 1.0f - origin.x) * tDeltaX : (origin.x - x) * tDeltaX;
    float tMaxY = (stepY > 0) ? (y + 1.0f - origin.y) * tDeltaY : (origin.y - y) * tDeltaY;
    float tMaxZ = (stepZ > 0) ? (z + 1.0f - origin.z) * tDeltaZ : (origin.z - z) * tDeltaZ;

    float distance = 0.0f;
    Math::Vec3 normal{0.0f, 0.0f, 0.0f};

    while (distance <= maxDistance) {
        BlockStateId state = world.getBlockState(x, y, z);
        BlockID block = BlockRegistry::global().toLegacyId(state);
        // Se apunta a cualquier bloque visible salvo aire y fluidos (antorchas, cultivos y raíles incluidos)
        const auto& props = BlockRegistry::global().get(state);
        bool fluid = props.legacyId == BlockID::Water || props.legacyId == BlockID::FlowingWater ||
                     props.legacyId == BlockID::Lava || props.legacyId == BlockID::FlowingLava;
        if (state != AIR_BLOCK_STATE && !fluid && props.legacyId != BlockID::Air) {
            result.hit = true;
            result.blockX = x;
            result.blockY = y;
            result.blockZ = z;
            result.normal = normal;
            result.blockID = block;
            result.blockState = state;
            return result;
        }

        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                x += stepX;
                distance = tMaxX;
                tMaxX += tDeltaX;
                normal = Math::Vec3(-static_cast<float>(stepX), 0.0f, 0.0f);
            } else {
                z += stepZ;
                distance = tMaxZ;
                tMaxZ += tDeltaZ;
                normal = Math::Vec3(0.0f, 0.0f, -static_cast<float>(stepZ));
            }
        } else {
            if (tMaxY < tMaxZ) {
                y += stepY;
                distance = tMaxY;
                tMaxY += tDeltaY;
                normal = Math::Vec3(0.0f, -static_cast<float>(stepY), 0.0f);
            } else {
                z += stepZ;
                distance = tMaxZ;
                tMaxZ += tDeltaZ;
                normal = Math::Vec3(0.0f, 0.0f, -static_cast<float>(stepZ));
            }
        }
    }

    return result;
}

} // namespace OpenCraft
