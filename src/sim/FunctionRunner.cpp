#include "sim/FunctionRunner.hpp"
#include "sim/WorldSession.hpp"
#include <sstream>

namespace OpenCraft {

void FunctionRunner::registerFunction(const std::string& name, const std::vector<std::string>& commands) {
    m_functions[name] = commands;
}

bool FunctionRunner::loadFunctionFromText(const std::string& name, const std::string& textContent) {
    std::istringstream stream(textContent);
    std::string line;
    std::vector<std::string> commands;

    while (std::getline(stream, line)) {
        // Eliminar retorno de carro
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // Omitir líneas vacías o comentarios (#)
        size_t firstNonSpace = line.find_first_not_of(" \t");
        if (firstNonSpace == std::string::npos || line[firstNonSpace] == '#') {
            continue;
        }
        commands.push_back(line.substr(firstNonSpace));
    }

    m_functions[name] = std::move(commands);
    return true;
}

bool FunctionRunner::hasFunction(const std::string& name) const { return m_functions.find(name) != m_functions.end(); }

const std::vector<std::string>* FunctionRunner::getFunction(const std::string& name) const {
    auto it = m_functions.find(name);
    if (it != m_functions.end()) {
        return &it->second;
    }
    return nullptr;
}

void FunctionRunner::clear() {
    m_functions.clear();
    m_commandsExecutedThisTick = 0;
}

CommandExecutionResult FunctionRunner::runFunction(const std::string& name, const CommandContext& ctx,
                                                   WorldSession& session, size_t currentDepth) {
    if (currentDepth >= MAX_RECURSION_DEPTH) {
        return CommandExecutionResult{
            .success = false,
            .message = "Error: Profundidad máxima de recursión excedida (16). Prevención de bucles infinitos activada.",
            .affectedCount = 0};
    }

    auto it = m_functions.find(name);
    if (it == m_functions.end()) {
        return CommandExecutionResult{
            .success = false, .message = "Error: Función no encontrada: " + name, .affectedCount = 0};
    }

    const auto& commandList = it->second;
    uint32_t totalAffected = 0;
    size_t executedInThisFunction = 0;

    for (const auto& rawCmd : commandList) {
        if (m_commandsExecutedThisTick >= DEFAULT_MAX_COMMANDS_PER_TICK) {
            return CommandExecutionResult{.success = false,
                                          .message = "Error: Presupuesto de comandos excedido (máximo 10000 por tick). "
                                                     "Ejecución abortada de forma segura.",
                                          .affectedCount = totalAffected};
        }

        m_commandsExecutedThisTick++;
        executedInThisFunction++;

        // Chequeo si la línea es una llamada recursiva o a otra función: /function <otra>
        std::string cmd = rawCmd;
        if (!cmd.empty() && cmd[0] == '/') {
            cmd = cmd.substr(1);
        }

        std::istringstream iss(cmd);
        std::string token;
        iss >> token;

        if (token == "function") {
            std::string subName;
            iss >> subName;
            if (!subName.empty()) {
                auto subRes = runFunction(subName, ctx, session, currentDepth + 1);
                if (!subRes.success) {
                    return subRes;
                }
                totalAffected += subRes.affectedCount;
                continue;
            }
        }

        auto res = session.getCommandEngine().execute(rawCmd, ctx, session);
        if (res.success) {
            totalAffected += res.affectedCount;
        }
    }

    return CommandExecutionResult{.success = true,
                                  .message = "Función '" + name + "' ejecutada con éxito (" +
                                             std::to_string(executedInThisFunction) + " comandos)",
                                  .affectedCount = totalAffected};
}

} // namespace OpenCraft
