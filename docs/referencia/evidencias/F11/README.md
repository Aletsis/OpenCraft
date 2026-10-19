# Dossier de Evidencias — Fase 11: Automatización, Cinabrio, Máquinas y Transporte

**Fecha de Ejecución y Certificación:** 16 de Octubre de 2026  
**Versión de Referencia:** la versión 1.21 del juego de referencia  
**Esquema de Persistencia:** Esquema v9 (`CURRENT_SCHEMA_VERSION = 9`)  
**Hito Jugable Alcanzado:** 🟢 **H5 — Mundo Productivo** (Granjas automatizadas, transporte de vagonetas, crafter autónomo 1.21 y circuitos deterministas de cinabrio)

---

## 1. Resumen Ejecutivo de Entrega

La **Fase 11 (Automatización: Cinabrio, máquinas y transporte de objetos)** cierra integralmente la brecha **B14 (Automatización)** y expande la brecha **B15 (Transporte: rieles y vagonetas)** bajo estricta paridad canónica con Java 1.21.

Se han incorporado:
1. **Motor Canónico de Cinabrio (`CinnabarEngine`):**
   - Propagación de señal BFS acotada con atenuación estándar (niveles 15 a 0).
   - Distinción estricta de alimentación fuerte (*strong powering*) y débil (*weak powering*).
   - Bloques emisores: bloque de cinabrio, antorcha de cinabrio (con delay de 2 ticks e inversión lógica, sin alimentar hacia abajo a su propio bloque base), palanca (persistente), botones (20 ticks piedra, 30 ticks madera), placas de presión y detector de luz diurna (modos diurno/nocturno).
   - Repetidores de cinabrio: retardos configurables (2, 4, 6, 8 ticks de juego = 1 a 4 ticks de cinabrio) y función de cerrojo/latch (*repeater lock* lateral).
   - Comparadores analógicos: sustracción vs. comparación, lectura analógica de contenedores (cofres, hornos, tolvas, dispensadores, soltadores) y fórmula canónica de Crafter 1.21.
   - Paridad Java de **Cuasiconectividad** (*Quasi-Connectivity*): activación de actuadores si el bloque superior recibe energía.

2. **Simulador de Pistones y Movimiento de Bloques (`PistonSimulator`):**
   - Límite canónico de empuje de **12 bloques**.
   - Resistencia e inmovilidad de bloques especiales (Bedrock, Obsidiana, Bloques con Entidad/Cofres).
   - Ruptura de bloques frágiles (antorchas, cable de cinabrio, cultivos).
   - Pistón pegajoso (*Sticky Piston*) con mecánica Java de **pulso de 1 tick** (*1-tick block drop*): al recibir un pulso ultra-corto, deja atrás el bloque sin arrastrarlo.

3. **Bloques Entidad de Automatización y Transporte:**
   - **Tolva (`HopperBlockEntity`):** 5 ranuras, succión 3D de entidades de ítems superiores, enfriamiento canónico de 8 ticks (0.4s), bloqueo por señal de cinabrio, y transferencia atómica direccionada.
   - **Soltador (`DropperBlockEntity`):** 9 ranuras, inyección directa en contenedores adyacentes o eyectado de ítems al mundo.
   - **Dispensador (`DispenserBlockEntity`):** 9 ranuras, disparo de proyectiles de flecha (`ProjectileEntity`) y vaciado de cubetas de agua/lava con colocación de fluidos.
   - **Fabricador Autónomo 1.21 (`CrafterBlockEntity`):** Cuadrícula 3x3, máscara de ranuras deshabilitadas (`m_disabledSlotsMask`), activación por pulso de cinabrio, consumo atómico de 1 unidad por ranura habilitada, eyectado al mundo o inyección en contenedor adyacente, y señal analógica de comparador (`disabledSlots + nonEmptySlots`).

4. **Sistema de Rieles y Vagonetas (`RailSystem` & `MinecartEntity`):**
   - Identificación de rieles normales, propulsores (`PoweredRail`), detectores (`DetectorRail`) y activadores.
   - Física de vagonetas en rieles: inercia, fricción, aceleración en riel propulsor encendido y freno dinámico en riel propulsor apagado.
   - Generación de señal 15 por riel detector al paso de una vagoneta.
   - Variantes de vagoneta: Estándar, Vagoneta con cofre (27 ranuras) y Vagoneta con tolva (5 ranuras con recolección automática de ítems a su paso).

5. **Persistencia y Migración de Datos (Esquema v9):**
   - Serialización binaria endian-safe con checksum CRC32 en `WorldFormat.cpp` y `WorldFormat.hpp`.
   - Compatibilidad e idempotencia de migración desde esquemas v1 a v8 hacia el esquema v9 sin regresiones.

---

## 2. Resultados de la Suite Automatizada (48 / 48 Tests Superados)

La suite de CTest ha alcanzado **48 pruebas ejecutadas y 100% aprobadas** de manera continua e ininterrumpida:

```text
Test project <repo>/build
 1/48 Test  #1: SimulationDeterminismTest ................   Passed    0.09 sec
 2/48 Test  #2: BlockRegistryTest ........................   Passed    0.00 sec
 3/48 Test  #3: NetworkPacketSecurityTest ................   Passed    0.00 sec
 4/48 Test  #4: GameCommandsAndChunkBordersTest ..........   Passed    0.07 sec
 5/48 Test  #5: PersistenceDeterminismTest ...............   Passed    0.19 sec
 6/48 Test  #6: TransactionRecoveryTest ..................   Passed    0.01 sec
 7/48 Test  #7: CorruptionAndIntegrityTest ...............   Passed    0.04 sec
 8/48 Test  #8: SchemaMigrationAndBackupTest .............   Passed    0.01 sec
 9/48 Test  #9: WorldManagerAndLockTest ..................   Passed    0.01 sec
10/48 Test #10: GeneratorDeterminismTest .................   Passed    0.20 sec
11/48 Test #11: StreamingCacheAndMemoryTest ..............   Passed    0.15 sec
12/48 Test #12: StreamingPersistenceTest .................   Passed    0.07 sec
13/48 Test #13: ConcurrencyAndBoundariesTest .............   Passed    0.01 sec
14/48 Test #14: FrustumCullingTest .......................   Passed    0.00 sec
15/48 Test #15: InventoryConservationTest ................   Passed    0.00 sec
16/48 Test #16: ItemEntityPickupTest .....................   Passed    0.09 sec
17/48 Test #17: ChestTransactionsTest ....................   Passed    0.03 sec
18/48 Test #18: InventoryPersistenceTest .................   Passed    0.08 sec
19/48 Test #19: ProductionChainTest ......................   Passed    0.03 sec
20/48 Test #20: RecipeCapacityAndBatchTest ...............   Passed    0.03 sec
21/48 Test #21: FurnaceSimulationPersistenceTest .........   Passed    0.08 sec
22/48 Test #22: ToolDurabilityAndMiningTest ..............   Passed    0.03 sec
23/48 Test #23: SurvivalStatsTest ........................   Passed    0.03 sec
24/48 Test #24: DamageAndHazardsTest .....................   Passed    0.03 sec
25/48 Test #25: MovementAndCollisionTest .................   Passed    0.03 sec
26/48 Test #26: DeathAndRespawnTest ......................   Passed    0.05 sec
27/48 Test #27: BedAndGameModesTest ......................   Passed    0.03 sec
28/48 Test #28: LightingAndPropagationTest ...............   Passed    0.01 sec
29/48 Test #29: FluidFlowAndInteractionTest ..............   Passed    0.01 sec
30/48 Test #30: ImmersionAndEnvironmentalHazardsTest .....   Passed    0.03 sec
31/48 Test #31: ScheduledUpdatesAndWeatherTest ...........   Passed    0.03 sec
32/48 Test #32: RenderPassesAndAoTest ....................   Passed    0.00 sec
33/48 Test #33: PathfindingAndNavigationTest .............   Passed    0.04 sec
34/48 Test #34: MobSpawningAndLightRulesTest .............   Passed    0.04 sec
35/48 Test #35: CombatDamageAndLootTransactionTest .......   Passed    0.03 sec
36/48 Test #36: MobArchetypesAndPersistenceTest ..........   Passed    0.10 sec
37/48 Test #37: MultiplayerSessionAndHandshakeTest .......   Passed    0.14 sec
38/48 Test #38: AuthoritativeActionsAndReachTest .........   Passed    0.19 sec
39/48 Test #39: SnapshotAndChunkReplicationTest ..........   Passed    0.37 sec
40/48 Test #40: DegradedNetworkAndConvergenceTest ........   Passed    0.40 sec
41/48 Test #41: WorldGenerationBiomesAnd100SeedsTest .....   Passed   14.40 sec
42/48 Test #42: StructuresAndMultiChunkDeterminismTest ...   Passed    0.06 sec
43/48 Test #43: AgricultureGrowthAndRenewableCycleTest ...   Passed    0.03 sec
44/48 Test #44: VillagerEconomyAndConcurrentTradeTest ....   Passed    0.03 sec
45/48 Test #45: CinnabarLogicAndTimingTest ...............   Passed    0.06 sec
46/48 Test #46: PistonsAndBlockMovementTest ..............   Passed    0.01 sec
47/48 Test #47: HoppersDroppersAndCrafterTest ............   Passed    0.01 sec
48/48 Test #48: RailsAndMinecartTransportTest ............   Passed    0.00 sec

100% tests passed out of 48
Total Test time (real) = 17.41 sec
```

---

## 3. Pruebas Específicas de la Fase 11

### Test 45: `test_cinnabar_logic_and_timing`
- **Atenuación BFS (15 -> 0):** Comprobada propagación sobre 16 bloques consecutivos de polvo de cinabrio (15 -> 14 ... 1 -> 0).
- **Inversión y Retardo de Antorchas:** Comprobado que la antorcha no alimenta hacia abajo su propio bloque de soporte y se apaga correctamente al energizarse el bloque inferior.
- **Repetidor (Latch):** Retardo configurable y bloqueo de estado mediante señal lateral.
- **Comparador Analógico:** Cálculo proporcional sobre inventario de contenedor (0, 1 y 15).
- **Comparador con Crafter 1.21:** Señal equivalente a ranuras deshabilitadas + ranuras no vacías.
- **Detector de Luz Diurna:** Nivel 15 a mediodía, nivel 0 a medianoche.
- **Estrés de Reloj Oscilador:** 500 ciclos de conmutación rápida sin desbordamiento de presupuesto ni bucles infinitos.

### Test 46: `test_pistons_and_block_movement`
- **Límite Canónico de 12 Bloques:** Empuje exitoso de 12 bloques y rechazo estricto de 13 bloques sin corrupción.
- **Bloques Inamovibles:** Bedrock, Obsidiana y Cofres resisten el empuje.
- **Bloques Frágiles:** Ruptura con conversión a ítem (antorchas, cables).
- **Pistón Pegajoso y Pulso de 1 Tick:** Retracción normal arrastra el bloque; pulso de 1 tick suelta el bloque (*block drop*).
- **Cuasiconectividad:** Detección de energía en el bloque inmediatamente superior.

### Test 47: `test_hoppers_droppers_and_crafter`
- **Tolva:** Succión 3D de entidades flotantes con activación de enfriamiento de 8 ticks.
- **Bloqueo por Cinabrio:** Inhibición total de absorción y expulsión ante señal activa.
- **Conservación de Masa:** Transferencia secuencial de 1,000 ítems exactos entre tolva y cofre sin pérdida ni duplicación.
- **Soltador:** Eyección e inyección en contenedor contiguo.
- **Dispensador:** Flechas eyectadas como entidades de proyectil balístico y cubetas vaciadas con colocación de agua líquida en el mundo.
- **Crafter 1.21:** Reconocimiento de receta 3x3 respetando ranuras deshabilitadas, consumo de 1 ítem por ranura activa y expulsión del producto.

### Test 48: `test_rails_and_minecart_transport`
- **Clasificación:** Riel común, propulsor, detector y activador.
- **Física y Fricción:** Movimiento guiado por la vía con desaceleración natural.
- **Riel Propulsor:** Freno drástico cuando está apagado y aceleración continua hacia la velocidad máxima (8 m/s) cuando está encendido.
- **Riel Detector:** Generación de señal 15 al entrar en contacto con una vagoneta.
- **Vagoneta con Tolva:** Absorción automática de ítems sueltos sobre la vía durante el trayecto.
- **Vagoneta con Cofre:** Almacenamiento portátil de 27 ranuras.

---

## 4. Estado de Puerta de Salida y Cierre de Hito H5

- [x] Todas las pruebas unitarias y de integración de Fase 11 creadas y superadas al 100%.
- [x] Regresión cero en fases anteriores (48/48 pruebas superadas).
- [x] Persistencia y migración de Esquema v9 verificadas.
- [x] Documentación actualizada en Plan Maestro y Matriz de Brechas.
- [x] Hito **H5 — Mundo Productivo** formalmente completado.
