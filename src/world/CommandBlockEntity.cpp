#include "world/CommandBlockEntity.hpp"
#include "sim/WorldSession.hpp"
#include "sim/CommandEngine.hpp"

namespace OpenCraft {

CommandBlockEntity::CommandBlockEntity(BlockPos pos) : BlockEntity(pos) {}

bool CommandBlockEntity::execute(WorldSession& session) {
    if (m_command.empty()) {
        m_successCount = 0;
        m_lastOutput = "Comando vacío";
        return false;
    }

    CommandContext ctx{.sourceEntityId = 0,
                       .sourcePosition =
                           Math::Vec3(static_cast<float>(m_pos.x) + 0.5f, static_cast<float>(m_pos.y) + 0.5f,
                                      static_cast<float>(m_pos.z) + 0.5f),
                       .sourceDimension = static_cast<uint8_t>(session.getCurrentDimension()),
                       .permissionLevel = 4, // Bloque de comando opera con privilegios plenos
                       .isSilent = !session.getCommandEngine().getGameRules().commandBlockOutput};

    auto res = session.getCommandEngine().execute(m_command, ctx, session);
    m_successCount = res.affectedCount;
    m_lastOutput = res.message;
    return res.success;
}

} // namespace OpenCraft
