# Evidencias de Aceptación — Fase 06: Supervivencia, movimiento y reglas de partida

**Fecha:** 15 de Octubre de 2026  
**Build:** `OpenCraft 0.1.0` (CMake C++20, GCC 15, Arch Linux / POSIX)  
**Suite de Pruebas:** 27/27 pruebas automáticas aprobadas al 100% (5 pruebas específicas de Fase 06 añadidas a CTest).

---

## 1. Resumen de Implementación

En la Fase 06 se implementó el motor integral de supervivencia, movimiento, daño y reglas de juego de OpenCraft, transformando la interacción con el mundo en una experiencia con riesgos, penalizaciones deterministas y recuperación:

1. **Metabolismo, Hambre y Salud (`PlayerEntity`, `WorldSession`):**
   - Atributos canónicos de supervivencia: Salud (20.0 HP / 10 corazones), Hambre (20 puntos / 10 muslos), Saturación (5.0 canónica inicial, hasta 20.0) y Agotamiento (exhaustion acumulativo 0.0 a 4.0).
   - Consumo de alimentos con saturación (`ItemDefinition::saturationRestore`): Manzana (4 hambre, 2.4 sat), Pan (5 hambre, 6.0 sat), Filete cocinado (8 hambre, 12.8 sat) y Ternera cruda (3 hambre, 1.8 sat).
   - Bloqueo de alimentación si el hambre está al máximo (20), salvo items que tengan excepciones.
   - Acumulación de agotamiento por acciones (saltar, correr, minar) y deducción determinista hacia saturación y hambre cada 4.0 puntos.
   - Regeneración canónica simulada a 20 Hz por tick: regeneración rápida (1 HP cada 10 ticks si saturación > 0 y hambre >= 20) y regeneración estándar (1 HP cada 80 ticks si hambre >= 18).
   - Inanición: cuando el hambre llega a 0, se aplica 1 daño de inanición cada 80 ticks. En dificultad Normal preserva al jugador en 1 HP; en Difícil le produce la muerte.

2. **Movimiento, Agachado, Sprint y Prevención de Tunneling (`WorldSession`, `Player`):**
   - Velocidades canónicas: Caminata (4.3 m/s), Sprint (~5.6 m/s con requisito de hambre > 6 y generación de agotamiento 0.1/bloque), Agachado (~1.3 m/s y altura de hitbox reducida a 1.5m).
   - Normalización de entrada diagonal en cliente y servidor: vectores diagonales acotados a magnitud 1.0 evitando aceleración indebida de $\sqrt{2}$.
   - Protección contra caídas en bordes (*sneak ledge drop protection*): al agacharse, el movimiento no permite caer sobre vacíos o caídas > 0.5 bloques.
   - Subpasos de colisión (*substepping / continuous collision sweep*): subdivisión de la integración de movimiento en pasos máximos de $\le 0.08$m, previniendo el atravesamiento de muros sólidos (*tunneling*) incluso a tasas lentas de fotogramas (15 FPS).

3. **Daño Ambiental, Peligros e Invulnerabilidad:**
   - Daño por caída: acumulación de distancia en caída libre `fallDistance`. Daño exacto $\lfloor \text{fallDistance} - 3.0 \rfloor$ al impactar en suelo.
   - Daño por vacío: daño implacable y continuo cuando la altitud del jugador cae por debajo de $Y < -96$.
   - Sistema de oxígeno y asfixia por inmersión: capacidad de 300 ticks (15s) de aire bajo el agua, pérdida de burbujas, 1 daño de ahogamiento cada 20 ticks al agotarse el aire, y recuperación rápida (5 ticks/tick) al salir a la superficie.
   - Daño por asfixia/sofocación dentro de bloques sólidos.
   - Ventana canónica de invulnerabilidad temporal (`hurtTime = 10` ticks / 0.5s): el jugador no recibe impactos sucesivos repetidos en el mismo instante.

4. **Ciclo de Muerte, Soltado de Botín y Reaparición Segura (`findSafeSpawn`, `RespawnCommand`):**
   - Transición de muerte: al llegar la salud a 0, se emite `PlayerDiedEvent`, se bloquea el movimiento y se activa el estado de muerte (`isAlive = false`).
   - Conservación estricta y soltado de botín: todos los objetos del inventario principal, hotbar, offhand y cursor se expulsan al mundo como entidades `ItemEntity` en la posición del deceso, y los inventarios se limpian al 100%.
   - Cero clonación/duplicación: guardar el mundo durante la transición de muerte y recargarlo no duplica items ni revive al jugador en estado ambiguo.
   - Reaparición segura: algoritmo `findSafeSpawn` que busca vertical y horizontalmente un espacio libre de 2 bloques de aire sobre un bloque sólido, impidiendo que el jugador reaparezca sofocado dentro de roca sólida.

5. **Bloque de Cama, Ciclo de Sueño y Modo de Juego (`BlockID::Bed`, `SleepInBedCommand`):**
   - Registro de `opencraft:bed` como bloque sólido con propiedades de descanso.
   - Interacción de día: establece el punto de reaparición del jugador (`hasCustomSpawnPoint = true`, `spawnPoint = pos`) sin alterar el reloj del mundo.
   - Descanso nocturno: si el tiempo del mundo se encuentra en la franja nocturna (tick % 24000 en $[12000, 23500]$), el jugador duerme y el reloj del mundo avanza instantáneamente al amanecer (tick 24000).
   - Manejo de cama obstruida o destruida: si al morir la cama fue rota o bloqueada, el motor detecta la ausencia del bloque cama y realiza un fallback seguro al punto de generación original del mundo (`worldSpawn`), informando al jugador.
   - Políticas de modo: Modo Creativo (inmunidad a daños ambientales, caída, ahogamiento y vacío; capacidad de romper Bedrock; sin consumo de hambre) frente a Modo Supervivencia (física estricta y consumo de recursos).

6. **Esquema de Persistencia v5 (`SCHEMA_VERSION_5`):**
   - Actualización del formato binario con protección CRC32 para persistir los atributos de supervivencia: saturación, agotamiento, aire, modo de juego (`gameMode`), coordenadas de spawn personalizado (`spawnPoint`, `hasCustomSpawnPoint`) y estado vital (`isAlive`).
   - Retrocompatibilidad completa con esquemas previos v1, v2, v3 y v4, garantizando que mundos anteriores se carguen inicializando los atributos de supervivencia con valores canónicos por defecto.

7. **HUD y Renderizado de Supervivencia (`HUD`, `Application`):**
   - Barra de salud: visualización dinámica de 10 corazones con cálculo de mitades y soporte visual para hurt-shake.
   - Barra de hambre: visualización de 10 muslos de comida en el extremo opuesto del hotbar.
   - Indicador de oxígeno: burbujas de aire flotantes cuando el jugador se sumerge en agua.
   - Pantalla de muerte: superposición translúcida roja al fallecer con texto "Has muerto" y botón interactivo funcional de "Reaparecer" (activable con ratón o tecla Espacio).
   - Atajos de teclado: Shift para agacharse, Ctrl para correr, F4 para conmutar entre Supervivencia y Creativo, y clic secundario para comer o usar camas.

---

## 2. Resultados de las Pruebas de Aceptación (Criterios de Salida)

### Prueba 1: Estadísticas de Supervivencia y Metabolismo
- **Ejecutable:** `build/test_survival_stats`
- **Condición:** Jugador con hambre y salud modificados. Verificación de consumo de alimentos, rechazo de comida a hambre máxima, agotamiento determinista y regeneración estándar vs rápida a 20 Hz de simulación.
- **Resultado:**
  ```text
  --- [F06 Test 1] Estadísticas de Supervivencia y Metabolismo ---
  Paso 1: Estadísticas iniciales canónicas verificadas (20 HP, 20 Hambre, 5 Saturación).
  Paso 2: Rechazo de consumo de comida con hambre al máximo verificado.
  Paso 3: Consumo de filete cocinado restaura 8 hambre y saturación exactos.
  Paso 4: Consumo de manzana restaura 4 hambre y 2.4 saturación.
  Paso 5: Conversión determinista de agotamiento (4.0) a saturación y hambre comprobada.
  Paso 6: Regeneración estándar a 20 Hz (1 HP cada 80 ticks) verificada con exactitud temporal.
  Paso 7: Regeneración rápida con saturación (1 HP cada 10 ticks) comprobada con éxito.
  >>> [F06 Test 1] APROBADO AL 100% <<<
  ```

### Prueba 2: Daño Ambiental, Peligros y Ventana de Invulnerabilidad
- **Ejecutable:** `build/test_damage_and_hazards`
- **Condición:** Caída de 7.6m (4 de daño), caídas seguras ($\le 3.0$m), ventana de 10 ticks de invulnerabilidad temporal frente a ráfagas de daño, daño por vacío ($Y < -96$), pérdida/recuperación de oxígeno y daño por inanición según dificultad.
- **Resultado:**
  ```text
  --- [F06 Test 2] Daño Ambiental, Peligros y Ventana de Invulnerabilidad ---
  Paso 1: Daño por caída calculado correctamente (7.6m -> 4 HP de daño, reset a 0).
  Paso 2: Caída segura (<= 3.0m) produce 0 daño comprobado.
  Paso 3: Ventana de invulnerabilidad temporal (10 ticks / 0.5s) verificada.
  Paso 4: Daño por vacío (Y < -96) continuo e implacable verificado.
  Paso 5: Consumo de oxígeno, daño por ahogamiento y recuperación rápida fuera del agua comprobados.
  Paso 6: Inanición según reglas de dificultad (Normal preserva 1 HP, Difícil elimina al jugador) verificada.
  >>> [F06 Test 2] APROBADO AL 100% <<<
  ```

### Prueba 3: Movimiento, Normalización y Prevención de Tunneling
- **Ejecutable:** `build/test_movement_and_collision`
- **Condición:** Comprobación de velocidades (caminar 4.3 m/s, sprint 5.6 m/s, agachado 1.3 m/s), límite de sprint por hambre ($\le 6$), normalización diagonal estricta, retención de caída en borde al agacharse, y subpasos de colisión a 15 FPS para evitar atravesar paredes sólidas de 1 bloque.
- **Resultado:**
  ```text
  --- [F06 Test 3] Movimiento, Normalización y Prevención de Tunneling ---
  Paso 1: Velocidad de caminata verificada en 4.3 m/s.
  Paso 2: Velocidad de sprint (5.6 m/s) y restricción con hambre <= 6 verificada.
  Paso 3: Velocidad de agachado verificada en 1.3 m/s.
  Paso 4: Normalización de entrada diagonal comprobada (magnitud exacta de 4.3 m/s sin exceso sqrt(2)).
  Paso 5: Protección de caída en borde al agacharse verificada.
  Paso 6: Prevención de atravesar sólidos (tunneling) mediante substepping a 15 FPS verificada.
  >>> [F06 Test 3] APROBADO AL 100% <<<
  ```

### Prueba 4: Ciclo de Muerte, Soltado de Drops y Reaparición Segura
- **Ejecutable:** `build/test_death_and_respawn`
- **Condición:** Jugador con 48 diamantes y espada sufre daño letal. Verificación de expulsión de entidades `ItemEntity`, inventario vacío, persistencia en disco durante la muerte sin clonación ni duplicación de recursos, y reaparición limpia en zona con aire y suelo.
- **Resultado:**
  ```text
  --- [F06 Test 4] Ciclo de Muerte, Soltado de Drops y Reaparición Segura ---
  Paso 1: Jugador configurado con 48 diamantes (32 en slot, 16 en cursor) y 1 espada.
  Paso 2: Jugador fallecido verificado (salud = 0, isAlive = false).
  Paso 3: Todos los objetos soltados como ItemEntity en posición de muerte. Inventario vaciado al 100%.
  Paso 4: Guardado durante transición de muerte y recarga probado: CERO duplicación de recursos.
  Paso 5: Búsqueda de spawn seguro evita quedar atrapado en roca sólida.
  Paso 6: Reaparición limpia con salud/hambre al máximo y en posición segura verificada.
  >>> [F06 Test 4] APROBADO AL 100% <<<
  ```

### Prueba 5: Camas, Ciclo de Sueño, Políticas de Modo y Esquema v5
- **Ejecutable:** `build/test_bed_and_gamemodes`
- **Condición:** Uso de cama de día y de noche (salto de tick a 24000), destrucción de cama y fallback automático a `worldSpawn`, inmunidad y reglas del Modo Creativo, persistencia Schema v5 con CRC32 y retrocompatibilidad con mundos de Schema v4.
- **Resultado:**
  ```text
  --- [F06 Test 5] Camas, Ciclo de Sueño, Políticas de Modo y Esquema v5 ---
  Paso 1: Interacción con cama de día fija punto de reaparición sin alterar el reloj.
  Paso 2: Dormir de noche adelanta el tiempo a la mañana (tick 24000).
  Paso 3: Cama destruida detectada al reaparecer: fallback seguro al spawn del mundo realizado.
  Paso 4: Políticas de Modo Creativo (inmunidad, rotura de Bedrock) y Supervivencia verificadas.
  Paso 5: Persistencia binaria del Esquema v5 verificada con exactitud.
  Paso 6: Retrocompatibilidad con mundos y jugadores de Esquema v4 confirmada.
  >>> [F06 Test 5] APROBADO AL 100% <<<
  ```

---

## 3. Estado de la Suite Completa de Pruebas (CTest)

```text
Test project <repo>/build
      Start  1: SimulationDeterminismTest
 1/27 Test  #1: SimulationDeterminismTest ..........   Passed    0.05 sec
      Start  2: BlockRegistryTest
 2/27 Test  #2: BlockRegistryTest ..................   Passed    0.01 sec
      Start  3: NetworkPacketSecurityTest
 3/27 Test  #3: NetworkPacketSecurityTest ..........   Passed    0.01 sec
      Start  4: GameCommandsAndChunkBordersTest
 4/27 Test  #4: GameCommandsAndChunkBordersTest ....   Passed    0.04 sec
      Start  5: PersistenceDeterminismTest
 5/27 Test  #5: PersistenceDeterminismTest .........   Passed    0.14 sec
      Start  6: TransactionRecoveryTest
 6/27 Test  #6: TransactionRecoveryTest ............   Passed    0.01 sec
      Start  7: CorruptionAndIntegrityTest
 7/27 Test  #7: CorruptionAndIntegrityTest .........   Passed    0.01 sec
      Start  8: SchemaMigrationAndBackupTest
 8/27 Test  #8: SchemaMigrationAndBackupTest .......   Passed    0.01 sec
      Start  9: WorldManagerAndLockTest
 9/27 Test  #9: WorldManagerAndLockTest ............   Passed    0.01 sec
      Start 10: GeneratorDeterminismTest
10/27 Test #10: GeneratorDeterminismTest ...........   Passed    0.03 sec
      Start 11: StreamingCacheAndMemoryTest
11/27 Test #11: StreamingCacheAndMemoryTest ........   Passed    0.15 sec
      Start 12: StreamingPersistenceTest
12/27 Test #12: StreamingPersistenceTest ...........   Passed    0.07 sec
      Start 13: ConcurrencyAndBoundariesTest
13/27 Test #13: ConcurrencyAndBoundariesTest .......   Passed    0.01 sec
      Start 14: FrustumCullingTest
14/27 Test #14: FrustumCullingTest .................   Passed    0.00 sec
      Start 15: InventoryConservationTest
15/27 Test #15: InventoryConservationTest ..........   Passed    0.00 sec
      Start 16: ItemEntityPickupTest
16/27 Test #16: ItemEntityPickupTest ...............   Passed    0.03 sec
      Start 17: ChestTransactionsTest
17/27 Test #17: ChestTransactionsTest ..............   Passed    0.01 sec
      Start 18: InventoryPersistenceTest
18/27 Test #18: InventoryPersistenceTest ...........   Passed    0.03 sec
      Start 19: ProductionChainTest
19/27 Test #19: ProductionChainTest ................   Passed    0.01 sec
      Start 20: RecipeCapacityAndBatchTest
20/27 Test #20: RecipeCapacityAndBatchTest .........   Passed    0.01 sec
      Start 21: FurnaceSimulationPersistenceTest
21/27 Test #21: FurnaceSimulationPersistenceTest ...   Passed    0.03 sec
      Start 22: ToolDurabilityAndMiningTest
22/27 Test #22: ToolDurabilityAndMiningTest ........   Passed    0.01 sec
      Start 23: SurvivalStatsTest
23/27 Test #23: SurvivalStatsTest ..................   Passed    0.01 sec
      Start 24: DamageAndHazardsTest
24/27 Test #24: DamageAndHazardsTest ...............   Passed    0.01 sec
      Start 25: MovementAndCollisionTest
25/27 Test #25: MovementAndCollisionTest ...........   Passed    0.01 sec
      Start 26: DeathAndRespawnTest
26/27 Test #26: DeathAndRespawnTest ................   Passed    0.01 sec
      Start 27: BedAndGameModesTest
27/27 Test #27: BedAndGameModesTest ................   Passed    0.01 sec

100% tests passed out of 27

Total Test time (real) =   0.74 sec
```

---

## 4. Verificación de la Puerta de Salida

- [x] **Todas las pruebas específicas de Fase 06 pasan y se conservan sus evidencias.** (5 ejecutables dedicados, 100% aprobados con validaciones estrictas).
- [x] **La regresión de fases anteriores (F01 a F05) sigue pasando.** (22/22 pruebas previas pasan limpiamente, 27/27 totales).
- [x] **Guardado/carga y límites de recursos se prueban con los nuevos datos.** (Esquema v5 con CRC32, backward compatibility con esquemas v1 a v4, serialización y validación de atributos de supervivencia).
- [x] **No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.** (Comprobada la cero clonación al morir y guardar/cargar durante la transición).
- [x] **Las diferencias con la referencia y el trabajo de catálogo restante están identificados.** (Transiciones visuales de día/noche en F07, IAs y mobs hostiles en F08, modos Espectador y Aventura en F13).

---

## 5. Diferencias con la Referencia y Trabajo Restante (Backlog)

1. **Iluminación Suave y Ciclo Visual Día/Noche (Fase 07):**
   - Aunque la simulación matemática del reloj de 24000 ticks y el salto nocturno con camas están implementados y activos a 20 Hz, la modulación cromática del cielo, la niebla atmosférica y la propagación de luz de bloques y cielo corresponden a la Fase 07.
2. **Entidades Hostiles y Proyectiles (Fase 08):**
   - Las reglas de daño recibido, retroceso (knockback), invulnerabilidad y muerte están listas en el motor. La IA de mobs agresivos (Zombies, Esqueletos, Acechador) y sus proyectiles se integran en la Fase 08.
3. **Modos de Juego Adicionales (Fase 13):**
   - Los modos base (Supervivencia y Creativo) están implementados y funcionales. Los modos Espectador (atravesar bloques sin colisión) y Aventura (minado restringido a herramientas con metadatos específicos) están programados para Fase 13.
