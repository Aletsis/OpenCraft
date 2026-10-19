# Dossier de Evidencias: Fase 10 — Exploración, Ecosistemas y Biomas (H5: Madurez)

## 1. Resumen Ejecutivo
La **Fase 10** inaugura el hito **H5: Madurez** de la hoja de ruta de paridad con el juego de referencia canónico al expandir drásticamente el mundo de OpenCraft en cuatro pilares fundamentales:
1. **Generador de Mundo v3 con Biomas y Cuevas 3D (Brecha B11)**: Selección continua de 6 biomas canónicos (Plains, Forest, Desert, Snowy Tundra, Mountains, Ocean) basada en campos de temperatura, humedad, continentalidad y rugosidad; densidad 3D para cuevas ramificadas con acuíferos a nivel freático ($y \le 62$) y distribución vertical canónica de menas (carbón, hierro, oro, diamante, esmeralda exclusiva en bioma de montaña).
2. **Estructuras Multi-Chunk Deterministas (Brecha B11)**: Generación por cuadrícula de regiones con anclajes deterministas independientes del orden de carga de chunks (Aldeas con casas y pozos, Mazmorras subterráneas con spawners de monstruos y cofres de botín, Ruinas en desierto/océano). Generación sin duplicación de cofres y preservación tras recarga en `WorldStorage` (Schema v8).
3. **Agricultura, Hidratación y Cadena Renovable (Brecha B12)**: Terreno de cultivo (`Farmland`), hidratación por agua hasta 4 bloques de radio horizontal, ciclo biológico de cultivos dependiente de luz ($\ge 9$) para Trigo, Zanahorias y Patatas. Cosechas con devolución garantizada de semillas asegurando sostenibilidad alimentaria estricta sin trampas ni pérdida de stock. Pesca con caña en fuentes de agua con tabla canónica de peces (80%), tesoros (10%) y chatarra (10%).
4. **Fauna, Reproducción, Comercio Aldeano y Transporte Náutico (Brechas B12, B13, B15)**: Expansión de criaturas con Oveja, Cerdo, Gallina y Lobo. Sistema de reproducción mediante alimentación específica en modo celo/amor con generación de crías y dispersión de experiencia. Domesticación y obediencia de lobos con hueso. Comercio aldeano con profesiones (Granjero, Bibliotecario, Armero), reposición periódica de existencias y resolución atómica en concurrencia (cero duplicación de ítems, cero pérdida de moneda). Barcos flotantes con física de flotabilidad y orientación mediante brújula hacia el punto de reaparición inicial del mundo.

---

## 2. Requisitos y Criterios de Aceptación Verificados

| Componente | Criterio de paridad con el juego de referencia | Estado | Verificación |
|---|---|---|---|
| **Biomas y Generador v3** | Modelo climático continuo ($T, H, C, R$) que asigna de forma determinista Plains, Forest, Desert, Snowy, Mountain y Ocean. Bloques de superficie acordes (Arena/Arenisca en desierto, Nieve/Hielo en bioma frío, Piedra expuesta en picos montañosos). 100 semillas distintas probadas sin generar `NaN`, colapsos ni valores anómalos. Distribución de menas con esmeralda exclusiva en alturas de montaña ($y \ge 70$). | ✅ Cumplido | `WorldGenerationBiomesAnd100SeedsTest` |
| **Estructuras Multi-Chunk y Botín** | Generación de aldeas, mazmorras y ruinas cuyos límites cruzan bordes de chunks. Los chunks se generan de forma idéntica en orden normal, inverso y aleatorio/diagonal (mismo hash criptográfico). Cofres con contenido determinista derivado del anclaje de estructura; cero duplicación y persistencia completa en Schema v8. | ✅ Cumplido | `StructuresAndMultiChunkDeterminismTest` |
| **Agricultura y Renovabilidad** | Farmland detecta hidratación por agua adyacente ($\le 4$ bloques). Los cultivos solo crecen si reciben luz efectiva $\ge 9$ y suelo arado debajo. Cosecha de 10 ciclos de trigo devuelve más semillas de las consumidas ($\ge 10$), garantizando un ecosistema cerrado autosustentable. Pesca autoritativa en agua con tiempos de espera canónicos y recompensas diferenciadas. | ✅ Cumplido | `AgricultureGrowthAndRenewableCycleTest` |
| **Economía Aldeana y Concurrencia** | Aldeanos con profesiones canónicas y comercio basado en esmeraldas. Dos clientes envían simultáneamente transacciones para una oferta con $1$ solo uso restante: exactamente una transacción tiene éxito y la otra es rechazada con `TradeResult::OutOfStock`. Cero duplicación de esmeraldas y reposición periódica a stock completo. | ✅ Cumplido | `VillagerEconomyAndConcurrentTradeTest` |
| **Reproducción y Domesticación** | Trigo/zanahoria/semillas activan modo amor en parejas de la misma especie; procreación genera una cría proporcional y orbes de experiencia con enfriamiento de 6000 ticks. Huesos domestican lobos con salud expandida a 20 HP, persistencia y comando de sentarse. | ✅ Cumplido | `VillagerEconomyAndConcurrentTradeTest` |
| **Navegación Náutica y Brújula** | Entidad de barco con flotabilidad hidrostática sobre bloques de agua, amortiguación de arrastre y velocidad horizontal. Brújula que calcula con precisión trigonométrica el vector director hacia el punto de spawn del mundo (`spawnPoint - playerPos`). | ✅ Cumplido | `VillagerEconomyAndConcurrentTradeTest` |
| **Retrocompatibilidad y Persistencia** | Compatibilidad hacia atrás garantizada en `WorldGenerator` (soporta mundos v1, v2 y v3). Formato de guardado extendido a `CURRENT_SCHEMA_VERSION = 8` con serialización limpia de nuevos bloques y entidades. 0 regresiones en las 40 pruebas existentes. | ✅ Cumplido | CTest Suites 1 a 40 |

---

## 3. Resultados de Pruebas Automatizadas (CTest)

Total de suites de prueba ejecutadas: **44/44 pasando limpiamente (100% de éxito en 15.94 segundos)**.

```
Test project <repo>/build
      Start  1: SimulationDeterminismTest
 1/44 Test  #1: SimulationDeterminismTest ................   Passed    0.11 sec
      Start  2: BlockRegistryTest
 2/44 Test  #2: BlockRegistryTest ........................   Passed    0.00 sec
      Start  3: NetworkPacketSecurityTest
 3/44 Test  #3: NetworkPacketSecurityTest ................   Passed    0.00 sec
      Start  4: GameCommandsAndChunkBordersTest
 4/44 Test  #4: GameCommandsAndChunkBordersTest ..........   Passed    0.07 sec
      Start  5: PersistenceDeterminismTest
 5/44 Test  #5: PersistenceDeterminismTest ...............   Passed    0.19 sec
      Start  6: TransactionRecoveryTest
 6/44 Test  #6: TransactionRecoveryTest ..................   Passed    0.01 sec
      Start  7: CorruptionAndIntegrityTest
 7/44 Test  #7: CorruptionAndIntegrityTest ...............   Passed    0.03 sec
      Start  8: SchemaMigrationAndBackupTest
 8/44 Test  #8: SchemaMigrationAndBackupTest .............   Passed    0.01 sec
      Start  9: WorldManagerAndLockTest
 9/44 Test  #9: WorldManagerAndLockTest ..................   Passed    0.00 sec
      Start 10: GeneratorDeterminismTest
10/44 Test #10: GeneratorDeterminismTest .................   Passed    0.18 sec
      Start 11: StreamingCacheAndMemoryTest
11/44 Test #11: StreamingCacheAndMemoryTest ..............   Passed    0.15 sec
      Start 12: StreamingPersistenceTest
12/44 Test #12: StreamingPersistenceTest .................   Passed    0.06 sec
      Start 13: ConcurrencyAndBoundariesTest
13/44 Test #13: ConcurrencyAndBoundariesTest .............   Passed    0.00 sec
      Start 14: FrustumCullingTest
14/44 Test #14: FrustumCullingTest .......................   Passed    0.00 sec
      Start 15: InventoryConservationTest
15/44 Test #15: InventoryConservationTest ................   Passed    0.00 sec
      Start 16: ItemEntityPickupTest
16/44 Test #16: ItemEntityPickupTest .....................   Passed    0.08 sec
      Start 17: ChestTransactionsTest
17/44 Test #17: ChestTransactionsTest ....................   Passed    0.03 sec
      Start 18: InventoryPersistenceTest
18/44 Test #18: InventoryPersistenceTest .................   Passed    0.07 sec
      Start 19: ProductionChainTest
19/44 Test #19: ProductionChainTest ......................   Passed    0.03 sec
      Start 20: RecipeCapacityAndBatchTest
20/44 Test #20: RecipeCapacityAndBatchTest ...............   Passed    0.03 sec
      Start 21: FurnaceSimulationPersistenceTest
21/44 Test #21: FurnaceSimulationPersistenceTest .........   Passed    0.07 sec
      Start 22: ToolDurabilityAndMiningTest
22/44 Test #22: ToolDurabilityAndMiningTest ..............   Passed    0.02 sec
      Start 23: SurvivalStatsTest
23/44 Test #23: SurvivalStatsTest ........................   Passed    0.03 sec
      Start 24: DamageAndHazardsTest
24/44 Test #24: DamageAndHazardsTest .....................   Passed    0.03 sec
      Start 25: MovementAndCollisionTest
25/44 Test #25: MovementAndCollisionTest .................   Passed    0.03 sec
      Start 26: DeathAndRespawnTest
26/44 Test #26: DeathAndRespawnTest ......................   Passed    0.05 sec
      Start 27: BedAndGameModesTest
27/44 Test #27: BedAndGameModesTest ......................   Passed    0.02 sec
      Start 28: LightingAndPropagationTest
28/44 Test #28: LightingAndPropagationTest ...............   Passed    0.01 sec
      Start 29: FluidFlowAndInteractionTest
29/44 Test #29: FluidFlowAndInteractionTest ..............   Passed    0.01 sec
      Start 30: ImmersionAndEnvironmentalHazardsTest
30/44 Test #30: ImmersionAndEnvironmentalHazardsTest .....   Passed    0.03 sec
      Start 31: ScheduledUpdatesAndWeatherTest
31/44 Test #31: ScheduledUpdatesAndWeatherTest ...........   Passed    0.03 sec
      Start 32: RenderPassesAndAoTest
32/44 Test #32: RenderPassesAndAoTest ....................   Passed    0.00 sec
      Start 33: PathfindingAndNavigationTest
33/44 Test #33: PathfindingAndNavigationTest .............   Passed    0.03 sec
      Start 34: MobSpawningAndLightRulesTest
34/44 Test #34: MobSpawningAndLightRulesTest .............   Passed    0.03 sec
      Start 35: CombatDamageAndLootTransactionTest
35/44 Test #35: CombatDamageAndLootTransactionTest .......   Passed    0.03 sec
      Start 36: MobArchetypesAndPersistenceTest
36/44 Test #36: MobArchetypesAndPersistenceTest ..........   Passed    0.09 sec
      Start 37: MultiplayerSessionAndHandshakeTest
37/44 Test #37: MultiplayerSessionAndHandshakeTest .......   Passed    0.13 sec
      Start 38: AuthoritativeActionsAndReachTest
38/44 Test #38: AuthoritativeActionsAndReachTest .........   Passed    0.16 sec
      Start 39: SnapshotAndChunkReplicationTest
39/44 Test #39: SnapshotAndChunkReplicationTest ..........   Passed    0.23 sec
      Start 40: DegradedNetworkAndConvergenceTest
40/44 Test #40: DegradedNetworkAndConvergenceTest ........   Passed    0.21 sec
      Start 41: WorldGenerationBiomesAnd100SeedsTest
41/44 Test #41: WorldGenerationBiomesAnd100SeedsTest .....   Passed   13.52 sec
      Start 42: StructuresAndMultiChunkDeterminismTest
42/44 Test #42: StructuresAndMultiChunkDeterminismTest ...   Passed    0.05 sec
      Start 43: AgricultureGrowthAndRenewableCycleTest
43/44 Test #43: AgricultureGrowthAndRenewableCycleTest ...   Passed    0.03 sec
      Start 44: VillagerEconomyAndConcurrentTradeTest
44/44 Test #44: VillagerEconomyAndConcurrentTradeTest ....   Passed    0.03 sec

100% tests passed out of 44

Total Test time (real) =  15.94 sec
```

---

## 4. Criterios de Salida Cumplidos
- [x] Generador v3 implementado con biomas continuos, cuevas 3D y distribución canónica de minerales.
- [x] Estructuras multi-chunk con botín determinista y anclajes invariantes respecto al orden de generación.
- [x] Agricultura completa con Farmland hidratado, crecimiento dependiente de luz y sostenibilidad de semillas.
- [x] Pesca funcional con tabla canónica de probabilidad (peces, tesoro, chatarra).
- [x] Fauna ampliada (Oveja, Cerdo, Gallina, Lobo), reproducción con modo amor y domesticación de lobos.
- [x] Aldeanos con profesiones, comercio atómico concurrente y reposición de existencias.
- [x] Transporte con barcos (física acuática) y navegación con brújula hacia el punto de spawn.
- [x] Persistencia Schema v8 con retrocompatibilidad para mundos v1 y v2.
- [x] 4 nuevas suites de prueba automatizadas (44/44 pruebas totales pasando al 100%).
- [x] Cero regresiones en funcionalidades previas del motor.
