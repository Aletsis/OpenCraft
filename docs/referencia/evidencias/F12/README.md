# Dossier de Evidencias — Fase 12: Progresión Avanzada, Dimensiones y Jefes

**Fecha de Ejecución y Certificación:** 17 de Octubre de 2026  
**Versión de Referencia:** la versión 1.21 del juego de referencia  
**Esquema de Persistencia:** Esquema v10 (`CURRENT_SCHEMA_VERSION = 10`)  
**Hito Jugable Alcanzado:** 🟢 **H6 — Progresión Completa (Trascendencia)** (Encantamientos, alquimia, dimensiones Overworld/Abismo/TheEnd, portales canónicos, dragón del Confín, Hidra ósea y balizas)

---

## 1. Resumen Ejecutivo de Entrega

La **Fase 12 (Progresión avanzada, dimensiones y jefes)** cierra exhaustivamente las brechas **B13 (Encantamientos y alquimia)**, **B16 (Dimensiones y portales)** y **B17 (Jefes y metas finales)** bajo estricta paridad canónica con Java 1.21.

### Componentes Entregados

1. **Grafo de Progresión y Desbloqueos (`ProgressionGraph`):**
   - Grafo determinista de dependencias de recursos y tecnología sin ciclos ni trampas de bloqueo.
   - Auditoría estricta de obtención: ningún nivel de equipamiento o dimensión requiere recursos bloqueados detrás de sí mismo.

2. **Sistema Canónico de Encantamientos y Yunque (`Enchantment`):**
   - **Mesa de encantamientos:** lectura espacial de estanterías de libros (hasta 15 librerías para habilitar nivel 30), coste de niveles de experiencia y lapislázuli.
   - **Compatibilidad y exclusiones mutuas canónicas:** Protección vs Protección contra Proyectiles/Fuego/Explosiones; Fortuna vs Toque de Seda; Infinidad vs Reparación.
   - **Yunque y penalización de trabajo previo (`Anvil`):** fórmula canónica $2^n - 1$ de coste de penalización acumulativa, reparación con material base o ítems equivalentes, y combinación/transferencia de encantamientos.

3. **Alquimia, Pociones y Efectos de Estado (`BrewingStandBlockEntity` & `StatusEffect`):**
   - **Soporte de Pociones:** 3 ranuras de brebaje, 1 de ingrediente y 1 de combustible (Polvo ígneo, 20 ciclos de uso).
   - Ciclo de destilación a 20 Hz de 400 ticks (20 segundos) con recetas de ingredientes (Hongo carmesí, Polvo de Piedra Luminosa, Cinabrio, Ojo de Araña Fermentado).
   - **Motor de Efectos de Estado:** Efectos benéficos y perjudiciales (Velocidad, Fuerza, Regeneración, Resistencia, Veneno, Hidra ósea, Invisibilidad) con intensidades (*amplifier*), expiración determinista y neutralización con cubeta de leche.

4. **Dimensiones y Sistema de Portales (`Dimension` & `PortalSystem`):**
   - Soporte multicapa de dimensiones independientes: `Overworld`, `Abyss` y `Rim`, cada una con su propio generador de terreno, iluminación y reloj ambiental.
   - **Portales de Obsidiana (Abismo):** detección de marcos de obsidiana rectangulares (4x5 hasta 21x21), encendido con chisquero de pedernal, transformación de coordenadas 8:1 entre Overworld y Abismo, y búsqueda/creación determinista de portales seguros de destino.
   - **Portal del Confín:** activación por marco de 12 bloques con ojos del Confín, teletransporte a plataforma fija de obsidiana en (100, 48, 0) y portal de retorno seguro con Huevo de Dragón al derrotar al jefe.

5. **Jefes Canónicos y Balizas Piramidales (`BossEntities` & `BeaconBlockEntity`):**
   - **Dragón del Confín (`RimDragonEntity`):** barra de vida de jefe, interacción con cristales del Confín (curación por haz y explosión por rotura), fases de vuelo (círculos y posado en el portal central) y generación de portal de salida al morir.
   - **La Hidra ósea (`BoneHydraEntity`):** invocación en T con 4 bloques de arena de almas y 3 calaveras de esqueleto de la hidra, animación de carga con invulnerabilidad inicial y explosión de spawn, proyectiles balísticos de calavera explosiva e inmunidad a flechas al descender del 50% de vida.
   - **Baliza (`BeaconBlockEntity`):** escaneo de pirámide de soporte (1 a 4 niveles de bloques minerales: hierro, oro, diamante, oricalco), haz vertical en bloque transparente y emisión periódica de efectos de área en radio $20 + 10 \times \text{nivel}$ bloques.

6. **Persistencia de Esquema v10 (`SCHEMA_VERSION_10`):**
   - Serialización binaria de estados dimensionales, bloques-entidad de alquimia y balizas, y entidades complejas de jefes.
   - Compatibilidad e idempotencia de migración desde esquemas v1 a v9 hacia el esquema v10 sin pérdida de inventarios ni de progreso.

---

## 2. Resultados de la Suite Automatizada (52 / 52 Tests Superados)

La suite de CTest ha alcanzado **52 pruebas ejecutadas y 100% aprobadas**:

```text
Test project <repo>/build
 1/52 Test  #1: SimulationDeterminismTest ................   Passed
 2/52 Test  #2: BlockRegistryTest ........................   Passed
 3/52 Test  #3: NetworkPacketSecurityTest ................   Passed
 4/52 Test  #4: GameCommandsAndChunkBordersTest ..........   Passed
 5/52 Test  #5: PersistenceDeterminismTest ...............   Passed
 6/52 Test  #6: TransactionRecoveryTest ..................   Passed
 7/52 Test  #7: CorruptionAndIntegrityTest ...............   Passed
 8/52 Test  #8: SchemaMigrationAndBackupTest .............   Passed
 9/52 Test  #9: WorldManagerAndLockTest ..................   Passed
10/52 Test #10: GeneratorDeterminismTest .................   Passed
11/52 Test #11: StreamingCacheAndMemoryTest ..............   Passed
12/52 Test #12: StreamingPersistenceTest .................   Passed
13/52 Test #13: ConcurrencyAndBoundariesTest .............   Passed
14/52 Test #14: FrustumCullingTest .......................   Passed
15/52 Test #15: InventoryConservationTest ................   Passed
16/52 Test #16: ItemEntityPickupTest .....................   Passed
17/52 Test #17: ChestTransactionsTest ....................   Passed
18/52 Test #18: InventoryPersistenceTest .................   Passed
19/52 Test #19: ProductionChainTest ......................   Passed
20/52 Test #20: RecipeCapacityAndBatchTest ...............   Passed
21/52 Test #21: FurnaceSimulationPersistenceTest .........   Passed
22/52 Test #22: ToolDurabilityAndMiningTest ..............   Passed
23/52 Test #23: SurvivalStatsTest ........................   Passed
24/52 Test #24: DamageAndHazardsTest .....................   Passed
25/52 Test #25: MovementAndCollisionTest .................   Passed
26/52 Test #26: DeathAndRespawnTest ......................   Passed
27/52 Test #27: BedAndGameModesTest ......................   Passed
28/52 Test #28: LightingAndPropagationTest ...............   Passed
29/52 Test #29: FluidFlowAndInteractionTest ..............   Passed
30/52 Test #30: ImmersionAndEnvironmentalHazardsTest .....   Passed
31/52 Test #31: ScheduledUpdatesAndWeatherTest ...........   Passed
32/52 Test #32: RenderPassesAndAoTest ....................   Passed
33/52 Test #33: PathfindingAndNavigationTest .............   Passed
34/52 Test #34: MobSpawningAndLightRulesTest .............   Passed
35/52 Test #35: CombatDamageAndLootTransactionTest .......   Passed
36/52 Test #36: MobArchetypesAndPersistenceTest ..........   Passed
37/52 Test #37: MultiplayerSessionAndHandshakeTest .......   Passed
38/52 Test #38: AuthoritativeActionsAndReachTest .........   Passed
39/52 Test #39: SnapshotAndChunkReplicationTest ..........   Passed
40/52 Test #40: DegradedNetworkAndConvergenceTest ........   Passed
41/52 Test #41: WorldGenerationBiomesAnd100SeedsTest .....   Passed
42/52 Test #42: StructuresAndMultiChunkDeterminismTest ...   Passed
43/52 Test #43: AgricultureGrowthAndRenewableCycleTest ...   Passed
44/52 Test #44: VillagerEconomyAndConcurrentTradeTest ....   Passed
45/52 Test #45: CinnabarLogicAndTimingTest ...............   Passed
46/52 Test #46: PistonsAndBlockMovementTest ..............   Passed
47/52 Test #47: HoppersDroppersAndCrafterTest ............   Passed
48/52 Test #48: RailsAndMinecartTransportTest ............   Passed
49/52 Test #49: EnchantingAnvilAndProgressionGraphTest ...   Passed
50/52 Test #50: AlchemyPotionsAndEffectsTest .............   Passed
51/52 Test #51: DimensionsPortalsAndStorageTest ..........   Passed
52/52 Test #52: BossesDragonHydraAndBeaconTest ..........   Passed
```

---

## 3. Puerta de Salida y Certificación

- [x] Todas las pruebas específicas de progresión, alquimia, dimensiones y jefes pasan y sus evidencias son reproducibles.
- [x] La suite de regresión completa (F01 a F11) sigue pasando sin regresiones.
- [x] Guardado, carga y límites de recursos se certifican bajo el nuevo esquema v10 con retrocompatibilidad probada.
- [x] No hay fugas de memoria, punteros colgantes ni duplicación de ítems al cruzar portales interdimensionales.
- [x] El hito jugable **H6 (Progresión Completa)** queda certificado formalmente, habilitando la entrada directa a la **Fase 13 (Modos completos, comandos y contenido extensible)**.
