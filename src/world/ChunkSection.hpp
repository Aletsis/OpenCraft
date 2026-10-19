#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include "world/Block.hpp"
#include "world/BlockRegistry.hpp"

namespace OpenCraft {

class ChunkSection {
public:
    static constexpr int SECTION_SIZE = 16;
    static constexpr size_t BLOCKS_PER_SECTION = SECTION_SIZE * SECTION_SIZE * SECTION_SIZE; // 4096

    explicit ChunkSection(int sectionY = 0);
    ~ChunkSection() = default;

    int getSectionY() const noexcept { return m_sectionY; }
    void setSectionY(int y) noexcept { m_sectionY = y; }

    bool isEmpty() const noexcept { return m_nonAirCount == 0; }
    uint16_t getNonAirCount() const noexcept { return m_nonAirCount; }

    BlockStateId getBlockState(int x, int y, int z) const noexcept;
    void setBlockState(int x, int y, int z, BlockStateId state) noexcept;

    uint8_t getSkyLight(int x, int y, int z) const noexcept;
    void setSkyLight(int x, int y, int z, uint8_t level) noexcept;

    uint8_t getBlockLight(int x, int y, int z) const noexcept;
    void setBlockLight(int x, int y, int z, uint8_t level) noexcept;

    static constexpr size_t getIndex(int x, int y, int z) noexcept {
        return static_cast<size_t>((y * SECTION_SIZE + z) * SECTION_SIZE + x);
    }

    const std::array<BlockStateId, BLOCKS_PER_SECTION>& getRawBlocks() const noexcept { return m_blocks; }

    void setRawBlocks(const std::array<BlockStateId, BLOCKS_PER_SECTION>& blocks) noexcept;

    const std::array<uint8_t, BLOCKS_PER_SECTION>& getRawLight() const noexcept { return m_light; }

    void setRawLight(const std::array<uint8_t, BLOCKS_PER_SECTION>& light) noexcept { m_light = light; }

private:
    int m_sectionY = 0;
    uint16_t m_nonAirCount = 0;
    std::array<BlockStateId, BLOCKS_PER_SECTION> m_blocks{};
    std::array<uint8_t, BLOCKS_PER_SECTION> m_light{};
};

} // namespace OpenCraft
