#pragma once

#include "sim/CommandEngine.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace OpenCraft {

class WorldSession;

class FunctionRunner {
public:
    static constexpr size_t DEFAULT_MAX_COMMANDS_PER_TICK = 10000;
    static constexpr size_t MAX_RECURSION_DEPTH = 16;

    FunctionRunner() = default;
    ~FunctionRunner() = default;

    void registerFunction(const std::string& name, const std::vector<std::string>& commands);
    bool loadFunctionFromText(const std::string& name, const std::string& textContent);
    bool hasFunction(const std::string& name) const;
    const std::vector<std::string>* getFunction(const std::string& name) const;
    void clear();

    // Reinicia el contador de presupuesto al inicio de cada tick de simulación
    void onTickStart() noexcept { m_commandsExecutedThisTick = 0; }

    size_t getCommandsExecutedThisTick() const noexcept { return m_commandsExecutedThisTick; }

    // Ejecuta una función por nombre controlando profundidad y presupuesto
    CommandExecutionResult runFunction(const std::string& name, const CommandContext& ctx, WorldSession& session,
                                       size_t currentDepth = 0);

private:
    std::unordered_map<std::string, std::vector<std::string>> m_functions;
    size_t m_commandsExecutedThisTick = 0;
};

} // namespace OpenCraft
