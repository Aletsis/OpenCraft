# Dossier de Evidencias: Fase 08 — Criaturas, IA y Combate (H3: Primera Noche)

## 1. Resumen Ejecutivo
La **Fase 08** culmina el hito fundamental **H3: Primera Noche** de paridad con el juego de referencia canónico al dotar a OpenCraft de un subsistema completo de criaturas (`MobEntity`), Inteligencia Artificial por máquina de estados (`AIState`) acoplada a navegación A* tridimensional con presupuesto estricto de exploración (128 nodos), generador y despawn periódico de criaturas (`MobSpawner`) regido por topes de población (70 monstruos, 10 pasivos) y niveles de luz solar/bloque, y un sistema físico de combate balanceado (armaduras con mitigación cuadrática canónica, absorción al 100% con escudo frontal, retroceso/knockback direccional, proyectiles balísticos con gravedad y arrastre `ProjectileEntity`, y orbes de experiencia gravitacionales con absorción magnética `ExperienceOrbEntity`).

Asimismo, se eleva la persistencia del mundo al **Esquema v7** (`SCHEMA_VERSION_7 = 7`), preservando fielmente los atributos de las criaturas en cada chunk, el progreso de experiencia (`xpTotal`, `xpLevel`, `xpProgress`) y las ranuras de armadura del jugador, con retrocompatibilidad intacta para mundos creados en versiones v1 a v6.

---

## 2. Requisitos y Criterios de Aceptación Verificados

| Componente | Criterio de paridad con el juego de referencia | Estado | Verificación |
|---|---|---|---|
| **Arquetipos de Criatura** | Cuatro arquetipos canónicos: **Vaca** (pasiva, pánico/flee ante daño), **Zombie** (hostil melee, combustión espontánea diurna bajo luz solar), **Esqueleto** (hostil de rango con ametrallamiento/disparo de flechas a distancia), y **Acechador** (emboscada sigilosa, mecha de 30 ticks con siseo y detonación con destrucción destructiva de bloques/griefing). | ✅ Cumplido | `MobArchetypesAndPersistenceTest` |
| **Pathfinding A* y Navegación** | A* tridimensional con presupuesto estricto de 128 expansiones de nodos por búsqueda (evitando freezing o sobrecarga de CPU en situaciones sin salida), evitación de obstáculos geométricos, escalada asistida de 1 bloque e invalidación reactiva de rutas ante modificaciones en el terreno (`notifyTerrainChange`). | ✅ Cumplido | `PathfindingAndNavigationTest` |
| **Generación y Despawn (`MobSpawner`)** | Reglas de aparición: monstruos hostiles requieren nivel de luz efectivo <= 7 y dificultad > 0; criaturas pasivas requieren bloques de césped y nivel de luz >= 9. Topes de población estrictos (Mob Caps: 70 monstruos, 10 pasivas). Despawn instantáneo a distancias superiores a 128 bloques del jugador. | ✅ Cumplido | `MobSpawningAndLightRulesTest` |
| **Mecánicas de Combate y Armaduras** | Fórmula canónica de absorción de daño por armadura: `DañoEfectivo = Daño * (1.0 - puntosArmadura * 0.04)`. Bloqueo frontal activo de escudo con 100% de absorción de daño. Ventana de invulnerabilidad post-daño (`hurtTime` de 10 ticks = 0.5s). Retroceso/knockback físico direccional. | ✅ Cumplido | `CombatDamageAndLootTransactionTest` |
| **Proyectiles y Balística** | `ProjectileEntity` (flechas) con substepping, gravedad parabólica (-14.0 m/s²), fricción atmosférica, cálculo automático de orientación (yaw/pitch) y colisión AABB contra criaturas y jugador. | ✅ Cumplido | `CombatDamageAndLootTransactionTest`, `MobArchetypesAndPersistenceTest` |
| **Botín Atómico y Experiencia** | Dropeo determinista y atómico al morir la criatura (carne podrida, huesos, flechas, pólvora, cuero). Spawneo de `ExperienceOrbEntity` con atracción magnética suave (rango de 6.0 bloques) y absorción a <= 1.5 metros, alimentando la progresión de nivel y experiencia del jugador. | ✅ Cumplido | `CombatDamageAndLootTransactionTest` |
| **Presupuesto de Rendimiento** | Simulación simultánea de 100 entidades activas con IA, física de colisión y ticking dentro de un presupuesto holgado (< 50 ms/tick a 20 Hz fija). | ✅ Cumplido | `MobArchetypesAndPersistenceTest` |
| **Persistencia Esquema v7** | Serialización binaria segura de mobs por chunk (`mobs.dat`), armaduras de jugador y contadores de XP en `player.dat`. Carga determinista sin duplicación ni pérdida de entidades ni objetos. | ✅ Cumplido | `MobArchetypesAndPersistenceTest` |

---

## 3. Resultados de Pruebas Automatizadas (CTest)

Total de suites de prueba ejecutadas: **36/36 pasando limpiamente (100% de éxito)**.

```
Test project <repo>/build
      Start  1: SimulationDeterminismTest
 1/36 Test  #1: SimulationDeterminismTest ..............   Passed    0.08 sec
      Start  2: BlockRegistryTest
 2/36 Test  #2: BlockRegistryTest ......................   Passed    0.00 sec
      Start  3: NetworkPacketSecurityTest
 3/36 Test  #3: NetworkPacketSecurityTest ..............   Passed    0.00 sec
      Start  4: GameCommandsAndChunkBordersTest
 4/36 Test  #4: GameCommandsAndChunkBordersTest ........   Passed    0.07 sec
      Start  5: PersistenceDeterminismTest
 5/36 Test  #5: PersistenceDeterminismTest .............   Passed    0.20 sec
      Start  6: TransactionRecoveryTest
 6/36 Test  #6: TransactionRecoveryTest ................   Passed    0.01 sec
      Start  7: CorruptionAndIntegrityTest
 7/36 Test  #7: CorruptionAndIntegrityTest .............   Passed    0.03 sec
      Start  8: SchemaMigrationAndBackupTest
 8/36 Test  #8: SchemaMigrationAndBackupTest ...........   Passed    0.01 sec
      Start  9: WorldManagerAndLockTest
 9/36 Test  #9: WorldManagerAndLockTest ................   Passed    0.01 sec
      Start 10: GeneratorDeterminismTest
10/36 Test #10: GeneratorDeterminismTest ...............   Passed    0.03 sec
      Start 11: StreamingCacheAndMemoryTest
11/36 Test #11: StreamingCacheAndMemoryTest ............   Passed    0.14 sec
      Start 12: StreamingPersistenceTest
12/36 Test #12: StreamingPersistenceTest ...............   Passed    0.07 sec
      Start 13: ConcurrencyAndBoundariesTest
13/36 Test #13: ConcurrencyAndBoundariesTest ...........   Passed    0.00 sec
      Start 14: FrustumCullingTest
14/36 Test #14: FrustumCullingTest .....................   Passed    0.00 sec
      Start 15: InventoryConservationTest
15/36 Test #15: InventoryConservationTest ..............   Passed    0.00 sec
      Start 16: ItemEntityPickupTest
16/36 Test #16: ItemEntityPickupTest ...................   Passed    0.08 sec
      Start 17: ChestTransactionsTest
17/36 Test #17: ChestTransactionsTest ..................   Passed    0.03 sec
      Start 18: InventoryPersistenceTest
18/36 Test #18: InventoryPersistenceTest ...............   Passed    0.07 sec
      Start 19: ProductionChainTest
19/36 Test #19: ProductionChainTest ....................   Passed    0.03 sec
      Start 20: RecipeCapacityAndBatchTest
20/36 Test #20: RecipeCapacityAndBatchTest .............   Passed    0.03 sec
      Start 21: FurnaceSimulationPersistenceTest
21/36 Test #21: FurnaceSimulationPersistenceTest .......   Passed    0.07 sec
      Start 22: ToolDurabilityAndMiningTest
22/36 Test #22: ToolDurabilityAndMiningTest ............   Passed    0.03 sec
      Start 23: SurvivalStatsTest
23/36 Test #23: SurvivalStatsTest ......................   Passed    0.03 sec
      Start 24: DamageAndHazardsTest
24/36 Test #24: DamageAndHazardsTest ...................   Passed    0.03 sec
      Start 25: MovementAndCollisionTest
25/36 Test #25: MovementAndCollisionTest ...............   Passed    0.03 sec
      Start 26: DeathAndRespawnTest
26/36 Test #26: DeathAndRespawnTest ....................   Passed    0.05 sec
      Start 27: BedAndGameModesTest
27/36 Test #27: BedAndGameModesTest ....................   Passed    0.03 sec
      Start 28: LightingAndPropagationTest
28/36 Test #28: LightingAndPropagationTest .............   Passed    0.01 sec
      Start 29: FluidFlowAndInteractionTest
29/36 Test #29: FluidFlowAndInteractionTest ............   Passed    0.01 sec
      Start 30: ImmersionAndEnvironmentalHazardsTest
30/36 Test #30: ImmersionAndEnvironmentalHazardsTest ...   Passed    0.03 sec
      Start 31: ScheduledUpdatesAndWeatherTest
31/36 Test #31: ScheduledUpdatesAndWeatherTest .........   Passed    0.03 sec
      Start 32: RenderPassesAndAoTest
32/36 Test #32: RenderPassesAndAoTest ..................   Passed    0.00 sec
      Start 33: PathfindingAndNavigationTest
33/36 Test #33: PathfindingAndNavigationTest ...........   Passed    0.03 sec
      Start 34: MobSpawningAndLightRulesTest
34/36 Test #34: MobSpawningAndLightRulesTest ...........   Passed    0.04 sec
      Start 35: CombatDamageAndLootTransactionTest
35/36 Test #35: CombatDamageAndLootTransactionTest .....   Passed    0.03 sec
      Start 36: MobArchetypesAndPersistenceTest
36/36 Test #36: MobArchetypesAndPersistenceTest ........   Passed    0.10 sec

100% tests passed out of 36
Total Test time (real) = 1.47 sec
```

---

## 4. Verificación de la Puerta de Salida (Exit Criteria)

- [x] **Todas las pruebas específicas de F08 pasan**: 4 nuevas suites de prueba (`test_pathfinding_and_navigation`, `test_mob_spawning_and_light`, `test_combat_and_loot_transactions`, `test_mob_archetypes_and_persistence`) superadas al 100%.
- [x] **Regresión limpia de fases anteriores (F01 a F07)**: Los 32 tests anteriores continúan pasando sin ninguna alteración en su comportamiento determinista.
- [x] **Guardado y persistencia verificados**: Esquema v7 almacena y recupera criaturas por chunk, slots de armaduras y estadísticas completas de experiencia.
- [x] **Cero duplicación de botín / transacciones atómicas**: Transición de muerte procesada con flags atómicos de dropeo único.
- [x] **Hito H3 (Primera Noche) completado**: La cadena de supervivencia, refugio iluminado, aparición nocturna, combate y recuperación sin duplicación se encuentra completamente funcional.
