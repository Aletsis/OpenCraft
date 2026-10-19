# Evidencias de Aceptación — Fase 05: Minería, fabricación y herramientas

**Fecha:** 15 de Octubre de 2026  
**Build:** `OpenCraft 0.1.0` (CMake C++20, GCC 15, Arch Linux / POSIX)  
**Suite de Pruebas:** 22/22 pruebas automáticas aprobadas al 100% (4 pruebas específicas de Fase 05 añadidas a CTest).

---

## 1. Resumen de Implementación

En la Fase 05 se implementó el primer ciclo económico y de progresión de OpenCraft (obtener recursos, transformarlos en estaciones de trabajo y mejorar herramientas), con estricta conservación de recursos:

1. **Jerarquía y Tipos de Herramientas (`ToolType`, `ToolTier`):**
   - Tipos: `None`, `Pickaxe`, `Axe`, `Shovel`, `Sword`, `Hoe`.
   - Niveles/Tiers de material: `None` (mano desarmada), `Wood`, `Stone`, `Iron`, `Diamond`, `Orichalcum`.
   - Durabilidades canónicas del juego de referencia implementadas: Madera (59 usos), Piedra (131 usos), Hierro (250 usos).
   - Eficiencias de minado (`miningEfficiency`) y tiempos de quemado (`fuelBurnTicks`) para herramientas y combustibles.

2. **Propiedades de Bloques y Reglas de Cosecha (`BlockRegistry`, `BlockProperties`):**
   - Incorporación de `hardness`, `requiredTool`, `minHarvestTier` y `dropItemName` a todos los bloques del catálogo.
   - Nuevos bloques registrados: `opencraft:crafting_table`, `opencraft:furnace`, `opencraft:coal_ore`, `opencraft:iron_ore`.
   - Regla de cosecha estricta: minar un bloque con nivel inferior al requerido (ej. minar `Stone` o `IronOre` con la mano desarmada `ToolTier::None`) destruye el bloque pero genera **cero botín**. Con la herramienta adecuada (`ToolTier::Wood` para piedra, `ToolTier::Stone` para hierro) se suelta el drop correspondiente (`Cobblestone`, `RawIron`, `Coal`).

3. **Motor de Recetas y Crafteo (`RecipeRegistry`, `ShapedRecipe`, `ShapelessRecipe`, `SmeltingRecipe`):**
   - Soporte para recetas con forma (shaped) 2×2 y 3×3 y sin forma (shapeless).
   - Coincidencia invariante ante traslación en rejilla y espejo horizontal (flip).
   - Fabricación en lote (`craftAll`): cálculo del multiplicador máximo según el ingrediente limitante, deducción exacta de ingredientes y producción acumulada sin desbordamiento ni cantidades negativas.
   - Comprobación preventiva de capacidad de salida: si el destino (cursor o inventario) está lleno o no tiene espacio suficiente, la transacción se rechaza limpiamente con consumo **cero** de ingredientes.

4. **Estaciones de Trabajo y Cuadrículas:**
   - Cuadrícula 2×2 en inventario personal del jugador (`OpenContainerKind::None`).
   - Mesa de trabajo 3×3 (`OpenContainerKind::CraftingTable`): abrir la mesa habilita la rejilla 3×3; al cerrarla, cualquier ingrediente remanente en la rejilla se devuelve automáticamente al inventario del jugador.

5. **Bloque-Entidad de Horno (`FurnaceBlockEntity`):**
   - 3 ranuras canónicas: Entrada (0), Combustible (1) y Salida (2).
   - Simulación determinista a 20 Hz por tick: consumo de combustible (`burnTimeRemaining`, `maxBurnTime`) y progreso de cocción (`cookProgressTicks`, `cookTotalTicks` = 200 ticks / 10s).
   - Nunca avanza en función de los FPS; pausado determinista al descargar chunks y reanudación exacta al cargar.
   - Recetas de fundición registradas: hierro crudo/mineral a lingote de hierro, carbón vegetal, cocción de alimentos de prueba (`raw_beef` a `cooked_beef`).

6. **Esquema de Persistencia v4 (`WorldFormat`):**
   - Actualización a `SCHEMA_VERSION_4` con retrocompatibilidad completa para versiones 1, 2 y 3.
   - Serialización de hornos (`FurnaceSaveData`) con slots, combustible y progreso de cocción protegidos con CRC32.

7. **HUD e Interfaz Gráfica (`HUD`, `Application`):**
   - Pantalla interactiva para Mesa de Trabajo 3×3 con ranura de salida y flecha de proceso.
   - Pantalla interactiva para Horno con ranuras de entrada, combustible y salida, animación de llama para combustión y flecha de progreso de cocción.
   - Integración fluida en ratón y pantalla táctil.

---

## 2. Resultados de las Pruebas de Aceptación (Criterios de Salida)

### Prueba 1: Cadena de Producción Completa (Madera -> Hierro)
- **Ejecutable:** `build/test_production_chain`
- **Condición:** Mundo nuevo e inventario vacío. Completar la progresión completa sin comandos tramposos: `wood_log -> planks -> crafting_table -> sticks -> wooden_pickaxe -> stone -> cobblestone -> furnace -> coal_ore -> stone_pickaxe -> iron_ore -> iron_ingots -> iron_pickaxe`.
- **Resultado:**
  ```text
  --- [F05 Prueba 1] Cadena de Produccion Completa (Madera -> Hierro) ---
  Paso 1: 3 troncos de madera recolectados.
  Paso 2: 4 tablones fabricados a partir de 1 tronco.
  Paso 3: Mesa de crafteo fabricada.
  Paso 4: 4 palos fabricados.
  Paso 5: Pico de madera fabricado y equipado.
  Paso 6: 8 adoquines (cobblestone) minados con pico de madera.
  Paso 7: Horno fabricado con 8 adoquines.
  Paso 8: 1 mineral de carbon minado con exito.
  Paso 9: Pico de piedra fabricado y equipado.
  Paso 10: 3 minerales de hierro crudo (raw iron) obtenidos.
  Fundiendo 3 lingotes de hierro en el horno (605 ticks = ~30s)...
  Paso 11: 3 lingotes de hierro fundidos exitosamente en el horno.
  Paso 12: Pico de hierro fabricado exitosamente.
  === Test 1 (Cadena de Produccion Completa) SUPERADO ===
  ```

### Prueba 2: Capacidad de Salida, Fabricación en Lote y Conservación Estricta
- **Ejecutable:** `build/test_recipe_capacity_and_batch`
- **Condición:** Recetas con material insuficiente, fabricación en lote asimétrica y solicitud de crafteo con inventario de destino lleno.
- **Resultado:**
  ```text
  --- [F05 Prueba 2] Capacidad de salida, fabricacion en lote y conservacion estricta ---
  1. Probando recetas con material insuficiente...
    -> Sin consumo con receta incompleta: OK
  2. Probando fabricacion en lote (batch craftAll)...
    -> 10 troncos -> 40 tablones (craftAll): OK
    -> Lote asimetrico min(7, 12) = 7 -> 28 palos, 5 sobrantes: OK
  3. Probando salida bloqueada (inventario destino lleno)...
    -> Bloqueo hacia cursor lleno: cero consumo y rechazo limpio: OK
    -> Bloqueo hacia inventario lleno: cero consumo y conservacion total: OK
    -> Tras liberar espacio, extraccion unica completada: OK
  === Test 2 (Capacidad de salida y lote) SUPERADO ===
  ```

### Prueba 3: Simulación de Horno a 20 Hz, Persistencia y Reanudación Exacta
- **Ejecutable:** `build/test_furnace_simulation_persistence`
- **Condición:** Horno a mitad de proceso de fundición (tick 100 de 200). Guardar mundo en Schema v4, cerrar sesión, recargar mundo y comprobar reanudación exacta y terminación de fundición sin duplicación.
- **Resultado:**
  ```text
  --- [F05 Prueba 3] Simulacion de Horno a 20 Hz, Persistencia y Reanudacion Exacta ---
  [INFO]  Mundo creado exitosamente: 'Test F05 Furnace' (ID: test_f05_furnace, Semilla: 777123)
  1. Simulando 100 ticks (5.0s a 20 Hz)...
  2. Guardando estado a disco en Schema v4 con CRC32...
  3. Reabriendo mundo y cargando datos guardados...
    -> Estado cargado con total fidelidad matematica.
  4. Completando 100 ticks restantes...
    -> Primer lingote obtenido sin duplicacion ni perdida de materiales.
  5. Fundiendo segundo lingote (200 ticks mas)...
    -> Cadena en horno concluida con 100% de conservacion.
  === Test 3 (Simulacion de Horno y Persistencia) SUPERADO ===
  ```

### Prueba 4: Durabilidad de Herramientas, Rotura y Nivel de Cosecha
- **Ejecutable:** `build/test_tool_durability_and_mining`
- **Condición:** Pico de madera con 1 uso restante (durabilidad 58/59). Minar bloque de piedra -> verificar rotura del pico y obtención de 1 adoquín. Petición repetida -> cero botín adicional. Minar piedra con la mano desarmada -> se rompe el bloque pero suelta 0 adoquines.
- **Resultado:**
  ```text
  --- [F05 Prueba 4] Durabilidad de Herramientas, Rotura y Nivel de Cosecha ---
  1. Pico de madera equipado con 1 uso restante (durabilidad 58/59)...
    -> Herramienta destruida al romperse: OK
    -> Exactamente 1 botin de adoquin generado: OK
    -> Peticion repetida no produce botin adicional: OK
  2. Minando piedra con la mano desarmada (ToolTier::None < ToolTier::Wood)...
    -> Minado desarmado sin botin (respetando minHarvestTier): OK
  === Test 4 (Durabilidad y Nivel de Cosecha) SUPERADO ===
  ```

---

## 3. Estado de la Suite Completa de Pruebas (CTest)

```text
Test project <repo>/build
      Start  1: SimulationDeterminismTest
 1/22 Test  #1: SimulationDeterminismTest ..........   Passed    0.02 sec
      Start  2: BlockRegistryTest
 2/22 Test  #2: BlockRegistryTest ..................   Passed    0.00 sec
      Start  3: NetworkPacketSecurityTest
 3/22 Test  #3: NetworkPacketSecurityTest ..........   Passed    0.00 sec
      Start  4: GameCommandsAndChunkBordersTest
 4/22 Test  #4: GameCommandsAndChunkBordersTest ....   Passed    0.01 sec
      Start  5: PersistenceDeterminismTest
 5/22 Test  #5: PersistenceDeterminismTest .........   Passed    0.07 sec
      Start  6: TransactionRecoveryTest
 6/22 Test  #6: TransactionRecoveryTest ............   Passed    0.01 sec
      Start  7: CorruptionAndIntegrityTest
 7/22 Test  #7: CorruptionAndIntegrityTest .........   Passed    0.01 sec
      Start  8: SchemaMigrationAndBackupTest
 8/22 Test  #8: SchemaMigrationAndBackupTest .......   Passed    0.01 sec
      Start  9: WorldManagerAndLockTest
 9/22 Test  #9: WorldManagerAndLockTest ............   Passed    0.00 sec
      Start 10: GeneratorDeterminismTest
10/22 Test #10: GeneratorDeterminismTest ...........   Passed    0.03 sec
      Start 11: StreamingCacheAndMemoryTest
11/22 Test #11: StreamingCacheAndMemoryTest ........   Passed    0.13 sec
      Start 12: StreamingPersistenceTest
12/22 Test #12: StreamingPersistenceTest ...........   Passed    0.06 sec
      Start 13: ConcurrencyAndBoundariesTest
13/22 Test #13: ConcurrencyAndBoundariesTest .......   Passed    0.00 sec
      Start 14: FrustumCullingTest
14/22 Test #14: FrustumCullingTest .................   Passed    0.00 sec
      Start 15: InventoryConservationTest
15/22 Test #15: InventoryConservationTest ..........   Passed    0.00 sec
      Start 16: ItemEntityPickupTest
16/22 Test #16: ItemEntityPickupTest ...............   Passed    0.03 sec
      Start 17: ChestTransactionsTest
17/22 Test #17: ChestTransactionsTest ..............   Passed    0.01 sec
      Start 18: InventoryPersistenceTest
18/22 Test #18: InventoryPersistenceTest ...........   Passed    0.03 sec
      Start 19: ProductionChainTest
19/22 Test #19: ProductionChainTest ................   Passed    0.01 sec
      Start 20: RecipeCapacityAndBatchTest
20/22 Test #20: RecipeCapacityAndBatchTest .........   Passed    0.01 sec
      Start 21: FurnaceSimulationPersistenceTest
21/22 Test #21: FurnaceSimulationPersistenceTest ...   Passed    0.03 sec
      Start 22: ToolDurabilityAndMiningTest
22/22 Test #22: ToolDurabilityAndMiningTest ........   Passed    0.01 sec

100% tests passed out of 22
```

---

## 4. Verificación de la Puerta de Salida

- [x] **Todas las pruebas específicas de Fase 05 pasan y se conservan sus evidencias.** (4 ejecutables dedicados, 100% aprobados).
- [x] **La regresión de fases anteriores (F01 a F04) sigue pasando.** (18/18 pruebas previas siguen pasando, 22/22 totales).
- [x] **Guardado/carga y límites de recursos se prueban con los nuevos datos.** (Schema v4 con CRC32 probado en `FurnaceSimulationPersistenceTest` y `InventoryPersistenceTest`).
- [x] **No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos.** (Conservación estricta verificada en todas las pruebas).
- [x] **Las diferencias con la referencia y el trabajo de catálogo restante están identificados.** (Sistemas avanzados de fundición y herrería para F12/F15 listados y documentados).
