#include "world/ChunkSection.hpp"
#include <algorithm>

namespace OpenCraft {

ChunkSection::ChunkSection(int sectionY) : m_sectionY(sectionY), m_nonAirCount(0) { m_blocks.fill(AIR_BLOCK_STATE); }

BlockStateId ChunkSection::getBlockState(int x, int y, int z) const noexcept {
    if (x < 0 || x >= SECTION_SIZE || y < 0 || y >= SECTION_SIZE || z < 0 || z >= SECTION_SIZE) {
        return AIR_BLOCK_STATE;
    }
    return m_blocks[getIndex(x, y, z)];
}

void ChunkSection::setBlockState(int x, int y, int z, BlockStateId state) noexcept {
    if (x < 0 || x >= SECTION_SIZE || y < 0 || y >= SECTION_SIZE || z < 0 || z >= SECTION_SIZE) {
        return;
    }
    size_t idx = getIndex(x, y, z);
    BlockStateId oldState = m_blocks[idx];
    if (oldState == state) {
        return;
    }

    if (oldState == AIR_BLOCK_STATE && state != AIR_BLOCK_STATE) {
        ++m_nonAirCount;
    } else if (oldState != AIR_BLOCK_STATE && state == AIR_BLOCK_STATE) {
        if (m_nonAirCount > 0) {
            --m_nonAirCount;
        }
    }

    m_blocks[idx] = state;
}

void ChunkSection::setRawBlocks(const std::array<BlockStateId, BLOCKS_PER_SECTION>& blocks) noexcept {
    m_blocks = blocks;
    m_nonAirCount = 0;
    for (BlockStateId s : m_blocks) {
        if (s != AIR_BLOCK_STATE) {
            ++m_nonAirCount;
        }
    }
}

uint8_t ChunkSection::getSkyLight(int x, int y, int z) const noexcept {
    if (x < 0 || x >= SECTION_SIZE || y < 0 || y >= SECTION_SIZE || z < 0 || z >= SECTION_SIZE) {
        return 0;
    }
    return (m_light[getIndex(x, y, z)] >> 4) & 0x0F;
}

void ChunkSection::setSkyLight(int x, int y, int z, uint8_t level) noexcept {
    if (x < 0 || x >= SECTION_SIZE || y < 0 || y >= SECTION_SIZE || z < 0 || z >= SECTION_SIZE) {
        return;
    }
    size_t idx = getIndex(x, y, z);
    m_light[idx] = static_cast<uint8_t>((m_light[idx] & 0x0F) | ((level & 0x0F) << 4));
}

uint8_t ChunkSection::getBlockLight(int x, int y, int z) const noexcept {
    if (x < 0 || x >= SECTION_SIZE || y < 0 || y >= SECTION_SIZE || z < 0 || z >= SECTION_SIZE) {
        return 0;
    }
    return m_light[getIndex(x, y, z)] & 0x0F;
}

void ChunkSection::setBlockLight(int x, int y, int z, uint8_t level) noexcept {
    if (x < 0 || x >= SECTION_SIZE || y < 0 || y >= SECTION_SIZE || z < 0 || z >= SECTION_SIZE) {
        return;
    }
    size_t idx = getIndex(x, y, z);
    m_light[idx] = static_cast<uint8_t>((m_light[idx] & 0xF0) | (level & 0x0F));
}

} // namespace OpenCraft
