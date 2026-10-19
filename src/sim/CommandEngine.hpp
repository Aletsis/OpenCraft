#pragma once

#include "math/Math.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace OpenCraft {

class WorldSession;
struct PlayerEntity;

struct GameRules {
    bool doDaylightCycle = true;
    bool keepInventory = false;
    bool mobGriefing = true;
    bool doMobSpawning = true;
    bool commandBlockOutput = true;
    bool naturalRegeneration = true;
    bool fallDamage = true;
    bool fireDamage = true;
    bool drowningDamage = true;
    bool pvp = true;
    int32_t randomTickSpeed = 3;
    int32_t maxCommandChainLength = 10000;

    bool getBool(const std::string& name, bool defaultVal = true) const;
    bool set(const std::string& name, const std::string& val);
    std::string get(const std::string& name) const;
    std::unordered_map<std::string, std::string> getAll() const;
};

struct CommandContext {
    uint32_t sourceEntityId = 0;
    Math::Vec3 sourcePosition{0.0f, 0.0f, 0.0f};
    uint8_t sourceDimension = 0;
    uint8_t permissionLevel = 4; // 0: Normal, 1: Mod, 2: GM, 3: Admin, 4: Server/Console/CmdBlock
    bool isSilent = false;
};

struct CommandExecutionResult {
    bool success = false;
    std::string message;
    uint32_t affectedCount = 0;
};

class CommandEngine {
public:
    static constexpr size_t MAX_FILL_VOLUME = 32768; // Límite seguro canónico para /fill

    CommandEngine();
    ~CommandEngine() = default;

    GameRules& getGameRules() noexcept { return m_gameRules; }
    const GameRules& getGameRules() const noexcept { return m_gameRules; }

    // Ejecuta una línea de comando autoritativa
    CommandExecutionResult execute(const std::string& commandLine, const CommandContext& ctx, WorldSession& session);

    // Resolución de selectores (@p, @a, @e, @s, <nombre>)
    std::vector<uint32_t> resolvePlayerTargets(const std::string& selector, const CommandContext& ctx,
                                               WorldSession& session);

    // Resolución de coordenadas absolutas o relativas (~)
    static bool parseCoordinate(const std::string& token, float origin, float& outVal);
    static bool parseBlockPos(const std::string& sx, const std::string& sy, const std::string& sz,
                              const Math::Vec3& origin, int& outX, int& outY, int& outZ);

    // Permisos por jugador
    void setOperator(const std::string& playerName, uint8_t level);
    void removeOperator(const std::string& playerName);
    uint8_t getPermissionLevel(const std::string& playerName) const;
    bool isOperator(const std::string& playerName) const;

private:
    GameRules m_gameRules;
    std::unordered_map<std::string, uint8_t> m_operatorLevels;

    // Comandos individuales
    CommandExecutionResult cmdGameMode(const std::vector<std::string>& args, const CommandContext& ctx,
                                       WorldSession& session);
    CommandExecutionResult cmdTime(const std::vector<std::string>& args, const CommandContext& ctx,
                                   WorldSession& session);
    CommandExecutionResult cmdWeather(const std::vector<std::string>& args, const CommandContext& ctx,
                                      WorldSession& session);
    CommandExecutionResult cmdTeleport(const std::vector<std::string>& args, const CommandContext& ctx,
                                       WorldSession& session);
    CommandExecutionResult cmdGive(const std::vector<std::string>& args, const CommandContext& ctx,
                                   WorldSession& session);
    CommandExecutionResult cmdSummon(const std::vector<std::string>& args, const CommandContext& ctx,
                                     WorldSession& session);
    CommandExecutionResult cmdSetBlock(const std::vector<std::string>& args, const CommandContext& ctx,
                                       WorldSession& session);
    CommandExecutionResult cmdFill(const std::vector<std::string>& args, const CommandContext& ctx,
                                   WorldSession& session);
    CommandExecutionResult cmdGameRule(const std::vector<std::string>& args, const CommandContext& ctx,
                                       WorldSession& session);
    CommandExecutionResult cmdHelp(const std::vector<std::string>& args, const CommandContext& ctx,
                                   WorldSession& session);
    CommandExecutionResult cmdKill(const std::vector<std::string>& args, const CommandContext& ctx,
                                   WorldSession& session);
    CommandExecutionResult cmdClear(const std::vector<std::string>& args, const CommandContext& ctx,
                                    WorldSession& session);
    CommandExecutionResult cmdEffect(const std::vector<std::string>& args, const CommandContext& ctx,
                                     WorldSession& session);
    CommandExecutionResult cmdXp(const std::vector<std::string>& args, const CommandContext& ctx,
                                 WorldSession& session);
};

} // namespace OpenCraft
