#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <array>
#include "world/Block.hpp"

#include "world/ToolTypes.hpp"

namespace OpenCraft {

using BlockStateId = uint16_t;
constexpr BlockStateId AIR_BLOCK_STATE = 0;

struct BlockProperties {
    std::string namespacedId = "opencraft:air";
    std::string displayName = "Air";
    bool isSolid = false;
    bool isOpaque = false;
    bool isTransparent = true;
    std::array<uint8_t, 6> textureIndices{0, 0, 0, 0, 0, 0};
    BlockID legacyId = BlockID::Air;
    float hardness = 1.0f;
    ToolType requiredTool = ToolType::None;
    ToolTier minHarvestTier = ToolTier::None;
    std::string dropItemName = ""; // Si está vacío, suelta el bloque correspondiente
    uint8_t lightEmission = 0;
    uint8_t lightFilter = 15;
};

class BlockRegistry {
public:
    BlockRegistry();
    ~BlockRegistry() = default;

    static BlockRegistry& global();

    // Registra un bloque devolviendo su ID de estado asignado
    BlockStateId registerBlock(const std::string& namespacedId, BlockProperties props);

    // Consulta por ID de estado
    const BlockProperties& get(BlockStateId stateId) const noexcept;
    const std::string& getNamespacedId(BlockStateId stateId) const noexcept;

    // Consulta por identificador con namespace (ej. "opencraft:stone")
    BlockStateId getStateId(std::string_view namespacedId) const noexcept;
    bool hasBlock(std::string_view namespacedId) const noexcept;

    size_t getStateCount() const noexcept { return m_properties.size(); }

    // Compatibilidad con BlockID original
    BlockStateId fromLegacyId(BlockID id) const noexcept;
    BlockID toLegacyId(BlockStateId stateId) const noexcept;

    // Reinicia el catálogo e inicializa los bloques vanilla de OpenCraft
    void reset();

private:
    std::vector<BlockProperties> m_properties;
    std::unordered_map<std::string, BlockStateId> m_idToState;
    std::array<BlockStateId, static_cast<size_t>(BlockID::Count)> m_legacyToState{};

    void registerDefaults();
};

} // namespace OpenCraft
