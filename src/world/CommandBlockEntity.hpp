#pragma once

#include "world/BlockEntity.hpp"
#include <string>

namespace OpenCraft {

class WorldSession;

enum class CommandBlockMode : uint8_t {
    Impulse = 0,
    Repeating = 1,
    Chain = 2
};

enum class CommandBlockCondition : uint8_t {
    Unconditional = 0,
    Conditional = 1
};

enum class CommandBlockActivation : uint8_t {
    NeedsCinnabar = 0,
    AlwaysActive = 1
};

class CommandBlockEntity : public BlockEntity {
public:
    explicit CommandBlockEntity(BlockPos pos);
    ~CommandBlockEntity() override = default;

    BlockEntityType getType() const noexcept override { return BlockEntityType::CommandBlock; }

    const std::string& getCommand() const noexcept { return m_command; }
    void setCommand(const std::string& cmd) { m_command = cmd; }

    CommandBlockMode getMode() const noexcept { return m_mode; }
    void setMode(CommandBlockMode mode) noexcept { m_mode = mode; }

    bool isConditional() const noexcept { return m_condition == CommandBlockCondition::Conditional; }
    void setConditional(bool cond) noexcept {
        m_condition = cond ? CommandBlockCondition::Conditional : CommandBlockCondition::Unconditional;
    }

    bool isAlwaysActive() const noexcept { return m_activation == CommandBlockActivation::AlwaysActive; }
    void setAlwaysActive(bool active) noexcept {
        m_activation = active ? CommandBlockActivation::AlwaysActive : CommandBlockActivation::NeedsCinnabar;
    }

    uint32_t getSuccessCount() const noexcept { return m_successCount; }
    void setSuccessCount(uint32_t count) noexcept { m_successCount = count; }

    const std::string& getLastOutput() const noexcept { return m_lastOutput; }
    void setLastOutput(const std::string& out) { m_lastOutput = out; }

    bool wasPowered() const noexcept { return m_wasPowered; }
    void setPowered(bool powered) noexcept { m_wasPowered = powered; }

    // Ejecución lógica del comando
    bool execute(WorldSession& session);

private:
    std::string m_command;
    std::string m_lastOutput;
    uint32_t m_successCount = 0;
    CommandBlockMode m_mode = CommandBlockMode::Impulse;
    CommandBlockCondition m_condition = CommandBlockCondition::Unconditional;
    CommandBlockActivation m_activation = CommandBlockActivation::NeedsCinnabar;
    bool m_wasPowered = false;
};

} // namespace OpenCraft
