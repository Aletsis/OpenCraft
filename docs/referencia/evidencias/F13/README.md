# Dossier de Evidencias — Fase 13: Modos Completos, Comandos y Contenido Extensible

**Fecha de Ejecución y Certificación:** 17 de Octubre de 2026  
**Versión de Referencia:** la versión 1.21 del juego de referencia  
**Esquema de Persistencia:** Esquema v11 (`CURRENT_SCHEMA_VERSION = 11`)  
**Hito Jugable Alcanzado:** 🟢 **H7 — Modos y Creadores** (Modos canónicos 0..3, Espectador noclip/vuelo/invulnerable, Hardcore permadeath permanente, motor de comandos con selectores y cap de seguridad, bloques de comandos y .mcfunction con cuota de tick y recursión, paquetes de contenido deterministas, logros y estadísticas)

---

## 1. Resumen Ejecutivo de Entrega

La **Fase 13 (Modos y Creadores)** cierra de manera integral y exhaustiva las brechas **B14 (Modos de juego y reglas)**, **B18 (Consola, comandos y bloques de comandos)** y **B19 (Datapacks, funciones y contenido extensible)** en paridad estricta con la versión 1.21 del juego de referencia.

### Componentes Entregados

1. **Modos Canónicos de Juego y Modo Hardcore (`Entity.hpp`, `WorldSession`):**
   - **Supervivencia (0):** Mecánicas completas de metabolismo, hambre, fatiga, daño ambiental y regeneración.
   - **Creativo (1):** Vuelo libre, invulnerabilidad general (excepto vacío), colocación y minado instantáneo de bloques sin consumo de inventario.
   - **Aventura (2):** Restricciones de interacción (imposibilidad de colocar o romper bloques arbitrarios sin etiquetas/herramientas canónicas de minado).
   - **Espectador (3):** Capacidad de noclip completa a través de bloques sólidos (`resolveCollisions`), vuelo permanente tridimensional, invulnerabilidad total contra todo daño (incluyendo vacío y fuego), e incapacidad de interactuar o golpear entidades.
   - **Modo Hardcore (`isHardcore`):** Permadeath estricto; al morir el jugador, la transición a Modo Espectador (`gameMode = 3`) es permanente e irreversible tanto en sesión como al persistir y recargar en disco.

2. **Motor Autoritativo de Comandos y Selectores (`CommandEngine`):**
   - **Intérprete determinista:** Soporte de sintaxis con parámetros, subcomandos y control jerárquico de permisos (0: usuario, 2: operador/moderador, 4: administrador/bloque de comando).
   - **Selectores canónicos de objetivo:** `@p` (jugador más cercano), `@a` (todos los jugadores), `@e` (todas las entidades), `@s` (entidad ejecutora).
   - **Coordenadas relativas (`~`):** Desplazamiento relativo respecto a la posición del ejecutor (`~`, `~5`, `~-2.5`).
   - **Catálogo de comandos implementados:**
     - `/gamemode <survival|creative|adventure|spectator>`
     - `/time <set|add|query> <day|noon|night|midnight|número>`
     - `/weather <clear|rain|thunder> [duración]`
     - `/tp [objetivo] <x> <y> <z>`
     - `/give <jugador> <ítem> [cantidad]`
     - `/summon <entidad> <x> <y> <z>`
     - `/setblock <x> <y> <z> <bloque>`
     - `/fill <x1> <y1> <z1> <x2> <y2> <z2> <bloque>` con **límite de seguridad estricto $\le 32768$ bloques** (rechazo inmediato ante volúmenes mayores).
     - `/gamerule <regla> [valor]` (`keepInventory`, `doDaylightCycle`, `mobGriefing`, `doMobSpawning`, `commandBlockOutput`, `naturalRegeneration`, `fallDamage`, `fireDamage`, `drowningDamage`, `pvp`).
     - `/effect <give|clear> <jugador> [efecto] [duración] [amplificador]`
     - `/function <nombre_función>`

3. **Bloques de Comandos y Ejecutor de Funciones (`CommandBlockEntity`, `FunctionRunner`):**
   - **Bloques de comandos:** Variantes Impulse, Repeating y Chain; modos condicional/incondicional; activación por cinabrio o AlwaysActive; ejecución periódica o en flanco de subida de energía a 20 Hz.
   - **Ejecución de scripts `.mcfunction`:** Ejecución secuencial de comandos línea a línea.
   - **Límite de profundidad de recursión:** Tope estricto de **16 niveles** para prevenir bucles de recursión infinita.
   - **Presupuesto de instrucciones por tick:** Cuota máxima de **10,000 comandos por tick**, abortando con seguridad cualquier macro descontrolada o congelamiento del hilo principal de simulación.

4. **Gestor de Paquetes de Contenido / Datapacks (`ContentPackManager`):**
   - Manifiestos de metadatos (`id`, `name`, `version`, `description`, `packFormat`, `dependencies`).
   - Validación estricta de referencias rotas contra el catálogo global de bloques e ítems.
   - Hash determinista de sincronización de red (`computeNetworkHash` / `verifyNetworkHash`) para validar paridad estricta entre cliente y servidor e impedir divergencias de sincronización.
   - Carga dinámica de recetas (Shaped/Shapeless), logros y funciones.

5. **Sistema de Logros y Estadísticas Canónicas (`AdvancementManager`, `StatisticsManager`):**
   - Grafo jerárquico de logros canónicos (árbol de historia, Abismo, End y aventura).
   - Disparadores por eventos de juego (`onPlayerJoin`, `onBlockMined`, `onItemCrafted`, `onDimensionChanged`, `onMobKilled`).
   - Seguimiento determinista de métricas de usuario (`mined`, `crafted`, `used`, `general`, `deaths`).

6. **Persistencia Esquema v11 (`WorldFormat`):**
   - Serialización y deserialización de estado Hardcore (`isHardcore`), colección de reglas de juego dinámicas (`gameRules`), colección de logros desbloqueados (`unlockedAdvancements`) y estadísticas generales (`generalStatistics`).
   - Idempotencia total y compatibilidad hacia atrás completa con mundos de esquemas v1 a v10.

---

## 2. Resultados de la Suite Automatizada (56 / 56 Tests Superados)

La suite de CTest ha alcanzado **56 pruebas ejecutadas y 100% aprobadas** con cero regresiones:

```text
Test project <repo>/build
 1/56 Test  #1: SimulationDeterminismTest ................   Passed    0.96 sec
 2/56 Test  #2: BlockRegistryTest ........................   Passed    0.01 sec
 3/56 Test  #3: NetworkPacketSecurityTest ................   Passed    0.00 sec
 4/56 Test  #4: GameCommandsAndChunkBordersTest ..........   Passed    0.75 sec
 5/56 Test  #5: PersistenceDeterminismTest ...............   Passed    2.02 sec
 6/56 Test  #6: TransactionRecoveryTest ..................   Passed    0.02 sec
 7/56 Test  #7: CorruptionAndIntegrityTest ...............   Passed    0.36 sec
 8/56 Test  #8: SchemaMigrationAndBackupTest .............   Passed    0.06 sec
 9/56 Test  #9: WorldManagerAndLockTest ..................   Passed    0.01 sec
10/56 Test #10: GeneratorDeterminismTest .................   Passed    1.12 sec
11/56 Test #11: StreamingCacheAndMemoryTest ..............   Passed    1.55 sec
12/56 Test #12: StreamingPersistenceTest .................   Passed    0.45 sec
13/56 Test #13: ConcurrencyAndBoundariesTest .............   Passed    0.02 sec
14/56 Test #14: FrustumCullingTest .......................   Passed    0.00 sec
15/56 Test #15: InventoryConservationTest ................   Passed    0.01 sec
16/56 Test #16: ItemEntityPickupTest .....................   Passed    0.66 sec
17/56 Test #17: ChestTransactionsTest ....................   Passed    0.25 sec
18/56 Test #18: InventoryPersistenceTest .................   Passed    0.64 sec
19/56 Test #19: ProductionChainTest ......................   Passed    0.26 sec
20/56 Test #20: RecipeCapacityAndBatchTest ...............   Passed    0.24 sec
21/56 Test #21: FurnaceSimulationPersistenceTest .........   Passed    0.67 sec
22/56 Test #22: ToolDurabilityAndMiningTest ..............   Passed    0.29 sec
23/56 Test #23: SurvivalStatsTest ........................   Passed    0.26 sec
24/56 Test #24: DamageAndHazardsTest .....................   Passed    0.32 sec
25/56 Test #25: MovementAndCollisionTest .................   Passed    0.27 sec
26/56 Test #26: DeathAndRespawnTest ......................   Passed    0.64 sec
27/56 Test #27: BedAndGameModesTest ......................   Passed    0.25 sec
28/56 Test #28: LightingAndPropagationTest ...............   Passed    0.06 sec
29/56 Test #29: FluidFlowAndInteractionTest ..............   Passed    0.08 sec
30/56 Test #30: ImmersionAndEnvironmentalHazardsTest .....   Passed    0.23 sec
31/56 Test #31: ScheduledUpdatesAndWeatherTest ...........   Passed    0.24 sec
32/56 Test #32: RenderPassesAndAoTest ....................   Passed    0.00 sec
33/56 Test #33: PathfindingAndNavigationTest .............   Passed    0.27 sec
34/56 Test #34: MobSpawningAndLightRulesTest .............   Passed    3.06 sec
35/56 Test #35: CombatDamageAndLootTransactionTest .......   Passed    0.37 sec
36/56 Test #36: MobArchetypesAndPersistenceTest ..........   Passed    1.53 sec
37/56 Test #37: MultiplayerSessionAndHandshakeTest .......   Passed    0.77 sec
38/56 Test #38: AuthoritativeActionsAndReachTest .........   Passed    0.84 sec
39/56 Test #39: SnapshotAndChunkReplicationTest ..........   Passed    0.86 sec
40/56 Test #40: DegradedNetworkAndConvergenceTest ........   Passed    1.46 sec
41/56 Test #41: WorldGenerationBiomesAnd100SeedsTest .....   Passed   92.84 sec
42/56 Test #42: StructuresAndMultiChunkDeterminismTest ...   Passed    0.45 sec
43/56 Test #43: AgricultureGrowthAndRenewableCycleTest ...   Passed    0.27 sec
44/56 Test #44: VillagerEconomyAndConcurrentTradeTest ....   Passed    0.32 sec
45/56 Test #45: CinnabarLogicAndTimingTest ...............   Passed    0.05 sec
46/56 Test #46: PistonsAndBlockMovementTest ..............   Passed    0.01 sec
47/56 Test #47: HoppersDroppersAndCrafterTest ............   Passed    0.01 sec
48/56 Test #48: RailsAndMinecartTransportTest ............   Passed    0.00 sec
49/56 Test #49: EnchantingAnvilAndProgressionGraphTest ...   Passed    0.02 sec
50/56 Test #50: AlchemyPotionsAndEffectsTest .............   Passed    0.00 sec
51/56 Test #51: DimensionsPortalsAndStorageTest ..........   Passed    0.04 sec
52/56 Test #52: BossesDragonHydraAndBeaconTest ..........   Passed    0.00 sec
53/56 Test #53: GameModesAndHardcorePersistenceTest ......   Passed    0.34 sec
54/56 Test #54: CommandParserAndSelectorsTest ............   Passed    0.01 sec
55/56 Test #55: CommandBlocksAndFunctionBudgetTest .......   Passed    0.04 sec
56/56 Test #56: ContentPacksAndAdvancementsTest ..........   Passed    0.00 sec

100% tests passed out of 56
Total Test time (real) = 116.29 sec
```

---

## 3. Matriz de Cobertura de las Nuevas Pruebas

| Test ID | Binario de Prueba | Brechas Cubiertas | Validaciones Clave |
|---|---|---|---|
| **#53** | `test_gamemodes_and_hardcore_persistence` | **B14** | Modos 0..3, restricciones de aventura, espectador noclip atravesando bloques sólidos, espectador invulnerable a vacío/fuego/caída, permadeath hardcore con bloqueo permanente e irreversible en espectador, persistencia y recarga de Esquema v11. |
| **#54** | `test_command_parser_and_selectors` | **B18** | Intérprete y permisos, selectores `@p`, `@a`, `@s`, `@e`, coordenadas relativas `~`, comandos `/gamemode`, `/time`, `/weather`, `/tp`, `/give`, `/summon`, `/setblock`, `/gamerule`, cap estricto de seguridad en `/fill` ($\le 32768$). |
| **#55** | `test_command_blocks_and_function_budget` | **B18**, **B19** | Bloque de comandos Impulse activado por cinabrio contiguo, Repeating AlwaysActive a 20 Hz, funciones `.mcfunction`, límite de recursión de 16 niveles y presupuesto de 10,000 instrucciones/tick con reinicio por tick. |
| **#56** | `test_content_packs_and_advancements` | **B19** | Carga de datapacks, rechazo de referencias rotas, hash determinista de sincronización de red, árbol jerárquico de logros y activación por triggers, persistencia de estadísticas de jugador (`mined`, `crafted`, `general`, `deaths`). |

---

## 4. Estado de Certificación

- **Estado de Fase 13:** 🟢 **Completada y Certificada**
- **Regresiones Detectadas:** 0
- **Tasa de Aprobación Global:** **100% (56/56 suites)**
- **Progreso del Plan Maestro:** 13 de 14 fases completadas (93% del roadmap integral de paridad con la versión 1.21 del juego de referencia).
