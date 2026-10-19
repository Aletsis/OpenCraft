#include "sim/CommandEngine.hpp"
#include "world/IdAliases.hpp"
#include "sim/WorldSession.hpp"
#include "sim/FunctionRunner.hpp"
#include "world/BlockRegistry.hpp"
#include "progression/StatusEffect.hpp"
#include "item/Item.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace OpenCraft {

bool GameRules::getBool(const std::string& name, bool defaultVal) const {
    if (name == "doDaylightCycle") return doDaylightCycle;
    if (name == "keepInventory") return keepInventory;
    if (name == "mobGriefing") return mobGriefing;
    if (name == "doMobSpawning") return doMobSpawning;
    if (name == "commandBlockOutput") return commandBlockOutput;
    if (name == "naturalRegeneration") return naturalRegeneration;
    if (name == "fallDamage") return fallDamage;
    if (name == "fireDamage") return fireDamage;
    if (name == "drowningDamage") return drowningDamage;
    if (name == "pvp") return pvp;
    return defaultVal;
}

bool GameRules::set(const std::string& name, const std::string& val) {
    auto parseBool = [](const std::string& s) -> bool { return (s == "true" || s == "1" || s == "yes"); };

    if (name == "doDaylightCycle") {
        doDaylightCycle = parseBool(val);
        return true;
    }
    if (name == "keepInventory") {
        keepInventory = parseBool(val);
        return true;
    }
    if (name == "mobGriefing") {
        mobGriefing = parseBool(val);
        return true;
    }
    if (name == "doMobSpawning") {
        doMobSpawning = parseBool(val);
        return true;
    }
    if (name == "commandBlockOutput") {
        commandBlockOutput = parseBool(val);
        return true;
    }
    if (name == "naturalRegeneration") {
        naturalRegeneration = parseBool(val);
        return true;
    }
    if (name == "fallDamage") {
        fallDamage = parseBool(val);
        return true;
    }
    if (name == "fireDamage") {
        fireDamage = parseBool(val);
        return true;
    }
    if (name == "drowningDamage") {
        drowningDamage = parseBool(val);
        return true;
    }
    if (name == "pvp") {
        pvp = parseBool(val);
        return true;
    }
    if (name == "randomTickSpeed") {
        try {
            randomTickSpeed = std::stoi(val);
            return true;
        } catch (...) {
            return false;
        }
    }
    if (name == "maxCommandChainLength") {
        try {
            maxCommandChainLength = std::stoi(val);
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

std::string GameRules::get(const std::string& name) const {
    if (name == "doDaylightCycle") return doDaylightCycle ? "true" : "false";
    if (name == "keepInventory") return keepInventory ? "true" : "false";
    if (name == "mobGriefing") return mobGriefing ? "true" : "false";
    if (name == "doMobSpawning") return doMobSpawning ? "true" : "false";
    if (name == "commandBlockOutput") return commandBlockOutput ? "true" : "false";
    if (name == "naturalRegeneration") return naturalRegeneration ? "true" : "false";
    if (name == "fallDamage") return fallDamage ? "true" : "false";
    if (name == "fireDamage") return fireDamage ? "true" : "false";
    if (name == "drowningDamage") return drowningDamage ? "true" : "false";
    if (name == "pvp") return pvp ? "true" : "false";
    if (name == "randomTickSpeed") return std::to_string(randomTickSpeed);
    if (name == "maxCommandChainLength") return std::to_string(maxCommandChainLength);
    return "unknown";
}

std::unordered_map<std::string, std::string> GameRules::getAll() const {
    return {{"doDaylightCycle", doDaylightCycle ? "true" : "false"},
            {"keepInventory", keepInventory ? "true" : "false"},
            {"mobGriefing", mobGriefing ? "true" : "false"},
            {"doMobSpawning", doMobSpawning ? "true" : "false"},
            {"commandBlockOutput", commandBlockOutput ? "true" : "false"},
            {"naturalRegeneration", naturalRegeneration ? "true" : "false"},
            {"fallDamage", fallDamage ? "true" : "false"},
            {"fireDamage", fireDamage ? "true" : "false"},
            {"drowningDamage", drowningDamage ? "true" : "false"},
            {"pvp", pvp ? "true" : "false"},
            {"randomTickSpeed", std::to_string(randomTickSpeed)},
            {"maxCommandChainLength", std::to_string(maxCommandChainLength)}};
}

CommandEngine::CommandEngine() {
    // Inicialización por defecto de operadores
    m_operatorLevels["server"] = 4;
}

void CommandEngine::setOperator(const std::string& playerName, uint8_t level) { m_operatorLevels[playerName] = level; }

void CommandEngine::removeOperator(const std::string& playerName) { m_operatorLevels.erase(playerName); }

uint8_t CommandEngine::getPermissionLevel(const std::string& playerName) const {
    auto it = m_operatorLevels.find(playerName);
    if (it != m_operatorLevels.end()) {
        return it->second;
    }
    return 0;
}

bool CommandEngine::isOperator(const std::string& playerName) const { return getPermissionLevel(playerName) >= 2; }

bool CommandEngine::parseCoordinate(const std::string& token, float origin, float& outVal) {
    if (token.empty()) return false;
    if (token[0] == '~') {
        if (token.length() == 1) {
            outVal = origin;
            return true;
        }
        try {
            float offset = std::stof(token.substr(1));
            outVal = origin + offset;
            return true;
        } catch (...) {
            return false;
        }
    }
    try {
        outVal = std::stof(token);
        return true;
    } catch (...) {
        return false;
    }
}

bool CommandEngine::parseBlockPos(const std::string& sx, const std::string& sy, const std::string& sz,
                                  const Math::Vec3& origin, int& outX, int& outY, int& outZ) {
    float fx = 0.0f, fy = 0.0f, fz = 0.0f;
    if (!parseCoordinate(sx, origin.x, fx)) return false;
    if (!parseCoordinate(sy, origin.y, fy)) return false;
    if (!parseCoordinate(sz, origin.z, fz)) return false;

    outX = static_cast<int>(std::floor(fx));
    outY = static_cast<int>(std::floor(fy));
    outZ = static_cast<int>(std::floor(fz));
    return true;
}

std::vector<uint32_t> CommandEngine::resolvePlayerTargets(const std::string& selector, const CommandContext& ctx,
                                                          WorldSession& session) {
    std::vector<uint32_t> results;
    PlayerEntity& localPlayer = session.getPlayer();

    if (selector == "@s") {
        if (ctx.sourceEntityId != 0 && ctx.sourceEntityId != INVALID_ENTITY_ID) {
            results.push_back(ctx.sourceEntityId);
        } else {
            results.push_back(localPlayer.id);
        }
    } else if (selector == "@p" || selector == "@a") {
        // En singleplayer o sesión con local player
        results.push_back(localPlayer.id);
        for (const auto& r : session.getConnectedPlayers()) {
            results.push_back(r.first);
        }
    } else {
        // Nombre directo
        if (selector == "Player" || selector == "@s" || selector.empty()) {
            results.push_back(localPlayer.id);
        } else {
            for (const auto& r : session.getConnectedPlayers()) {
                if (session.getPlayerName(r.first) == selector) {
                    results.push_back(r.first);
                    return results;
                }
            }
            results.push_back(localPlayer.id);
        }
    }
    return results;
}

CommandExecutionResult CommandEngine::execute(const std::string& commandLine, const CommandContext& ctx,
                                              WorldSession& session) {
    std::string clean = commandLine;
    // Quitar barra inicial si la tiene
    if (!clean.empty() && clean[0] == '/') {
        clean = clean.substr(1);
    }

    std::istringstream iss(clean);
    std::string cmd;
    iss >> cmd;
    if (cmd.empty()) {
        return CommandExecutionResult{.success = false, .message = "Comando vacío", .affectedCount = 0};
    }

    // Convertir comando a minúsculas
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) { return std::tolower(c); });

    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    if (cmd == "help") {
        return cmdHelp(args, ctx, session);
    }
    if (cmd == "gamemode") {
        return cmdGameMode(args, ctx, session);
    }
    if (cmd == "time") {
        return cmdTime(args, ctx, session);
    }
    if (cmd == "weather") {
        return cmdWeather(args, ctx, session);
    }
    if (cmd == "tp" || cmd == "teleport") {
        return cmdTeleport(args, ctx, session);
    }
    if (cmd == "give") {
        return cmdGive(args, ctx, session);
    }
    if (cmd == "summon") {
        return cmdSummon(args, ctx, session);
    }
    if (cmd == "setblock") {
        return cmdSetBlock(args, ctx, session);
    }
    if (cmd == "fill") {
        return cmdFill(args, ctx, session);
    }
    if (cmd == "gamerule") {
        return cmdGameRule(args, ctx, session);
    }
    if (cmd == "kill") {
        return cmdKill(args, ctx, session);
    }
    if (cmd == "clear") {
        return cmdClear(args, ctx, session);
    }
    if (cmd == "effect") {
        return cmdEffect(args, ctx, session);
    }
    if (cmd == "xp" || cmd == "experience") {
        return cmdXp(args, ctx, session);
    }
    if (cmd == "function") {
        if (args.empty()) {
            return CommandExecutionResult{.success = false, .message = "Uso: /function <nombre>", .affectedCount = 0};
        }
        return session.getFunctionRunner().runFunction(args[0], ctx, session);
    }

    return CommandExecutionResult{.success = false,
                                  .message = "Comando desconocido: /" + cmd + ". Escribe /help para ayuda.",
                                  .affectedCount = 0};
}

CommandExecutionResult CommandEngine::cmdHelp(const std::vector<std::string>& /*args*/, const CommandContext& /*ctx*/,
                                              WorldSession& /*session*/) {
    return CommandExecutionResult{.success = true,
                                  .message =
                                      "Comandos disponibles: /gamemode, /time, /weather, /tp, /give, /summon, "
                                      "/setblock, /fill, /gamerule, /kill, /clear, /effect, /xp, /function, /help",
                                  .affectedCount = 1};
}

CommandExecutionResult CommandEngine::cmdGameMode(const std::vector<std::string>& args, const CommandContext& ctx,
                                                  WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para cambiar modo de juego", .affectedCount = 0};
    }
    if (args.empty()) {
        return CommandExecutionResult{.success = false,
                                      .message =
                                          "Uso: /gamemode <survival|creative|adventure|spectator|0|1|2|3> [objetivo]",
                                      .affectedCount = 0};
    }

    std::string modeStr = args[0];
    std::transform(modeStr.begin(), modeStr.end(), modeStr.begin(), [](unsigned char c) { return std::tolower(c); });

    uint32_t targetMode = 0;
    if (modeStr == "survival" || modeStr == "s" || modeStr == "0")
        targetMode = 0;
    else if (modeStr == "creative" || modeStr == "c" || modeStr == "1")
        targetMode = 1;
    else if (modeStr == "adventure" || modeStr == "a" || modeStr == "2")
        targetMode = 2;
    else if (modeStr == "spectator" || modeStr == "sp" || modeStr == "3")
        targetMode = 3;
    else {
        return CommandExecutionResult{
            .success = false, .message = "Modo de juego inválido: " + modeStr, .affectedCount = 0};
    }

    std::string selector = (args.size() > 1) ? args[1] : "@s";
    auto targets = resolvePlayerTargets(selector, ctx, session);
    uint32_t affected = 0;

    for (uint32_t id : targets) {
        if (id == session.getPlayer().id) {
            // Si el mundo es Hardcore y el jugador murió (está en espectador), no puede volver a supervivencia
            if (session.isHardcore() && session.getPlayer().isSpectator() && targetMode != 3) {
                return CommandExecutionResult{
                    .success = false,
                    .message = "No se puede cambiar el modo de juego en un mundo Extremo después de morir.",
                    .affectedCount = 0};
            }
            session.getPlayer().gameMode = targetMode;
            if (targetMode == 1 || targetMode == 3) {
                session.getPlayer().isFlying = (targetMode == 3);
            } else {
                session.getPlayer().isFlying = false;
            }
            affected++;
        }
    }

    return CommandExecutionResult{
        .success = true, .message = "Modo de juego actualizado a " + modeStr, .affectedCount = affected};
}

CommandExecutionResult CommandEngine::cmdTime(const std::vector<std::string>& args, const CommandContext& ctx,
                                              WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para modificar el tiempo", .affectedCount = 0};
    }
    if (args.size() < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Uso: /time <set|add|query> <valor>", .affectedCount = 0};
    }

    std::string sub = args[0];
    std::string val = args[1];

    if (sub == "set") {
        uint64_t targetTick = 0;
        if (val == "day")
            targetTick = 1000;
        else if (val == "noon")
            targetTick = 6000;
        else if (val == "night")
            targetTick = 13000;
        else if (val == "midnight")
            targetTick = 18000;
        else {
            try {
                targetTick = std::stoull(val);
            } catch (...) {
                return CommandExecutionResult{
                    .success = false, .message = "Valor de tiempo numérico inválido", .affectedCount = 0};
            }
        }
        session.getClock().setTickCount(targetTick);
        return CommandExecutionResult{
            .success = true, .message = "Tiempo establecido en tick " + std::to_string(targetTick), .affectedCount = 1};
    } else if (sub == "add") {
        try {
            uint64_t delta = std::stoull(val);
            session.getClock().setTickCount(session.getClock().getTickCount() + delta);
            return CommandExecutionResult{.success = true,
                                          .message = "Se añadieron " + std::to_string(delta) + " ticks al tiempo",
                                          .affectedCount = 1};
        } catch (...) {
            return CommandExecutionResult{.success = false, .message = "Valor de adición inválido", .affectedCount = 0};
        }
    } else if (sub == "query") {
        return CommandExecutionResult{.success = true,
                                      .message = "Tiempo actual: " + std::to_string(session.getClock().getTickCount()) +
                                                 " ticks",
                                      .affectedCount = 1};
    }

    return CommandExecutionResult{.success = false, .message = "Subcomando de /time desconocido", .affectedCount = 0};
}

CommandExecutionResult CommandEngine::cmdWeather(const std::vector<std::string>& args, const CommandContext& ctx,
                                                 WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para cambiar el clima", .affectedCount = 0};
    }
    if (args.empty()) {
        return CommandExecutionResult{
            .success = false, .message = "Uso: /weather <clear|rain|thunder> [duración_ticks]", .affectedCount = 0};
    }

    std::string type = args[0];
    uint32_t duration = 6000;
    if (args.size() > 1) {
        try {
            duration = static_cast<uint32_t>(std::stoul(args[1]));
        } catch (...) {}
    }

    if (type == "clear") {
        session.setWeather(WeatherState::Clear, duration);
        return CommandExecutionResult{.success = true, .message = "Clima despejado", .affectedCount = 1};
    } else if (type == "rain") {
        session.setWeather(WeatherState::Rain, duration);
        return CommandExecutionResult{.success = true, .message = "Clima cambiado a lluvia", .affectedCount = 1};
    } else if (type == "thunder") {
        session.setWeather(WeatherState::Thunder, duration);
        return CommandExecutionResult{.success = true, .message = "Clima cambiado a tormenta", .affectedCount = 1};
    }

    return CommandExecutionResult{
        .success = false, .message = "Tipo de clima desconocido: " + type, .affectedCount = 0};
}

CommandExecutionResult CommandEngine::cmdTeleport(const std::vector<std::string>& args, const CommandContext& ctx,
                                                  WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para teletransportar", .affectedCount = 0};
    }
    if (args.empty()) {
        return CommandExecutionResult{.success = false, .message = "Uso: /tp [objetivo] <x y z>", .affectedCount = 0};
    }

    std::string selector = "@s";
    size_t coordIndex = 0;

    if (args.size() == 3) {
        coordIndex = 0;
    } else if (args.size() >= 4) {
        selector = args[0];
        coordIndex = 1;
    } else {
        return CommandExecutionResult{
            .success = false, .message = "Argumentos insuficientes para coordenadas", .affectedCount = 0};
    }

    float x = 0, y = 0, z = 0;
    if (!parseCoordinate(args[coordIndex], ctx.sourcePosition.x, x) ||
        !parseCoordinate(args[coordIndex + 1], ctx.sourcePosition.y, y) ||
        !parseCoordinate(args[coordIndex + 2], ctx.sourcePosition.z, z)) {
        return CommandExecutionResult{
            .success = false, .message = "Coordenadas de teletransporte inválidas", .affectedCount = 0};
    }

    auto targets = resolvePlayerTargets(selector, ctx, session);
    uint32_t moved = 0;
    for (uint32_t id : targets) {
        // Jugador local o remoto: todos los objetivos resueltos se teletransportan
        PlayerEntity* p = session.getPlayerById(id);
        if (!p) continue;
        p->position = Math::Vec3(x, y, z);
        p->prevPosition = p->position;
        p->velocity = Math::Vec3(0, 0, 0);
        p->fallDistance = 0.0f;
        ++moved;
    }

    return CommandExecutionResult{.success = moved > 0,
                                  .message = moved > 0 ? "Teletransportado a (" + std::to_string(x) + ", " +
                                                             std::to_string(y) + ", " + std::to_string(z) + ")"
                                                       : "Ningún jugador coincide con " + selector,
                                  .affectedCount = moved};
}

CommandExecutionResult CommandEngine::cmdGive(const std::vector<std::string>& args, const CommandContext& ctx,
                                              WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para /give", .affectedCount = 0};
    }
    if (args.size() < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Uso: /give <jugador> <ítem> [cantidad]", .affectedCount = 0};
    }

    std::string selector = args[0];
    std::string itemName = args[1];
    uint16_t count = 1;
    if (args.size() > 2) {
        try {
            count = static_cast<uint16_t>(std::stoul(args[2]));
        } catch (...) {
            count = 1;
        }
    }

    auto& itemReg = ItemRegistry::global();
    ItemId id = itemReg.getId(itemName);
    if (id == EMPTY_ITEM_ID) {
        return CommandExecutionResult{.success = false, .message = "Ítem desconocido: " + itemName, .affectedCount = 0};
    }

    auto targets = resolvePlayerTargets(selector, ctx, session);
    uint32_t affected = 0;
    for (uint32_t pid : targets) {
        if (pid == session.getPlayer().id) {
            ItemStack stack(id, count);
            session.getPlayer().inventory.addItem(stack);
            session.getPlayer().syncHotbar();
            affected++;
        }
    }

    return CommandExecutionResult{.success = true,
                                  .message = "Entregado " + std::to_string(count) + " de " + itemName,
                                  .affectedCount = affected};
}

CommandExecutionResult CommandEngine::cmdSummon(const std::vector<std::string>& args, const CommandContext& ctx,
                                                WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para invocar entidades", .affectedCount = 0};
    }
    if (args.empty()) {
        return CommandExecutionResult{
            .success = false, .message = "Uso: /summon <tipo_entidad> [x y z]", .affectedCount = 0};
    }

    std::string entityType = args[0];
    Math::Vec3 spawnPos = ctx.sourcePosition;
    if (args.size() >= 4) {
        float x = 0, y = 0, z = 0;
        if (parseCoordinate(args[1], ctx.sourcePosition.x, x) && parseCoordinate(args[2], ctx.sourcePosition.y, y) &&
            parseCoordinate(args[3], ctx.sourcePosition.z, z)) {
            spawnPos = Math::Vec3(x, y, z);
        }
    }

    // Cualquier criatura registrada, con o sin espacio de nombres y con sus nombres anteriores
    std::string fullName = entityType.find(':') == std::string::npos ? "opencraft:" + entityType : entityType;
    fullName = IdAliases::canonical(fullName);
    MobType mob = mobTypeFromString(fullName);
    if (mobTypeToString(mob) != fullName) {
        return CommandExecutionResult{
            .success = false, .message = "Entidad desconocida: " + entityType, .affectedCount = 0};
    }

    session.spawnMob(mob, spawnPos);
    return CommandExecutionResult{.success = true, .message = "Entidad invocada: " + entityType, .affectedCount = 1};
}

CommandExecutionResult CommandEngine::cmdSetBlock(const std::vector<std::string>& args, const CommandContext& ctx,
                                                  WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para /setblock", .affectedCount = 0};
    }
    if (args.size() < 4) {
        return CommandExecutionResult{.success = false,
                                      .message = "Uso: /setblock <x> <y> <z> <bloque> [replace|keep|destroy]",
                                      .affectedCount = 0};
    }

    int bx = 0, by = 0, bz = 0;
    if (!parseBlockPos(args[0], args[1], args[2], ctx.sourcePosition, bx, by, bz)) {
        return CommandExecutionResult{
            .success = false, .message = "Coordenadas de bloque inválidas", .affectedCount = 0};
    }

    std::string blockName = args[3];
    auto& reg = BlockRegistry::global();
    BlockStateId state = reg.getStateId(blockName);
    if (state == AIR_BLOCK_STATE && blockName != "opencraft:air" && blockName != "air") {
        return CommandExecutionResult{
            .success = false, .message = "Bloque desconocido: " + blockName, .affectedCount = 0};
    }

    std::string mode = (args.size() > 4) ? args[4] : "replace";
    if (mode == "keep" && session.getWorld().getBlockState(bx, by, bz) != AIR_BLOCK_STATE) {
        return CommandExecutionResult{
            .success = false, .message = "No se pudo colocar el bloque (el destino no está vacío)", .affectedCount = 0};
    }

    session.getWorld().setBlockState(bx, by, bz, state);
    return CommandExecutionResult{.success = true,
                                  .message = "Bloque colocado en (" + std::to_string(bx) + ", " + std::to_string(by) +
                                             ", " + std::to_string(bz) + ")",
                                  .affectedCount = 1};
}

CommandExecutionResult CommandEngine::cmdFill(const std::vector<std::string>& args, const CommandContext& ctx,
                                              WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para /fill", .affectedCount = 0};
    }
    if (args.size() < 7) {
        return CommandExecutionResult{.success = false,
                                      .message = "Uso: /fill <x1> <y1> <z1> <x2> <y2> <z2> <bloque> [replace|keep]",
                                      .affectedCount = 0};
    }

    int x1 = 0, y1 = 0, z1 = 0;
    int x2 = 0, y2 = 0, z2 = 0;
    if (!parseBlockPos(args[0], args[1], args[2], ctx.sourcePosition, x1, y1, z1) ||
        !parseBlockPos(args[3], args[4], args[5], ctx.sourcePosition, x2, y2, z2)) {
        return CommandExecutionResult{
            .success = false, .message = "Coordenadas de /fill inválidas", .affectedCount = 0};
    }

    int minX = std::min(x1, x2), maxX = std::max(x1, x2);
    int minY = std::min(y1, y2), maxY = std::max(y1, y2);
    int minZ = std::min(z1, z2), maxZ = std::max(z1, z2);

    size_t dx = static_cast<size_t>(maxX - minX + 1);
    size_t dy = static_cast<size_t>(maxY - minY + 1);
    size_t dz = static_cast<size_t>(maxZ - minZ + 1);
    size_t volume = dx * dy * dz;

    // Salvaguarda canónica estricta contra fill excesivo
    if (volume > MAX_FILL_VOLUME) {
        return CommandExecutionResult{.success = false,
                                      .message = "Error: Límite de bloques excedido: el volumen especificado (" +
                                                 std::to_string(volume) + ") supera el máximo permitido de " +
                                                 std::to_string(MAX_FILL_VOLUME) + " bloques.",
                                      .affectedCount = 0};
    }

    std::string blockName = args[6];
    auto& reg = BlockRegistry::global();
    BlockStateId state = reg.getStateId(blockName);
    if (state == AIR_BLOCK_STATE && blockName != "opencraft:air" && blockName != "air") {
        return CommandExecutionResult{
            .success = false, .message = "Bloque desconocido: " + blockName, .affectedCount = 0};
    }

    std::string mode = (args.size() > 7) ? args[7] : "replace";
    uint32_t filled = 0;

    for (int y = minY; y <= maxY; ++y) {
        for (int z = minZ; z <= maxZ; ++z) {
            for (int x = minX; x <= maxX; ++x) {
                if (mode == "keep" && session.getWorld().getBlockState(x, y, z) != AIR_BLOCK_STATE) {
                    continue;
                }
                session.getWorld().setBlockState(x, y, z, state);
                filled++;
            }
        }
    }

    return CommandExecutionResult{.success = true,
                                  .message = "Se rellenaron con éxito " + std::to_string(filled) + " bloques",
                                  .affectedCount = filled};
}

CommandExecutionResult CommandEngine::cmdGameRule(const std::vector<std::string>& args, const CommandContext& ctx,
                                                  WorldSession& /*session*/) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para /gamerule", .affectedCount = 0};
    }
    if (args.empty()) {
        auto all = m_gameRules.getAll();
        std::string list = "Reglas de juego: ";
        for (const auto& r : all) {
            list += r.first + "=" + r.second + ", ";
        }
        return CommandExecutionResult{
            .success = true, .message = list, .affectedCount = static_cast<uint32_t>(all.size())};
    }

    std::string ruleName = args[0];
    if (args.size() == 1) {
        std::string currentVal = m_gameRules.get(ruleName);
        if (currentVal == "unknown") {
            return CommandExecutionResult{
                .success = false, .message = "Regla de juego desconocida: " + ruleName, .affectedCount = 0};
        }
        return CommandExecutionResult{.success = true, .message = ruleName + " = " + currentVal, .affectedCount = 1};
    }

    std::string newVal = args[1];
    if (m_gameRules.set(ruleName, newVal)) {
        return CommandExecutionResult{
            .success = true, .message = "Regla " + ruleName + " establecida a " + newVal, .affectedCount = 1};
    }

    return CommandExecutionResult{
        .success = false, .message = "No se pudo establecer la regla " + ruleName, .affectedCount = 0};
}

CommandExecutionResult CommandEngine::cmdKill(const std::vector<std::string>& args, const CommandContext& ctx,
                                              WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para /kill", .affectedCount = 0};
    }

    std::string selector = args.empty() ? "@s" : args[0];
    auto targets = resolvePlayerTargets(selector, ctx, session);
    for (uint32_t id : targets) {
        if (id == session.getPlayer().id) {
            session.applyDamage(1000.0f, DamageCause::Void);
        }
    }

    return CommandExecutionResult{
        .success = true, .message = "Objetivos eliminados", .affectedCount = static_cast<uint32_t>(targets.size())};
}

CommandExecutionResult CommandEngine::cmdClear(const std::vector<std::string>& args, const CommandContext& ctx,
                                               WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para /clear", .affectedCount = 0};
    }

    std::string selector = args.empty() ? "@s" : args[0];
    auto targets = resolvePlayerTargets(selector, ctx, session);
    for (uint32_t id : targets) {
        if (id == session.getPlayer().id) {
            session.getPlayer().inventory.clear();
            session.getPlayer().syncHotbar();
        }
    }

    return CommandExecutionResult{
        .success = true, .message = "Inventario vaciado", .affectedCount = static_cast<uint32_t>(targets.size())};
}

CommandExecutionResult CommandEngine::cmdEffect(const std::vector<std::string>& args, const CommandContext& ctx,
                                                WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para /effect", .affectedCount = 0};
    }
    if (args.size() < 2) {
        return CommandExecutionResult{.success = false,
                                      .message =
                                          "Uso: /effect <give|clear> <objetivo> [efecto] [duración] [amplificador]",
                                      .affectedCount = 0};
    }

    std::string sub = args[0];
    std::string selector = args[1];
    auto targets = resolvePlayerTargets(selector, ctx, session);

    if (sub == "clear") {
        for (uint32_t id : targets) {
            if (id == session.getPlayer().id) {
                session.getPlayer().statusEffects.clearAllEffects();
            }
        }
        return CommandExecutionResult{
            .success = true, .message = "Efectos limpiados", .affectedCount = static_cast<uint32_t>(targets.size())};
    }

    if (args.size() < 3) {
        return CommandExecutionResult{.success = false, .message = "Falta especificar efecto", .affectedCount = 0};
    }

    std::string effectName = args[2];
    uint32_t durationTicks = 600; // 30 segundos
    uint8_t amplifier = 0;

    if (args.size() > 3) {
        try {
            durationTicks = static_cast<uint32_t>(std::stoul(args[3]) * 20);
        } catch (...) {}
    }
    if (args.size() > 4) {
        try {
            amplifier = static_cast<uint8_t>(std::stoul(args[4]));
        } catch (...) {}
    }

    EffectType effType = StatusEffectManager::getEffectFromName(effectName);

    for (uint32_t id : targets) {
        if (id == session.getPlayer().id) {
            session.getPlayer().statusEffects.addEffect(ActiveEffect{
                .type = effType, .durationTicks = durationTicks, .amplifier = amplifier, .isAmbient = false});
        }
    }

    return CommandExecutionResult{.success = true,
                                  .message = "Efecto aplicado: " + effectName,
                                  .affectedCount = static_cast<uint32_t>(targets.size())};
}

CommandExecutionResult CommandEngine::cmdXp(const std::vector<std::string>& args, const CommandContext& ctx,
                                            WorldSession& session) {
    if (ctx.permissionLevel < 2) {
        return CommandExecutionResult{
            .success = false, .message = "Permisos insuficientes para /xp", .affectedCount = 0};
    }
    if (args.size() < 3) {
        return CommandExecutionResult{
            .success = false, .message = "Uso: /xp add <objetivo> <cantidad> [levels|points]", .affectedCount = 0};
    }

    std::string sub = args[0];
    std::string selector = args[1];
    uint32_t amount = 0;
    try {
        amount = static_cast<uint32_t>(std::stoul(args[2]));
    } catch (...) {
        return CommandExecutionResult{.success = false, .message = "Cantidad numérica inválida", .affectedCount = 0};
    }

    bool isLevels = (args.size() > 3 && (args[3] == "levels" || args[3] == "l"));
    auto targets = resolvePlayerTargets(selector, ctx, session);

    for (uint32_t id : targets) {
        if (id == session.getPlayer().id) {
            if (isLevels) {
                session.getPlayer().xpLevel += amount;
            } else {
                session.getPlayer().addExperience(amount);
            }
        }
    }

    return CommandExecutionResult{.success = true,
                                  .message = "Experiencia otorgada: " + std::to_string(amount) +
                                             (isLevels ? " niveles" : " puntos"),
                                  .affectedCount = static_cast<uint32_t>(targets.size())};
}

} // namespace OpenCraft
