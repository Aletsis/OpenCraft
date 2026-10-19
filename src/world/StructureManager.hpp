#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "world/Block.hpp"
#include "item/ItemStack.hpp"

namespace OpenCraft {

class Chunk;
class ChestBlockEntity;

enum class StructureType : uint8_t {
    None = 0,
    Village,
    Dungeon,
    Ruin,
    TrialChambers
};

struct StructurePieceBlock {
    int worldX;
    int worldY;
    int worldZ;
    BlockID block;
};

struct StructureChestDef {
    int worldX;
    int worldY;
    int worldZ;
    std::vector<ItemStack> loot;
};

struct StructureInstance {
    StructureType type = StructureType::None;
    int anchorX = 0;
    int anchorY = 0;
    int anchorZ = 0;
    int minX = 0, maxX = 0;
    int minY = 0, maxY = 0;
    int minZ = 0, maxZ = 0;
    uint64_t structureSeed = 0;

    bool overlapsChunk(int chunkX, int chunkZ) const noexcept;
    void generateForChunk(int chunkX, int chunkZ, Chunk& chunk, std::vector<StructureChestDef>& outChests) const;
};

class StructureManager {
public:
    static constexpr int REGION_SIZE_CHUNKS = 8; // Celdas de 8x8 chunks (128x128 bloques)

    // Obtiene todas las estructuras activas deterministas que intersecan el chunk (chunkX, chunkZ)
    static std::vector<StructureInstance> getStructuresOverlappingChunk(int chunkX, int chunkZ, uint64_t worldSeed);

    // Genera tablas de botín deterministas para un cofre en una estructura
    static std::vector<ItemStack> generateLoot(StructureType type, uint64_t chestSeed);
};

} // namespace OpenCraft
