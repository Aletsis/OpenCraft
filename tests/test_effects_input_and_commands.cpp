#include <cassert>
#include <cmath>
#include <iostream>
#include "game/TutorialSystem.hpp"
#include "input/GamepadController.hpp"
#include "physics/Raycast.hpp"
#include "platform/PlatformPaths.hpp"
#include "progression/StatusEffect.hpp"
#include "render/ParticleSystem.hpp"
#include "sim/WorldSession.hpp"

using namespace OpenCraft;

static bool near(float a, float b) { return std::abs(a - b) < 1e-3f; }

static void testStatusEffects() {
    // Nombres en ambos sentidos, incluido el identificador anterior de Decaimiento
    for (uint8_t i = 1; i <= 26; ++i) {
        auto t = static_cast<EffectType>(i);
        assert(StatusEffectManager::getEffectFromName(StatusEffectManager::getEffectName(t)) == t);
    }
    assert(StatusEffectManager::getEffectFromName("wither") == EffectType::Decay);

    StatusEffectManager m;
    m.addEffect({EffectType::Speed, 100, 0});
    m.addEffect({EffectType::Speed, 50, 1}); // nivel mayor sustituye
    assert(m.getAmplifier(EffectType::Speed) == 1);
    assert(m.getSpeedMultiplier() > 1.0f);
    m.addEffect({EffectType::Slowness, 100, 0});
    m.addEffect({EffectType::Strength, 100, 0});
    m.addEffect({EffectType::Weakness, 100, 0});
    m.addEffect({EffectType::Resistance, 100, 1});
    m.addEffect({EffectType::FireResistance, 100, 0});
    assert(m.getIncomingDamageFactor() < 1.0f);
    assert(m.hasFireImmunity());
    (void)m.getDamageMultiplier();
    m.removeEffect(EffectType::Speed);
    assert(!m.hasEffect(EffectType::Speed) && m.getEffect(EffectType::Speed) == nullptr);
    m.clearAllEffects();
    assert(near(m.getSpeedMultiplier(), 1.0f) && near(m.getDamageMultiplier(), 1.0f));
    assert(near(m.getIncomingDamageFactor(), 1.0f) && !m.hasFireImmunity());

    // Regeneración cura, veneno no mata, decaimiento sí resta vida; los efectos caducan
    PlayerEntity p;
    p.health = 10.0f;
    m.addEffect({EffectType::Regeneration, 200, 1});
    for (int i = 0; i < 200; ++i) m.tick(p, nullptr);
    assert(p.health > 10.0f && !m.hasEffect(EffectType::Regeneration));
    p.health = 3.0f;
    m.addEffect({EffectType::Poison, 400, 1});
    for (int i = 0; i < 400; ++i) m.tick(p, nullptr);
    assert(near(p.health, 1.0f) && "El veneno deja al jugador con medio corazón");
    p.health = 5.0f;
    m.addEffect({EffectType::Decay, 400, 0});
    for (int i = 0; i < 400; ++i) m.tick(p, nullptr);
    assert(p.health < 1.0f);

    MobEntity mob(1, MobType::Cow, Math::Vec3(0, 0, 0));
    mob.health = 5.0f;
    StatusEffectManager mm;
    mm.addEffect({EffectType::Regeneration, 100, 2});
    mm.addEffect({EffectType::Poison, 100, 2});
    mm.addEffect({EffectType::Decay, 100, 2});
    for (int i = 0; i < 100; ++i) mm.tickMob(mob, nullptr);
    assert(!mm.hasEffect(EffectType::Poison));
    std::cout << "[OK] Efectos de estado" << std::endl;
}

static void testParticles() {
    World world;
    ParticleSystem ps(64);
    assert(ps.getMaxParticles() == 64);
    ps.spawnBlockBreakParticles(Math::Vec3(0, 70, 0), 0.5f, 0.5f, 0.5f, 10);
    ps.spawnSmoke(Math::Vec3(0, 70, 0));
    ps.spawnCrit(Math::Vec3(0, 70, 0));
    ps.spawnSplash(Math::Vec3(0, 70, 0));
    size_t n = ps.getActiveParticleCount();
    assert(n == 10 + 4 + 8 + 12);
    for (int i = 0; i < 100; ++i) ps.spawnSmoke(Math::Vec3(0, 70, 0)); // el anillo no crece
    assert(ps.getActiveParticleCount() <= 64 && ps.getParticles().size() == 64);
    for (int i = 0; i < 200; ++i) ps.update(0.05f, &world);
    assert(ps.getActiveParticleCount() == 0 && "Las partículas caducan");
    ps.spawnCrit(Math::Vec3(0, 70, 0));
    ps.setMaxParticles(4);
    assert(ps.getMaxParticles() == 64 && "Mínimo de 64 partículas");
    ps.clear();
    assert(ps.getActiveParticleCount() == 0);

    auto& swing = ps.getToolSwing();
    swing.trigger();
    assert(swing.isSwinging);
    for (int i = 0; i < 20; ++i) swing.update(0.05f);
    assert(!swing.isSwinging);
    std::cout << "[OK] Partículas y animación de herramienta" << std::endl;
}

static void testTutorial() {
    TutorialSystem t;
    assert(!t.isCompleted() && !t.getCurrentTitle().empty() && !t.getCurrentDescription().empty());
    t.onBlockMined(static_cast<uint16_t>(BlockID::Wood)); // fuera de orden: no avanza
    t.onPlayerMoved(6.0f);
    std::string step1 = t.getCurrentTitle();
    t.onPlayerRotated(45.0f);
    assert(t.getCurrentTitle() != step1);
    t.onBlockMined(static_cast<uint16_t>(BlockID::Stone));
    t.onBlockMined(static_cast<uint16_t>(BlockID::Wood));
    t.onItemCrafted(static_cast<uint16_t>(BlockID::Planks));
    t.onBlockPlaced(static_cast<uint16_t>(BlockID::CraftingTable));
    assert(!t.isCompleted());
    t.onGameSaved();
    assert(t.isCompleted() && !t.getCurrentTitle().empty() && !t.getCurrentDescription().empty());
    t.reset();
    assert(!t.isCompleted());
    std::cout << "[OK] Tutorial" << std::endl;
}

static void testGamepad() {
    GamepadController g;
    g.updateAxis(0, 0.1f);
    assert(near(g.getMovementVector().x, 0.0f) && "Zona muerta");
    g.updateAxis(1, -1.0f); // stick hacia arriba
    assert(near(g.getMovementVector().y, 1.0f) && "Arriba es avanzar");
    g.updateAxis(2, 0.5f);
    g.updateAxis(3, -0.5f);
    assert(g.getLookVector(2.0f).x > 0.0f && g.getLookVector().y < 0.0f);
    g.updateAxis(4, 0.9f);
    g.updateAxis(5, 2.0f);
    assert(g.isUsePressed() && g.isAttackPressed() && near(g.getState().rightTrigger, 1.0f));
    g.updateAxis(99, 1.0f);
    for (int b = 0; b <= 14; ++b) g.updateButton(b, true);
    assert(g.isJumpPressed() && g.isSneakPressed());
    assert(g.consumeInventoryTrigger() && !g.consumeInventoryTrigger());
    assert(g.consumePauseTrigger() && !g.consumePauseTrigger());
    (void)g.consumeHotbarDelta();
    assert(g.consumeHotbarDelta() == 0);
    g.updateButton(4, false);
    g.updateButton(5, false);
    g.updateButton(5, true);
    assert(g.consumeHotbarDelta() == 1);
    g.updateButton(4, true);
    assert(g.consumeHotbarDelta() == -1);
    for (int b = 0; b <= 14; ++b) g.updateButton(b, false);
    g.reset();
    assert(!g.isJumpPressed() && near(g.getMovementVector().y, 0.0f));
    std::cout << "[OK] Mando" << std::endl;
}

static void testRaycast() {
    World w;
    auto stone = BlockRegistry::global().fromLegacyId(BlockID::Stone);
    w.setChunk(0, 0, std::make_unique<Chunk>(0, 0));
    w.setBlockState(5, 70, 5, stone);
    RaycastHit h = VoxelRaycast::cast(w, Math::Vec3(5.5f, 70.5f, 1.5f), Math::Vec3(0, 0, 1), 6.0f);
    assert(h.hit && h.blockX == 5 && h.blockY == 70 && h.blockZ == 5);
    assert(near(h.normal.z, -1.0f) && h.blockID == BlockID::Stone);
    h = VoxelRaycast::cast(w, Math::Vec3(5.5f, 75.5f, 5.5f), Math::Vec3(0, -1, 0), 6.0f);
    assert(h.hit && h.blockY == 70 && near(h.normal.y, 1.0f));
    h = VoxelRaycast::cast(w, Math::Vec3(9.5f, 70.5f, 5.5f), Math::Vec3(-1, 0, 0), 6.0f);
    assert(h.hit && near(h.normal.x, 1.0f));
    h = VoxelRaycast::cast(w, Math::Vec3(5.5f, 70.5f, 1.5f), Math::Vec3(0, 0, 1), 2.0f);
    assert(!h.hit && "Fuera de alcance");
    h = VoxelRaycast::cast(w, Math::Vec3(5.5f, 70.5f, 1.5f), Math::Vec3(0, 0, 0), 6.0f);
    assert(!h.hit && "Dirección nula");
    h = VoxelRaycast::cast(w, Math::Vec3(1.2f, 71.3f, 1.7f), Math::Vec3(1, -0.2f, 0.9f).normalized(), 8.0f);
    (void)h;
    std::cout << "[OK] Raycast de vóxeles" << std::endl;
}

static void testPlatformPaths() {
    auto base = std::filesystem::temp_directory_path() / "opencraft_paths_test";
    std::filesystem::remove_all(base);
    PlatformPaths::setOverrideBaseDir(base);
    assert(!PlatformPaths::getPlatformName().empty());
    (void)PlatformPaths::getPlatformType();
    auto data = PlatformPaths::getUserDataDir();
    assert(data.string().starts_with(base.string()));
    assert(PlatformPaths::getSavesDir().string().starts_with(base.string()));
    assert(PlatformPaths::getBackupsDir().string().starts_with(base.string()));
    assert(PlatformPaths::getUserConfigDir().string().starts_with(base.string()));
    PlatformPaths::clearOverrideBaseDir();
    assert(!PlatformPaths::getUserDataDir().empty() && !PlatformPaths::getUserConfigDir().empty());
    (void)PlatformPaths::getExecutableDir();
    assert(!PlatformPaths::getAssetSearchPaths().empty());
    (void)PlatformPaths::resolveAsset("shaders/no_existe.vert");
    std::filesystem::remove_all(base);
    std::cout << "[OK] Rutas de plataforma" << std::endl;
}

static void testCommands() {
    WorldSession s;
    s.init(2, 1);
    auto& ce = s.getCommandEngine();
    CommandContext ctx;
    ctx.sourceEntityId = LOCAL_PLAYER_ENTITY_ID;
    ctx.sourcePosition = s.getPlayer().position;
    auto run = [&](const std::string& c) { return ce.execute(c, ctx, s); };

    assert(run("/help").success);
    assert(!run("/no_existe").success);
    assert(run("/gamemode creative").success && s.getPlayer().gameMode == 1);
    assert(run("/gamemode s").success && s.getPlayer().gameMode == 0);
    assert(!run("/gamemode banana").success);
    assert(run("/time set 13000").success);
    assert(run("/time add 100").success);
    assert(run("/time set day").success && run("/time set night").success);
    assert(!run("/time set").success);
    assert(run("/weather rain").success && run("/weather thunder").success && run("/weather clear").success);
    assert(!run("/weather nieve").success);
    assert(run("/tp @s 10 120 10").success && near(s.getPlayer().position.x, 10.0f));
    assert(run("/tp ~1 ~ ~-1").success);
    assert(!run("/tp @s x y z").success);
    assert(run("/give @s opencraft:stone 5").success);
    assert(!run("/give @s opencraft:no_existe 1").success);
    assert(run("/clear @s").success);
    assert(run("/summon opencraft:cow 12 120 12").success);
    assert(!run("/summon opencraft:no_existe 0 0 0").success);
    assert(run("/kill @e").success);
    assert(run("/setblock 3 100 3 opencraft:stone").success);
    assert(!run("/setblock 3 100 3 opencraft:no_existe").success);
    assert(run("/fill 0 101 0 2 102 2 opencraft:glass").success);
    assert(!run("/fill 0 0 0 200 200 200 opencraft:stone").success && "Volumen excesivo");
    assert(run("/gamerule keepInventory true").success);
    assert(run("/gamerule keepInventory").success);
    assert(!run("/gamerule noExiste true").success);
    assert(run("/effect give @s opencraft:speed 30 1").success);
    assert(run("/effect clear @s").success);
    assert(run("/xp add @s 50").success);
    assert(run("/experience add @s 2 levels").success);
    ctx.permissionLevel = 0;
    assert(!run("/gamemode creative").success && "Sin permisos");
    ctx.permissionLevel = 4;
    float out = 0.0f;
    assert(CommandEngine::parseCoordinate("~5", 10.0f, out) && near(out, 15.0f));
    assert(CommandEngine::parseCoordinate("-3.5", 10.0f, out) && near(out, -3.5f));
    assert(!CommandEngine::parseCoordinate("abc", 0.0f, out));
    std::cout << "[OK] Comandos" << std::endl;
}

int main() {
    std::cout << "--- Efectos, partículas, tutorial, mando, raycast, rutas y comandos ---" << std::endl;
    testStatusEffects();
    testParticles();
    testTutorial();
    testGamepad();
    testRaycast();
    testPlatformPaths();
    testCommands();
    std::cout << "EXITO: efectos, entrada y comandos verificados." << std::endl;
    return 0;
}
