# Dossier de Evidencias: Fase 09 — Multijugador Autoritativo y Servidor Dedicado (H4: Cooperación)

## 1. Resumen Ejecutivo
La **Fase 09** culmina el hito **H4: Cooperación** de la hoja de ruta de paridad con el juego de referencia canónico al dotar a OpenCraft de una arquitectura autoritativa completa cliente-servidor, un protocolo binario versionado y serializado explícitamente (`PROTOCOL_VERSION = 100`, Magic `OCNT`), un canal de transporte UDP fiable y ordenado (`ReliableChannel`) con simulación y recuperación ante condiciones de red degradada (100 ms RTT, 2% pérdida de paquetes, duplicación y reordenamiento), un subsistema multi-jugador en la simulación autoritativa (`WorldSession`), replicación bidireccional de entidades y snapshots, administración remota (/kick, /op, /whitelist), y un ejecutable dedicado headless independiente de SDL3 y GLESv2 (`opencraft_server`).

La persistencia atómica en disco se extiende para gestionar perfiles de múltiples jugadores conectados simultáneamente, resolviendo de raíz la brecha **B10** sin duplicación ni pérdida de inventarios en condiciones de carrera concurrentes.

---

## 2. Requisitos y Criterios de Aceptación Verificados

| Componente | Criterio de paridad con el juego de referencia | Estado | Verificación |
|---|---|---|---|
| **Protocolo Versionado y Serialización** | Cabecera mágica `0x4F434E54` (`OCNT`), versión de protocolo canónica 100, serialización/deserialización explícita con protección de desbordamiento de búfer y saneamiento de cadenas de texto (`ByteReader`/`ByteWriter`). Negociación Handshake/HandshakeAck y rechazo explícito con motivo (`HandshakeReject`) ante versiones cliente incompatibles o nombres inválidos. | ✅ Cumplido | `MultiplayerSessionAndHandshakeTest` |
| **Arquitectura Autoritativa y Alcance (Reach)** | Servidor valida rigurosamente todas las intenciones (`GameCommand`) enviadas por los clientes antes de mutar el mundo. Verificación de alcance de interacción (distancia ocular máxima $\le 5.0\text{ m}$); intentos de minar o colocar bloques a distancias superiores son rechazados con `CommandResult::OutOfReach` sin mutación no autorizada. | ✅ Cumplido | `AuthoritativeActionsAndReachTest` |
| **Concurrencia de Contenedores y Conservación de Ítems** | Dos clientes intentan extraer concurrentemente el último objeto de una ranura de cofre (`InventoryTransactionCommand`). Mediante control optimista de concurrencia (`expectedRevision`), exactamente una solicitud tiene éxito y la segunda es rechazada limpiamente con `RevisionConflict`. Conservación matemática estricta: cero pérdida de objetos, cero duplicación. | ✅ Cumplido | `AuthoritativeActionsAndReachTest` |
| **Combate y Mitigación PvP** | Daño autoritativo con validación de alcance cuerpo a cuerpo ($\le 3.5\text{ m}$), fórmula cuadrática canónica de mitigación por armadura `DañoEfectivo = Daño * (1.0 - puntos * 0.04)`, absorción al 100% con escudo frontal activo y ventana de invulnerabilidad post-daño (`hurtTime` = 10 ticks). | ✅ Cumplido | `AuthoritativeActionsAndReachTest` |
| **Replicación de Snapshot y Entidades** | Sincronización completa de estado al unirse nuevos clientes (entidades activas: mobs, objetos caídos, proyectiles, orbes de XP, otros jugadores). Replicación instantánea de mutaciones de bloques en tiempo real y telemetría de movimiento a todos los clientes suscritos. Reconexión limpia de clientes tardíos preservando inventarios y estado de mundo. | ✅ Cumplido | `SnapshotAndChunkReplicationTest` |
| **Canal Fiable y Red Degradada** | Transporte fiable sobre UDP con números de secuencia, retransmisiones automáticas tras timeout RTO (150 ms), deduplicación y cola de reensamblado secuencial. Probado bajo degradación severa: **100 ms RTT, 2% de pérdida, duplicados y reordenamiento**. Tras converger, los hashes de simulación (`computeStateHash()`) coinciden exactamente entre servidor y cliente (cero divergencia permanente). | ✅ Cumplido | `DegradedNetworkAndConvergenceTest` |
| **Servidor Dedicado Headless (`opencraft_server`)** | Ejecutable de servidor sin ventana ni dependencias gráficas, con argumentos CLI (`--port`, `--world`, `--seed`, `--max-players`, `--whitelist`, `--save-interval`), ticking fijo a 20 Hz, autoguardado periódico y apagado limpio ante señales POSIX (`SIGINT`, `SIGTERM`). | ✅ Cumplido | `opencraft_server` |
| **Administración, Seguridad y Chat** | Sistema de lista blanca (whitelist) persistente, comandos de administración (/kick, /op, /whitelist), difusión de chat entre participantes, validación de remitente anti-spoofing y desconexión automática por timeout tras 10 segundos de silencio. | ✅ Cumplido | `MultiplayerSessionAndHandshakeTest` |

---

## 3. Resultados de Pruebas Automatizadas (CTest)

Total de suites de prueba ejecutadas: **40/40 pasando limpiamente (100% de éxito en 1.69 segundos)**.

```
Test project <repo>/build
      Start  1: SimulationDeterminismTest
 1/40 Test  #1: SimulationDeterminismTest ..............   Passed    0.06 sec
      Start  2: BlockRegistryTest
 2/40 Test  #2: BlockRegistryTest ......................   Passed    0.00 sec
      Start  3: NetworkPacketSecurityTest
 3/40 Test  #3: NetworkPacketSecurityTest ..............   Passed    0.00 sec
      Start  4: GameCommandsAndChunkBordersTest
 4/40 Test  #4: GameCommandsAndChunkBordersTest ........   Passed    0.05 sec
      Start  5: PersistenceDeterminismTest
 5/40 Test  #5: PersistenceDeterminismTest .............   Passed    0.15 sec
      Start  6: TransactionRecoveryTest
 6/40 Test  #6: TransactionRecoveryTest ................   Passed    0.01 sec
      Start  7: CorruptionAndIntegrityTest
 7/40 Test  #7: CorruptionAndIntegrityTest .............   Passed    0.03 sec
      Start  8: SchemaMigrationAndBackupTest
 8/40 Test  #8: SchemaMigrationAndBackupTest ...........   Passed    0.01 sec
      Start  9: WorldManagerAndLockTest
 9/40 Test  #9: WorldManagerAndLockTest ................   Passed    0.00 sec
      Start 10: GeneratorDeterminismTest
10/40 Test #10: GeneratorDeterminismTest ...............   Passed    0.03 sec
      Start 11: StreamingCacheAndMemoryTest
11/40 Test #11: StreamingCacheAndMemoryTest ............   Passed    0.11 sec
      Start 12: StreamingPersistenceTest
12/40 Test #12: StreamingPersistenceTest ...............   Passed    0.06 sec
      Start 13: ConcurrencyAndBoundariesTest
13/40 Test #13: ConcurrencyAndBoundariesTest ...........   Passed    0.00 sec
      Start 14: FrustumCullingTest
14/40 Test #14: FrustumCullingTest .....................   Passed    0.00 sec
      Start 15: InventoryConservationTest
15/40 Test #15: InventoryConservationTest ..............   Passed    0.00 sec
      Start 16: ItemEntityPickupTest
16/40 Test #16: ItemEntityPickupTest ...................   Passed    0.05 sec
      Start 17: ChestTransactionsTest
17/40 Test #17: ChestTransactionsTest ..................   Passed    0.02 sec
      Start 18: InventoryPersistenceTest
18/40 Test #18: InventoryPersistenceTest ...............   Passed    0.06 sec
      Start 19: ProductionChainTest
19/40 Test #19: ProductionChainTest ....................   Passed    0.02 sec
      Start 20: RecipeCapacityAndBatchTest
20/40 Test #20: RecipeCapacityAndBatchTest .............   Passed    0.02 sec
      Start 21: FurnaceSimulationPersistenceTest
21/40 Test #21: FurnaceSimulationPersistenceTest .......   Passed    0.06 sec
      Start 22: ToolDurabilityAndMiningTest
22/40 Test #22: ToolDurabilityAndMiningTest ............   Passed    0.02 sec
      Start 23: SurvivalStatsTest
23/40 Test #23: SurvivalStatsTest ......................   Passed    0.02 sec
      Start 24: DamageAndHazardsTest
24/40 Test #24: DamageAndHazardsTest ...................   Passed    0.03 sec
      Start 25: MovementAndCollisionTest
25/40 Test #25: MovementAndCollisionTest ...............   Passed    0.02 sec
      Start 26: DeathAndRespawnTest
26/40 Test #26: DeathAndRespawnTest ....................   Passed    0.04 sec
      Start 27: BedAndGameModesTest
27/40 Test #27: BedAndGameModesTest ....................   Passed    0.02 sec
      Start 28: LightingAndPropagationTest
28/40 Test #28: LightingAndPropagationTest .............   Passed    0.01 sec
      Start 29: FluidFlowAndInteractionTest
29/40 Test #29: FluidFlowAndInteractionTest ............   Passed    0.01 sec
      Start 30: ImmersionAndEnvironmentalHazardsTest
30/40 Test #30: ImmersionAndEnvironmentalHazardsTest ...   Passed    0.02 sec
      Start 31: ScheduledUpdatesAndWeatherTest
31/40 Test #31: ScheduledUpdatesAndWeatherTest .........   Passed    0.03 sec
      Start 32: RenderPassesAndAoTest
32/40 Test #32: RenderPassesAndAoTest ..................   Passed    0.00 sec
      Start 33: PathfindingAndNavigationTest
33/40 Test #33: PathfindingAndNavigationTest ...........   Passed    0.02 sec
      Start 34: MobSpawningAndLightRulesTest
34/40 Test #34: MobSpawningAndLightRulesTest ...........   Passed    0.03 sec
      Start 35: CombatDamageAndLootTransactionTest
35/40 Test #35: CombatDamageAndLootTransactionTest .....   Passed    0.02 sec
      Start 36: MobArchetypesAndPersistenceTest
36/40 Test #36: MobArchetypesAndPersistenceTest ........   Passed    0.07 sec
      Start 37: MultiplayerSessionAndHandshakeTest
37/40 Test #37: MultiplayerSessionAndHandshakeTest .....   Passed    0.17 sec
      Start 38: AuthoritativeActionsAndReachTest
38/40 Test #38: AuthoritativeActionsAndReachTest .......   Passed    0.10 sec
      Start 39: SnapshotAndChunkReplicationTest
39/40 Test #39: SnapshotAndChunkReplicationTest ........   Passed    0.19 sec
      Start 40: DegradedNetworkAndConvergenceTest
40/40 Test #40: DegradedNetworkAndConvergenceTest ......   Passed    0.12 sec

100% tests passed out of 40
Total Test time (real) =   1.69 sec
```

---

## 4. Verificación de la Puerta de Salida (Exit Criteria)

- [x] **Todas las pruebas específicas de F09 pasan y se conservan sus evidencias**: 4 nuevas suites de prueba (`MultiplayerSessionAndHandshakeTest`, `AuthoritativeActionsAndReachTest`, `SnapshotAndChunkReplicationTest`, `DegradedNetworkAndConvergenceTest`) superadas al 100%.
- [x] **La regresión de fases anteriores afectadas sigue pasando**: Las 36 suites de prueba de F01 a F08 continúan pasando sin ninguna alteración ni degradación en su comportamiento determinista.
- [x] **Guardado/carga y límites de recursos se prueban con los nuevos datos**: Perfiles de múltiples jugadores gestionados de forma atómica en `WorldStorage`, sin fugas de memoria ni descriptores de socket zombi tras desconexión.
- [x] **No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado**: Verificado matemáticamente en transacciones concurrentes de cofres e inventarios compartidos.
- [x] **Las diferencias con la referencia y el trabajo de catálogo restante están identificados**: Brecha B10 cerrada por completo; la siguiente fase F10 (Exploración, flora, agricultura y ecosistemas) queda desbloqueada.
- [x] **Hito H4 (Cooperación) completado**: Sesión cooperativa verificable con servidor dedicado y LAN.
