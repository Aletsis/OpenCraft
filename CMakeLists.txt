cmake_minimum_required(VERSION 3.20)
project(OpenCraft VERSION 1.0.0 LANGUAGES C CXX)

# Etiqueta de pre-lanzamiento (vacía para la versión final); única fuente de la versión del producto
set(OPENCRAFT_VERSION_SUFFIX "-rc1")
set(OPENCRAFT_VERSION_STRING "${PROJECT_VERSION}${OPENCRAFT_VERSION_SUFFIX}")
add_compile_definitions(OPENCRAFT_VERSION_STRING="${OPENCRAFT_VERSION_STRING}")
file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "${OPENCRAFT_VERSION_STRING}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and optimization flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(OPENCRAFT_BUILD_CLIENT "Compilar el cliente gráfico (requiere SDL3 y OpenGL ES 3)" ON)
option(OPENCRAFT_BUILD_TESTS "Compilar la suite de pruebas automáticas" ON)

# SDL3 y GLES solo son necesarios para el cliente: el servidor dedicado y las pruebas no los usan
if(OPENCRAFT_BUILD_CLIENT)
    find_package(PkgConfig REQUIRED)
    find_package(SDL3 QUIET CONFIG)
    if(NOT SDL3_FOUND)
        pkg_check_modules(SDL3 REQUIRED sdl3)
    endif()
    pkg_check_modules(GLESV2 REQUIRED glesv2)
endif()

# ==============================================================================
# Biblioteca de Simulación y Datos (C++20 puro, sin dependencias de SDL ni GLES)
# ==============================================================================
add_library(opencraft_sim STATIC
    src/item/Item.cpp
    src/item/ItemStack.cpp
    src/item/Inventory.cpp
    src/sim/ItemEntity.cpp
    src/world/BlockRegistry.cpp
    src/world/IdAliases.cpp
    src/world/ChunkSection.cpp
    src/world/Chunk.cpp
    src/world/World.cpp
    src/world/WorldGenerator.cpp
    src/world/ChunkWorkerPool.cpp
    src/world/ChunkManager.cpp
    src/world/ChunkLighting.cpp
    src/world/ChunkMesher.cpp
    src/core/PngWriter.cpp
    src/render/AtlasPainter.cpp
    src/ui/BitmapFont.cpp
    src/render/Frustum.cpp
    src/sim/WorldSession.cpp
    src/physics/Raycast.cpp
    src/storage/WorldFormat.cpp
    src/storage/WorldStorage.cpp
    src/storage/WorldManager.cpp
    src/crafting/RecipeRegistry.cpp
    src/world/FurnaceBlockEntity.cpp
    src/world/LightEngine.cpp
    src/world/FluidSimulator.cpp
    src/sim/MobEntity.cpp
    src/sim/ProjectileEntity.cpp
    src/sim/ExperienceOrbEntity.cpp
    src/sim/MobSpawner.cpp
    src/network/PacketValidator.cpp
    src/network/ReliableChannel.cpp
    src/network/NetworkServer.cpp
    src/network/NetworkClient.cpp
    src/world/Biome.cpp
    src/world/StructureManager.cpp
    src/world/Agriculture.cpp
    src/sim/VillagerEntity.cpp
    src/sim/AnimalBreeding.cpp
    src/sim/BoatEntity.cpp
    src/world/HopperBlockEntity.cpp
    src/world/AutomationBlockEntities.cpp
    src/world/PistonSimulator.cpp
    src/world/CinnabarEngine.cpp
    src/sim/MinecartEntity.cpp
    src/progression/ProgressionGraph.cpp
    src/progression/Enchantment.cpp
    src/progression/StatusEffect.cpp
    src/world/BrewingStandBlockEntity.cpp
    src/world/BeaconBlockEntity.cpp
    src/world/Dimension.cpp
    src/world/PortalSystem.cpp
    src/sim/BossEntities.cpp
    src/sim/CommandEngine.cpp
    src/sim/FunctionRunner.cpp
    src/world/CommandBlockEntity.cpp
    src/world/TrialSpawnerBlockEntity.cpp
    src/world/VaultBlockEntity.cpp
    src/content/ContentPackManager.cpp
    src/progression/AdvancementManager.cpp
    src/progression/StatisticsManager.cpp
    src/platform/PlatformPaths.cpp
    src/platform/DeviceSettings.cpp
    src/platform/LifecycleManager.cpp
    src/audio/SpatialAudio.cpp
    src/audio/AudioSubtitles.cpp
    src/input/TouchController.cpp
    src/input/GamepadController.cpp
    src/render/ParticleSystem.cpp
    src/ui/LocalizationManager.cpp
    src/game/TutorialSystem.cpp
    src/benchmark/BenchmarkHarness.cpp
)

target_include_directories(opencraft_sim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ==============================================================================
# Servidor Dedicado Headless de OpenCraft (Fase 09 — Sin ventana ni GPU)
# ==============================================================================
add_executable(opencraft_server
    src/server_main.cpp
)
target_include_directories(opencraft_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(opencraft_server PRIVATE
    opencraft_sim
)

if(OPENCRAFT_BUILD_CLIENT)
# ==============================================================================
# Ejecutable Principal de OpenCraft (Aplicación, Renderizado, UI, Red)
# ==============================================================================
add_executable(opencraft
    src/main.cpp
    src/app/Application.cpp
    src/core/Window.cpp
    src/render/Shader.cpp
    src/render/TextureAtlas.cpp
    src/render/WorldRenderer.cpp
    src/render/EntityRenderer.cpp
    src/world/ChunkMesh.cpp
    src/ui/HUD.cpp
    src/ui/UIBatch.cpp
    src/ui/MenuSystem.cpp
    src/ui/ItemNames.cpp
    src/audio/AudioOutput.cpp
    src/network/LANDiscovery.cpp
    src/network/NetworkManager.cpp
)

target_include_directories(opencraft PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL3_INCLUDE_DIRS}
    ${GLESV2_INCLUDE_DIRS}
)

target_link_libraries(opencraft PRIVATE
    opencraft_sim
    ${GLESV2_LIBRARIES}
)
target_link_directories(opencraft PRIVATE ${GLESV2_LIBRARY_DIRS})

if(TARGET SDL3::SDL3)
    target_link_libraries(opencraft PRIVATE SDL3::SDL3)
else()
    target_link_libraries(opencraft PRIVATE ${SDL3_LIBRARIES})
    target_link_directories(opencraft PRIVATE ${SDL3_LIBRARY_DIRS})
endif()

# Copy or link assets next to the binary
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_CURRENT_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory"
)
add_dependencies(opencraft copy_assets)

endif()

# ==============================================================================
# Instalación (FHS): bin/ y share/opencraft/assets; el ejecutable resuelve los recursos
# relativos a su propia ubicación, de modo que el árbol instalado es reubicable
# ==============================================================================
include(GNUInstallDirs)
install(TARGETS opencraft_server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(OPENCRAFT_BUILD_CLIENT)
    install(TARGETS opencraft RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY assets/shaders assets/audio DESTINATION ${CMAKE_INSTALL_DATADIR}/opencraft/assets)
install(FILES assets/packaging/opencraft.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/packaging/opencraft.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES assets/packaging/opencraft.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps)
install(FILES LICENSE NOTICE README.md CHANGELOG.md docs/MANUAL.md docs/referencia/diferencias_conocidas.md
        DESTINATION ${CMAKE_INSTALL_DOCDIR})

if(OPENCRAFT_BUILD_TESTS)
# ==============================================================================
# Suite de Pruebas Automatizadas (CTest sin GPU)
# ==============================================================================
enable_testing()

add_executable(test_simulation tests/test_simulation.cpp)
target_link_libraries(test_simulation PRIVATE opencraft_sim)
add_test(NAME SimulationDeterminismTest COMMAND test_simulation)

add_executable(test_registry tests/test_registry.cpp)
target_link_libraries(test_registry PRIVATE opencraft_sim)
add_test(NAME BlockRegistryTest COMMAND test_registry)

add_executable(test_packets tests/test_packets.cpp)
target_link_libraries(test_packets PRIVATE opencraft_sim)
add_test(NAME NetworkPacketSecurityTest COMMAND test_packets)

add_executable(test_commands tests/test_commands.cpp)
target_link_libraries(test_commands PRIVATE opencraft_sim)
add_test(NAME GameCommandsAndChunkBordersTest COMMAND test_commands)

add_executable(test_persistence tests/test_persistence.cpp)
target_link_libraries(test_persistence PRIVATE opencraft_sim)
add_test(NAME PersistenceDeterminismTest COMMAND test_persistence)

add_executable(test_transactions tests/test_transactions.cpp)
target_link_libraries(test_transactions PRIVATE opencraft_sim)
add_test(NAME TransactionRecoveryTest COMMAND test_transactions)

add_executable(test_corruption tests/test_corruption.cpp)
target_link_libraries(test_corruption PRIVATE opencraft_sim)
add_test(NAME CorruptionAndIntegrityTest COMMAND test_corruption)

add_executable(test_migration tests/test_migration.cpp)
target_link_libraries(test_migration PRIVATE opencraft_sim)
add_test(NAME SchemaMigrationAndBackupTest COMMAND test_migration)

add_executable(test_world_manager tests/test_world_manager.cpp)
target_link_libraries(test_world_manager PRIVATE opencraft_sim)
add_test(NAME WorldManagerAndLockTest COMMAND test_world_manager)

# Pruebas de Fase 03 (Streaming y Generación Escalable)
add_executable(test_generator_determinism tests/test_generator_determinism.cpp)
target_link_libraries(test_generator_determinism PRIVATE opencraft_sim)
add_test(NAME GeneratorDeterminismTest COMMAND test_generator_determinism)

add_executable(test_streaming_cache tests/test_streaming_cache.cpp)
target_link_libraries(test_streaming_cache PRIVATE opencraft_sim)
add_test(NAME StreamingCacheAndMemoryTest COMMAND test_streaming_cache)

add_executable(test_streaming_persistence tests/test_streaming_persistence.cpp)
target_link_libraries(test_streaming_persistence PRIVATE opencraft_sim)
add_test(NAME StreamingPersistenceTest COMMAND test_streaming_persistence)

add_executable(test_concurrency_boundaries tests/test_concurrency_boundaries.cpp)
target_link_libraries(test_concurrency_boundaries PRIVATE opencraft_sim)
add_test(NAME ConcurrencyAndBoundariesTest COMMAND test_concurrency_boundaries)

add_executable(test_frustum_culling tests/test_frustum_culling.cpp)
target_link_libraries(test_frustum_culling PRIVATE opencraft_sim)
add_test(NAME FrustumCullingTest COMMAND test_frustum_culling)

# Pruebas de Fase 04 (Inventario, Objetos y Contenedores)
add_executable(test_inventory_conservation tests/test_inventory_conservation.cpp)
target_link_libraries(test_inventory_conservation PRIVATE opencraft_sim)
add_test(NAME InventoryConservationTest COMMAND test_inventory_conservation)

add_executable(test_item_entity_pickup tests/test_item_entity_pickup.cpp)
target_link_libraries(test_item_entity_pickup PRIVATE opencraft_sim)
add_test(NAME ItemEntityPickupTest COMMAND test_item_entity_pickup)

add_executable(test_entities_persistence_and_transaction tests/test_entities_persistence_and_transaction.cpp)
target_link_libraries(test_entities_persistence_and_transaction PRIVATE opencraft_sim)
add_test(NAME EntitiesPersistenceAndTransactionTest COMMAND test_entities_persistence_and_transaction)

add_executable(test_dimension_travel_and_persistence tests/test_dimension_travel_and_persistence.cpp)
target_link_libraries(test_dimension_travel_and_persistence PRIVATE opencraft_sim)
add_test(NAME DimensionTravelAndPersistenceTest COMMAND test_dimension_travel_and_persistence)

add_executable(test_chest_transactions tests/test_chest_transactions.cpp)
target_link_libraries(test_chest_transactions PRIVATE opencraft_sim)
add_test(NAME ChestTransactionsTest COMMAND test_chest_transactions)

add_executable(test_inventory_persistence tests/test_inventory_persistence.cpp)
target_link_libraries(test_inventory_persistence PRIVATE opencraft_sim)
add_test(NAME InventoryPersistenceTest COMMAND test_inventory_persistence)

# ==============================================================================
# Pruebas de Fase 05 (Minería, Fabricación y Herramientas)
# ==============================================================================
add_executable(test_production_chain tests/test_production_chain.cpp)
target_link_libraries(test_production_chain PRIVATE opencraft_sim)
add_test(NAME ProductionChainTest COMMAND test_production_chain)

add_executable(test_recipe_capacity_and_batch tests/test_recipe_capacity_and_batch.cpp)
target_link_libraries(test_recipe_capacity_and_batch PRIVATE opencraft_sim)
add_test(NAME RecipeCapacityAndBatchTest COMMAND test_recipe_capacity_and_batch)

add_executable(test_furnace_simulation_persistence tests/test_furnace_simulation_persistence.cpp)
target_link_libraries(test_furnace_simulation_persistence PRIVATE opencraft_sim)
add_test(NAME FurnaceSimulationPersistenceTest COMMAND test_furnace_simulation_persistence)

add_executable(test_tool_durability_and_mining tests/test_tool_durability_and_mining.cpp)
target_link_libraries(test_tool_durability_and_mining PRIVATE opencraft_sim)
add_test(NAME ToolDurabilityAndMiningTest COMMAND test_tool_durability_and_mining)

# ==============================================================================
# Pruebas de Fase 06 (Supervivencia, Movimiento y Reglas de Partida)
# ==============================================================================
add_executable(test_survival_stats tests/test_survival_stats.cpp)
target_link_libraries(test_survival_stats PRIVATE opencraft_sim)
add_test(NAME SurvivalStatsTest COMMAND test_survival_stats)

add_executable(test_damage_and_hazards tests/test_damage_and_hazards.cpp)
target_link_libraries(test_damage_and_hazards PRIVATE opencraft_sim)
add_test(NAME DamageAndHazardsTest COMMAND test_damage_and_hazards)

add_executable(test_movement_and_collision tests/test_movement_and_collision.cpp)
target_link_libraries(test_movement_and_collision PRIVATE opencraft_sim)
add_test(NAME MovementAndCollisionTest COMMAND test_movement_and_collision)

add_executable(test_death_and_respawn tests/test_death_and_respawn.cpp)
target_link_libraries(test_death_and_respawn PRIVATE opencraft_sim)
add_test(NAME DeathAndRespawnTest COMMAND test_death_and_respawn)

add_executable(test_bed_and_gamemodes tests/test_bed_and_gamemodes.cpp)
target_link_libraries(test_bed_and_gamemodes PRIVATE opencraft_sim)
add_test(NAME BedAndGameModesTest COMMAND test_bed_and_gamemodes)

# ==============================================================================
# Pruebas de Fase 07 (Luz, Tiempo, Clima y Simulación Ambiental)
# ==============================================================================
add_executable(test_lighting_and_propagation tests/test_lighting_and_propagation.cpp)
target_link_libraries(test_lighting_and_propagation PRIVATE opencraft_sim)
add_test(NAME LightingAndPropagationTest COMMAND test_lighting_and_propagation)

add_executable(test_fluid_flow_and_interaction tests/test_fluid_flow_and_interaction.cpp)
target_link_libraries(test_fluid_flow_and_interaction PRIVATE opencraft_sim)
add_test(NAME FluidFlowAndInteractionTest COMMAND test_fluid_flow_and_interaction)

add_executable(test_immersion_and_environmental_hazards tests/test_immersion_and_environmental_hazards.cpp)
target_link_libraries(test_immersion_and_environmental_hazards PRIVATE opencraft_sim)
add_test(NAME ImmersionAndEnvironmentalHazardsTest COMMAND test_immersion_and_environmental_hazards)

add_executable(test_scheduled_updates_and_weather tests/test_scheduled_updates_and_weather.cpp)
target_link_libraries(test_scheduled_updates_and_weather PRIVATE opencraft_sim)
add_test(NAME ScheduledUpdatesAndWeatherTest COMMAND test_scheduled_updates_and_weather)

add_executable(test_render_passes_and_ao tests/test_render_passes_and_ao.cpp)
target_link_libraries(test_render_passes_and_ao PRIVATE opencraft_sim)
add_test(NAME RenderPassesAndAoTest COMMAND test_render_passes_and_ao)

# ==============================================================================
# Pruebas de Fase 08 (Criaturas, IA y Combate)
# ==============================================================================
add_executable(test_pathfinding_and_navigation tests/test_pathfinding_and_navigation.cpp)
target_link_libraries(test_pathfinding_and_navigation PRIVATE opencraft_sim)
add_test(NAME PathfindingAndNavigationTest COMMAND test_pathfinding_and_navigation)

add_executable(test_mob_spawning_and_light tests/test_mob_spawning_and_light.cpp)
target_link_libraries(test_mob_spawning_and_light PRIVATE opencraft_sim)
add_test(NAME MobSpawningAndLightRulesTest COMMAND test_mob_spawning_and_light)

add_executable(test_combat_and_loot_transactions tests/test_combat_and_loot_transactions.cpp)
target_link_libraries(test_combat_and_loot_transactions PRIVATE opencraft_sim)
add_test(NAME CombatDamageAndLootTransactionTest COMMAND test_combat_and_loot_transactions)

add_executable(test_mob_archetypes_and_persistence tests/test_mob_archetypes_and_persistence.cpp)
target_link_libraries(test_mob_archetypes_and_persistence PRIVATE opencraft_sim)
add_test(NAME MobArchetypesAndPersistenceTest COMMAND test_mob_archetypes_and_persistence)

# ==============================================================================
# Pruebas de Fase 09 (Multijugador Autoritativo y Servidor)
# ==============================================================================
add_executable(test_multiplayer_session_and_handshake tests/test_multiplayer_session_and_handshake.cpp)
target_link_libraries(test_multiplayer_session_and_handshake PRIVATE opencraft_sim)
add_test(NAME MultiplayerSessionAndHandshakeTest COMMAND test_multiplayer_session_and_handshake)

add_executable(test_authoritative_actions_and_reach tests/test_authoritative_actions_and_reach.cpp)
target_link_libraries(test_authoritative_actions_and_reach PRIVATE opencraft_sim)
add_test(NAME AuthoritativeActionsAndReachTest COMMAND test_authoritative_actions_and_reach)

add_executable(test_snapshot_and_chunk_replication tests/test_snapshot_and_chunk_replication.cpp)
target_link_libraries(test_snapshot_and_chunk_replication PRIVATE opencraft_sim)
add_test(NAME SnapshotAndChunkReplicationTest COMMAND test_snapshot_and_chunk_replication)

add_executable(test_degraded_network_and_convergence tests/test_degraded_network_and_convergence.cpp)
target_link_libraries(test_degraded_network_and_convergence PRIVATE opencraft_sim)
add_test(NAME DegradedNetworkAndConvergenceTest COMMAND test_degraded_network_and_convergence)

# ==============================================================================
# Pruebas de Fase 10 (Biomas, Estructuras, Agricultura, Comercio y Navegación)
# ==============================================================================
add_executable(test_world_generation_biomes_and_100_seeds tests/test_world_generation_biomes_and_100_seeds.cpp)
target_link_libraries(test_world_generation_biomes_and_100_seeds PRIVATE opencraft_sim)
add_test(NAME WorldGenerationBiomesAnd100SeedsTest COMMAND test_world_generation_biomes_and_100_seeds)

add_executable(test_structures_and_multichunk_determinism tests/test_structures_and_multichunk_determinism.cpp)
target_link_libraries(test_structures_and_multichunk_determinism PRIVATE opencraft_sim)
add_test(NAME StructuresAndMultiChunkDeterminismTest COMMAND test_structures_and_multichunk_determinism)

add_executable(test_agriculture_growth_and_renewable_cycle tests/test_agriculture_growth_and_renewable_cycle.cpp)
target_link_libraries(test_agriculture_growth_and_renewable_cycle PRIVATE opencraft_sim)
add_test(NAME AgricultureGrowthAndRenewableCycleTest COMMAND test_agriculture_growth_and_renewable_cycle)

add_executable(test_villager_economy_and_concurrent_trade tests/test_villager_economy_and_concurrent_trade.cpp)
target_link_libraries(test_villager_economy_and_concurrent_trade PRIVATE opencraft_sim)
add_test(NAME VillagerEconomyAndConcurrentTradeTest COMMAND test_villager_economy_and_concurrent_trade)

# ==============================================================================
# Pruebas de Fase 11 (Automatización: Cinabrio, Pistones, Tolvas, Crafter y Rieles)
# ==============================================================================
add_executable(test_cinnabar_logic_and_timing tests/test_cinnabar_logic_and_timing.cpp)
target_link_libraries(test_cinnabar_logic_and_timing PRIVATE opencraft_sim)
add_test(NAME CinnabarLogicAndTimingTest COMMAND test_cinnabar_logic_and_timing)

add_executable(test_pistons_and_block_movement tests/test_pistons_and_block_movement.cpp)
target_link_libraries(test_pistons_and_block_movement PRIVATE opencraft_sim)
add_test(NAME PistonsAndBlockMovementTest COMMAND test_pistons_and_block_movement)

add_executable(test_hoppers_droppers_and_crafter tests/test_hoppers_droppers_and_crafter.cpp)
target_link_libraries(test_hoppers_droppers_and_crafter PRIVATE opencraft_sim)
add_test(NAME HoppersDroppersAndCrafterTest COMMAND test_hoppers_droppers_and_crafter)

add_executable(test_rails_and_minecart_transport tests/test_rails_and_minecart_transport.cpp)
target_link_libraries(test_rails_and_minecart_transport PRIVATE opencraft_sim)
add_test(NAME RailsAndMinecartTransportTest COMMAND test_rails_and_minecart_transport)

# ==============================================================================
# Pruebas de Fase 12 (Progresión Avanzada, Dimensiones, Jefes y Alquimia)
# ==============================================================================
add_executable(test_enchanting_anvil_and_progression_graph tests/test_enchanting_anvil_and_progression_graph.cpp)
target_link_libraries(test_enchanting_anvil_and_progression_graph PRIVATE opencraft_sim)
add_test(NAME EnchantingAnvilAndProgressionGraphTest COMMAND test_enchanting_anvil_and_progression_graph)

add_executable(test_alchemy_potions_and_effects tests/test_alchemy_potions_and_effects.cpp)
target_link_libraries(test_alchemy_potions_and_effects PRIVATE opencraft_sim)
add_test(NAME AlchemyPotionsAndEffectsTest COMMAND test_alchemy_potions_and_effects)

add_executable(test_dimensions_portals_and_storage tests/test_dimensions_portals_and_storage.cpp)
target_link_libraries(test_dimensions_portals_and_storage PRIVATE opencraft_sim)
add_test(NAME DimensionsPortalsAndStorageTest COMMAND test_dimensions_portals_and_storage)

add_executable(test_bosses_dragon_hydra_and_beacon tests/test_bosses_dragon_hydra_and_beacon.cpp)
target_link_libraries(test_bosses_dragon_hydra_and_beacon PRIVATE opencraft_sim)
add_test(NAME BossesDragonHydraAndBeaconTest COMMAND test_bosses_dragon_hydra_and_beacon)

# Fase 13: Modos completos, comandos y contenido extensible
add_executable(test_gamemodes_and_hardcore_persistence tests/test_gamemodes_and_hardcore_persistence.cpp)
target_link_libraries(test_gamemodes_and_hardcore_persistence PRIVATE opencraft_sim)
add_test(NAME GameModesAndHardcorePersistenceTest COMMAND test_gamemodes_and_hardcore_persistence)

add_executable(test_command_parser_and_selectors tests/test_command_parser_and_selectors.cpp)
target_link_libraries(test_command_parser_and_selectors PRIVATE opencraft_sim)
add_test(NAME CommandParserAndSelectorsTest COMMAND test_command_parser_and_selectors)

add_executable(test_command_blocks_and_function_budget tests/test_command_blocks_and_function_budget.cpp)
target_link_libraries(test_command_blocks_and_function_budget PRIVATE opencraft_sim)
add_test(NAME CommandBlocksAndFunctionBudgetTest COMMAND test_command_blocks_and_function_budget)

add_executable(test_content_packs_and_advancements tests/test_content_packs_and_advancements.cpp)
target_link_libraries(test_content_packs_and_advancements PRIVATE opencraft_sim)
add_test(NAME ContentPacksAndAdvancementsTest COMMAND test_content_packs_and_advancements)

# ==============================================================================
# Pruebas de Fase 14 (Experiencia Audiovisual, Accesibilidad y Plataformas)
# ==============================================================================
add_executable(test_device_settings_and_localization tests/test_device_settings_and_localization.cpp)
target_link_libraries(test_device_settings_and_localization PRIVATE opencraft_sim)
add_test(NAME DeviceSettingsAndLocalizationTest COMMAND test_device_settings_and_localization)

add_executable(test_touch_controller_and_safe_area tests/test_touch_controller_and_safe_area.cpp)
target_link_libraries(test_touch_controller_and_safe_area PRIVATE opencraft_sim)
add_test(NAME TouchControllerAndSafeAreaTest COMMAND test_touch_controller_and_safe_area)

add_executable(test_spatial_audio_and_subtitles tests/test_spatial_audio_and_subtitles.cpp)
target_link_libraries(test_spatial_audio_and_subtitles PRIVATE opencraft_sim)
add_test(NAME SpatialAudioAndSubtitlesTest COMMAND test_spatial_audio_and_subtitles)

add_executable(test_lifecycle_and_background_recovery tests/test_lifecycle_and_background_recovery.cpp)
target_link_libraries(test_lifecycle_and_background_recovery PRIVATE opencraft_sim)
add_test(NAME LifecycleAndBackgroundRecoveryTest COMMAND test_lifecycle_and_background_recovery)

add_executable(test_benchmark_and_quality_profiles tests/test_benchmark_and_quality_profiles.cpp)
target_link_libraries(test_benchmark_and_quality_profiles PRIVATE opencraft_sim)
add_test(NAME BenchmarkAndQualityProfilesTest COMMAND test_benchmark_and_quality_profiles)

# ==============================================================================
# Pruebas de Fase 15 (Catálogo Completo, Certificación y Lanzamiento)
# ==============================================================================
add_executable(test_trial_chambers_and_content_1_21 tests/test_trial_chambers_and_content_1_21.cpp)
target_link_libraries(test_trial_chambers_and_content_1_21 PRIVATE opencraft_sim)
add_test(NAME TrialChambersAndContent121Test COMMAND test_trial_chambers_and_content_1_21)

add_executable(test_trial_spawner_session_tick tests/test_trial_spawner_session_tick.cpp)
target_link_libraries(test_trial_spawner_session_tick PRIVATE opencraft_sim)
add_test(NAME TrialSpawnerSessionTickTest COMMAND test_trial_spawner_session_tick)

add_executable(test_content_hash_canonical tests/test_content_hash_canonical.cpp)
target_link_libraries(test_content_hash_canonical PRIVATE opencraft_sim)
add_test(NAME ContentHashCanonicalTest COMMAND test_content_hash_canonical)

add_executable(test_incremental_save_and_mob_chunks tests/test_incremental_save_and_mob_chunks.cpp)
target_link_libraries(test_incremental_save_and_mob_chunks PRIVATE opencraft_sim)
add_test(NAME IncrementalSaveAndMobChunksTest COMMAND test_incremental_save_and_mob_chunks)

add_executable(test_id_aliases tests/test_id_aliases.cpp)
target_link_libraries(test_id_aliases PRIVATE opencraft_sim)
add_test(NAME IdAliasesTest COMMAND test_id_aliases)

add_executable(test_mob_movement_steps_and_wander tests/test_mob_movement_steps_and_wander.cpp)
target_link_libraries(test_mob_movement_steps_and_wander PRIVATE opencraft_sim)
add_test(NAME MobMovementStepsAndWanderTest COMMAND test_mob_movement_steps_and_wander)

add_executable(test_atlas_meshing_and_text tests/test_atlas_meshing_and_text.cpp)
target_link_libraries(test_atlas_meshing_and_text PRIVATE opencraft_sim)
add_test(NAME AtlasMeshingAndTextTest COMMAND test_atlas_meshing_and_text)

add_executable(test_effects_input_and_commands tests/test_effects_input_and_commands.cpp)
target_link_libraries(test_effects_input_and_commands PRIVATE opencraft_sim)
add_test(NAME EffectsInputAndCommandsTest COMMAND test_effects_input_and_commands)

add_executable(test_session_inventory_and_containers tests/test_session_inventory_and_containers.cpp)
target_link_libraries(test_session_inventory_and_containers PRIVATE opencraft_sim)
add_test(NAME SessionInventoryAndContainersTest COMMAND test_session_inventory_and_containers)

add_executable(test_hoppers_and_machines tests/test_hoppers_and_machines.cpp)
target_link_libraries(test_hoppers_and_machines PRIVATE opencraft_sim)
add_test(NAME HoppersAndMachinesTest COMMAND test_hoppers_and_machines)

# Prueba de humo del servidor dedicado: crea un mundo, simula 2 s, guarda y sale
add_test(NAME DedicatedServerSmokeTest
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR}/server_smoke
            $<TARGET_FILE:opencraft_server> --port 35991 --world ci --ticks 40)
set_tests_properties(DedicatedServerSmokeTest PROPERTIES
    FIXTURES_REQUIRED server_smoke_dir
    PASS_REGULAR_EXPRESSION "cerrado correctamente \\(40 ticks\\)")
add_test(NAME DedicatedServerSmokeDir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/server_smoke)
set_tests_properties(DedicatedServerSmokeDir PROPERTIES FIXTURES_SETUP server_smoke_dir)
add_test(NAME DedicatedServerHelp COMMAND opencraft_server --help)
set_tests_properties(DedicatedServerHelp PROPERTIES PASS_REGULAR_EXPRESSION "--ticks")

add_executable(test_cross_system_regression tests/test_cross_system_regression.cpp)
target_link_libraries(test_cross_system_regression PRIVATE opencraft_sim)
add_test(NAME CrossSystemRegressionTest COMMAND test_cross_system_regression)

add_executable(test_server_stress_and_world_migration tests/test_server_stress_and_world_migration.cpp)
target_link_libraries(test_server_stress_and_world_migration PRIVATE opencraft_sim)
add_test(NAME ServerStressAndWorldMigrationTest COMMAND test_server_stress_and_world_migration)
endif()
