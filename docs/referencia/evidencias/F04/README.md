# Evidencias de Aceptación — Fase 04: Inventario, objetos y contenedores

**Fecha:** 15 de Octubre de 2026  
**Build:** `OpenCraft 0.1.0` (CMake C++20, GCC 15, Arch Linux / POSIX)  
**Suite de Pruebas:** 18/18 pruebas automáticas aprobadas al 100% (4 pruebas específicas de Fase 04 añadidas a CTest).

---

## 1. Resumen de Implementación

En la Fase 04 se implementó el sistema canónico de inventario, objetos y contenedores de OpenCraft, garantizando la conservación matemática estricta de recursos (sin duplicación ni pérdida accidental):

1. **Definiciones y Stacks de Ítems (`ItemDefinition`, `ItemStack`):**
   - Registro global desacoplado de bloques (`ItemRegistry`), con mapeo automático de bloques vanilla existentes a ítems colocables e ítems de recursos (diamantes, palos, lingotes) y herramientas con durabilidad (picos, espadas).
   - `ItemStack` con durabilidad, componentes binarios extensibles y operaciones atómicas (`split`, `merge`, `consume`, `damage`, `clamp`).
2. **Contenedor de Inventario (`Inventory`):**
   - Manejador de ranuras con control de versión monotónica (`revision`) para detección optimista de concurrencia.
   - Operaciones transaccionales libres de pérdida (`moveItem`, `swapSlots`, `splitSlot`, `addItem`, `removeItem`).
3. **Hotbar desacoplada del jugador (`PlayerEntity`):**
   - 36 ranuras totales (0..8 hotbar, 9..35 inventario principal) más ranura de cursor (`cursorItem`) y mano secundaria (`offhandItem`).
4. **Entidades de Objeto en el Mundo (`ItemEntity`):**
   - Física por ticks (gravedad, fricción en el suelo, colisión con AABB del terreno).
   - Despawn por tiempo de vida (máximo 6000 ticks / 5 minutos) y retardo de recogida configurable (`pickupDelayTicks`).
   - Recogida parcial estricta: cuando el jugador no tiene espacio suficiente, recoge hasta llenar sus ranuras y el remanente permanece en la entidad del mundo.
5. **Bloque-Entidad de Cofre (`ChestBlockEntity`):**
   - Bloque interactivo con 27 ranuras de almacenamiento persistente (`BlockID::Chest`).
   - Al romperse el bloque, todos los ítems contenidos se expulsan al mundo como entidades recogibles.
   - Control optimista de concurrencia mediante revisiones: si dos clientes actúan simultáneamente sobre la misma revisión, solo la primera transacción se confirma; la segunda se rechaza con `RevisionConflict`.
6. **Esquema de Persistencia v3 (`WorldFormat`):**
   - Versionado `SCHEMA_VERSION_3` con retrocompatibilidad completa para mundos de esquemas v1 y v2.
   - Serialización y deserialización protegida con checksum CRC32 del inventario completo, cursor, offhand y cofres ubicados en chunks.
   - Preservación segura del cursor: guardar o cerrar el juego nunca destruye los ítems sostenidos por el cursor.
7. **Interfaz Gráfica y HUD (`HUD`):**
   - Renderizado de la cuadrícula completa de 36 slots y cofre de 27 slots con número de stack, barras de durabilidad y tooltips.
   - Detección de clics (`getSlotAt`) unificada para ratón y toques táctiles.

---

## 2. Resultados de las Pruebas de Aceptación (Criterios de Salida)

### Prueba 1: Conservación estricta de recursos ante transacciones aleatorias
- **Ejecutable:** `build/test_inventory_conservation`
- **Condición:** Stack inicial de 64 diamantes y 1 espada con durabilidad. 1,500 transacciones aleatorias (`moveItem`, `swapSlots`, `splitSlot`, cancelaciones).
- **Resultado:**
  ```text
  --- [F04 Prueba 1] Conservación estricta de recursos en inventario ---
  Ejecutando 1500 transacciones aleatorias (move, swap, split, etc.)...
  Verificación exitosa: 1500 transacciones completadas sin pérdida ni duplicación.
  Diamantes finales: 64 (esperado 64)
  Espadas finales: 1 (esperado 1)
  [PASS] F04 Prueba 1 - Conservación de recursos completada con éxito.
  ```

### Prueba 2: Recogida parcial con inventario casi lleno y persistencia
- **Ejecutable:** `build/test_item_entity_pickup`
- **Condición:** Inventario con espacio para exactamente 3 unidades de piedra. Entidad de ítem en el suelo con 20 unidades de piedra.
- **Resultado:**
  - El jugador recoge 3 piedras (su ranura se llena al máximo de 64).
  - La entidad en el suelo permanece con las 17 piedras restantes.
  - Guardar el mundo a disco y recargarlo restaura exactamente las 64 piedras del jugador y las 17 piedras de la entidad en el mundo.
  ```text
  --- [F04 Prueba 2] Recogida parcial con inventario casi lleno y persistencia ---
  Paso 1 completado: Se recogieron 3 piedras, 17 permanecen en el mundo.
  Paso 2: Recargando mundo desde almacenamiento...
  [PASS] F04 Prueba 2 - Recogida parcial y persistencia completada con éxito.
  ```

### Prueba 3: Transacciones en cofre y verificación optimista de revisión
- **Ejecutable:** `build/test_chest_transactions`
- **Condición:** Cofre con revisión $R$. Dos comandos concurrentes intentan extraer 32 diamantes referenciando la revisión $R$.
- **Resultado:**
  - Comando 1: Confirmado (`Success`), revisión avanza a $R+1$.
  - Comando 2: Rechazado inmediatamente con `RevisionConflict`.
  - Reintento con revisión $R+1$: Confirmado exitosamente.
  ```text
  --- [F04 Prueba 3] Transacciones en cofre y verificación optimista de revisión ---
  Revisión inicial del contenedor: 0
  Nueva revisión del cofre tras primer comando: 2
  Resultado primer comando: Confirmado (Success)
  Resultado segundo comando: Rechazado por conflicto de versión (RevisionConflict)
  [PASS] F04 Prueba 3 - Verificación optimista de concurrencia completada con éxito.
  ```

### Prueba 4: Objeto en cursor, durabilidad y persistencia v3
- **Ejecutable:** `build/test_inventory_persistence`
- **Condición:** Jugador con 16 diamantes sostenidos en el cursor, pico de diamante con durabilidad 85, cofre con 45 lingotes de hierro. Guardar, salir y recargar mundo.
- **Resultado:**
  - La durabilidad se recupera exactamente en 85.
  - Los 16 diamantes del cursor se preservan íntegros sin pérdida ni duplicación.
  - Los 45 lingotes en el cofre se preservan íntegros.
  ```text
  --- [F04 Prueba 4] Objeto en cursor, durabilidad y persistencia v3 ---
  Guardando mundo con objeto en cursor, herramienta con durabilidad y cofre...
  Recargando mundo desde disco...
  Durabilidad recuperada con éxito: 85
  Objeto en cursor recuperado con éxito: 16 diamantes (en ranuras de inventario)
  Cofre recuperado con éxito: 45 lingotes de hierro en ranura 5
  [PASS] F04 Prueba 4 - Persistencia de cursor, durabilidad y cofre completada con éxito.
  ```

---

## 3. Salida de CTest Global

```text
Test project <repo>/build
      Start  1: SimulationDeterminismTest
 1/18 Test  #1: SimulationDeterminismTest .........   Passed    0.02 sec
      Start  2: BlockRegistryTest
 2/18 Test  #2: BlockRegistryTest .................   Passed    0.00 sec
      Start  3: NetworkPacketSecurityTest
 3/18 Test  #3: NetworkPacketSecurityTest .........   Passed    0.00 sec
      Start  4: GameCommandsAndChunkBordersTest
 4/18 Test  #4: GameCommandsAndChunkBordersTest ...   Passed    0.01 sec
      Start  5: PersistenceDeterminismTest
 5/18 Test  #5: PersistenceDeterminismTest ........   Passed    0.08 sec
      Start  6: TransactionRecoveryTest
 6/18 Test  #6: TransactionRecoveryTest ...........   Passed    0.01 sec
      Start  7: CorruptionAndIntegrityTest
 7/18 Test  #7: CorruptionAndIntegrityTest ........   Passed    0.01 sec
      Start  8: SchemaMigrationAndBackupTest
 8/18 Test  #8: SchemaMigrationAndBackupTest ......   Passed    0.01 sec
      Start  9: WorldManagerAndLockTest
 9/18 Test  #9: WorldManagerAndLockTest ...........   Passed    0.01 sec
      Start 10: GeneratorDeterminismTest
10/18 Test #10: GeneratorDeterminismTest ..........   Passed    0.04 sec
      Start 11: StreamingCacheAndMemoryTest
11/18 Test #11: StreamingCacheAndMemoryTest .......   Passed    0.14 sec
      Start 12: StreamingPersistenceTest
12/18 Test #12: StreamingPersistenceTest ..........   Passed    0.08 sec
      Start 13: ConcurrencyAndBoundariesTest
13/18 Test #13: ConcurrencyAndBoundariesTest ......   Passed    0.01 sec
      Start 14: FrustumCullingTest
14/18 Test #14: FrustumCullingTest ................   Passed    0.00 sec
      Start 15: InventoryConservationTest
15/18 Test #15: InventoryConservationTest .........   Passed    0.00 sec
      Start 16: ItemEntityPickupTest
16/18 Test #16: ItemEntityPickupTest ..............   Passed    0.03 sec
      Start 17: ChestTransactionsTest
17/18 Test #17: ChestTransactionsTest .............   Passed    0.01 sec
      Start 18: InventoryPersistenceTest
18/18 Test #18: InventoryPersistenceTest ..........   Passed    0.03 sec

100% tests passed out of 18

Total Test time (real) =   0.51 sec
```
