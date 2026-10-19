# Expediente de Evidencia — Fase 02: Mundos persistentes y recuperación

- **Fecha:** 15 de octubre de 2026
- **Estado de Fase 02:** COMPLETADA
- **Compilación:** GCC 16 (C++20), CMake 3.20+, Linux x86_64
- **Dependencias de entrada:** F01 (Núcleo y simulación)
- **Brechas abordadas:** B02 (Mundos persistentes y recuperación), B20 (Gestión de partidas, experiencia de usuario y seguridad de datos).

---

## 1. Resumen de Implementación Realizada

1. **Formato Binario OpenCraft Versionado (`WorldFormat`)**:
   - Cabeceras con identificación unívoca por tipo de archivo mediante *Magic Bytes*:
     - `OCMD` (`0x444D434F`): Metadatos del mundo (`world.meta`).
     - `OCPL` (`0x4C50434F`): Estado del jugador (`player.dat`).
     - `OCCK` (`0x4B43434F`): Chunks espaciales (`c.<x>.<z>.dat`).
     - `OCJR` (`0x524A434F`): Manifiesto transaccional (*journal*).
   - Formato desacoplado de structs crudos de C++, serializado en *little-endian* con límites estrictos de búfer (`MAX_FILE_SIZE = 16 MB`, `MAX_PALETTE_SIZE = 4096`).
   - Catálogo de bloques y paletas por nombres con namespace (`opencraft:stone`, etc.) para garantizar portabilidad entre versiones del juego.
   - Manejo seguro de contenido desconocido: bloques no reconocidos mapean automáticamente a `AIR_BLOCK_STATE` sin crasheos ni corrupción.

2. **Integridad Criptográfica con CRC32 IEEE 802.3**:
   - Cada archivo calcula y almacena una suma de comprobación CRC32 de su contenido.
   - Cualquier alteración de bits en disco es detectada en tiempo de carga, produciendo `StorageError::ChecksumMismatch` y evitando corrupciones silenciosas.

3. **Motor de Transacciones Atómicas en Dos Fases y Journaling (`WorldStorage`)**:
   - Los datos modificados se escriben a archivos temporales (`*.tmp`) con vaciado asegurado.
   - Se escribe el manifiesto de confirmación `journal.dat` con estado `COMMITTED`.
   - Los archivos temporales se promocionan atómicamente a su destino definitivo.
   - **Resistencia a caídas (*Crash-Proof*)**:
     - Si el proceso sufre una interrupción o corte de energía antes de confirmar (`PENDING`), al reiniciar se descartan los archivos temporales y se mantiene intacto el último estado coherente.
     - Si la interrupción ocurre tras escribir el commit (`COMMITTED`), al arrancar el journal completa automáticamente los renombres pendientes.
   - **Garantía estricta contra sobreescritura silenciosa**: Ante chunks corruptos o con checksum inválido, se informa el fallo y se preserva el archivo sin regenerar terreno sobre las construcciones del jugador.

4. **Gestor de Mundos y Exclusión Mutua (`WorldManager`)**:
   - Directorio raíz `saves/<world_id>/` y directorio de respaldos `backups/<world_id>_backup_<timestamp>/`.
   - Listado rápido de partidas (`listWorlds()`) extrayendo nombre, semilla, esquema, última fecha de juego y tamaño en disco.
   - Bloqueo de sesión activo (`session.lock`) mediante PID y marca de tiempo; previene que dos instancias de OpenCraft abran el mismo mundo simultáneamente, reconociendo procesos vivos y descartando bloqueos huérfanos tras caídas del sistema.
   - Migrador de esquemas (`migrateWorld`) que realiza una copia de seguridad obligatoria previa, actualiza versiones y garantiza idempotencia (migrar dos veces produce el mismo resultado estable).

5. **Autoguardado Periódico y Guardado al Salir**:
   - Temporizador de autoguardado configurable (30 s por defecto) en el bucle principal de `Application`.
   - Cola de chunks modificados: únicamente los chunks marcados como sucios (`isDirty()`) se serializan a disco, y su bandera se limpia únicamente tras la confirmación exitosa de la transacción.
   - Guardado automático asegurado al pulsar `ESC` o cerrar la ventana.
   - Soporte de argumentos de línea de comandos: `--list`, `--world <id>`, `--create <nombre> [--seed <semilla>]`, `--autosave <segundos>`.

---

## 2. Evidencia Reproducible de CTest

Comando de ejecución:
```bash
ctest --test-dir build --output-on-failure
```

Resultado obtenido:
```
Test project <repo>/build
    Start 1: SimulationDeterminismTest
1/9 Test #1: SimulationDeterminismTest .........   Passed    0.01 sec
    Start 2: BlockRegistryTest
2/9 Test #2: BlockRegistryTest .................   Passed    0.00 sec
    Start 3: NetworkPacketSecurityTest
3/9 Test #3: NetworkPacketSecurityTest .........   Passed    0.00 sec
    Start 4: GameCommandsAndChunkBordersTest
4/9 Test #4: GameCommandsAndChunkBordersTest ...   Passed    0.00 sec
    Start 5: PersistenceDeterminismTest
5/9 Test #5: PersistenceDeterminismTest ........   Passed    0.03 sec
    Start 6: TransactionRecoveryTest
6/9 Test #6: TransactionRecoveryTest ...........   Passed    0.00 sec
    Start 7: CorruptionAndIntegrityTest
7/9 Test #7: CorruptionAndIntegrityTest ........   Passed    0.00 sec
    Start 8: SchemaMigrationAndBackupTest
8/9 Test #8: SchemaMigrationAndBackupTest ......   Passed    0.01 sec
    Start 9: WorldManagerAndLockTest
9/9 Test #9: WorldManagerAndLockTest ...........   Passed    0.00 sec

100% tests passed out of 9

Total Test time (real) = 0.08 sec
```

### Detalle de Casos de Prueba de Persistencia (F02)

| Prueba | Acción Ejecutada | Resultado Exigido | Resultado Obtenido | Estado |
|---|---|---|---|---|
| **Persistencia determinista (120 ediciones en chunks +/-)** | 120 modificaciones en chunks positivos y negativos, cambios de posición/hotbar/semilla y recarga desde disco en sesión limpia | Coinciden bloques, posición, semilla y reglas; hash de estado FNV-1a idéntico | Coincidencia exacta de los 120 bloques, inventario y posición; hash idéntico antes y después | **APROBADO** |
| **Transacciones y recuperación ante caídas** | Inyección de interrupción con journal en estado `PENDING` y en estado `COMMITTED` antes de renombrar | Carga el estado anterior completo o promociona el nuevo, nunca una mezcla corrupta | `PENDING` descarta temporales y restaura estado previo; `COMMITTED` completa la promoción atómica | **APROBADO** |
| **Detección de corrupción y CRC32** | Mutación forzada de 1 byte en payload de chunk, alteración de número mágico y archivos truncados | Retorno de `ChecksumMismatch` e `InvalidMagic`; rechazo de regeneración silenciosa | Detección inmediata por CRC32, sin regeneración ni pérdida silenciosa de datos | **APROBADO** |
| **Migración de esquema y respaldos** | Migración de mundo con Esquema v1 a Esquema v2, y re-ejecución consecutiva de la migración | Generación de respaldo en `backups/`, datos íntegros y operación idempotente | Respaldo preservado en v1, datos activos en v2 y migración doble 100% idempotente | **APROBADO** |
| **Gestión de mundos y session.lock** | Apertura concurrente del mismo mundo por dos instancias independientes de `WorldManager` | La segunda instancia debe ser rechazada con `StorageError::Locked` | Apertura bloqueada limpiamente, liberación al cerrar y eliminación segura de mundos | **APROBADO** |

---

## 3. Puerta de Salida de Fase 02

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias reproducibles.
- [x] La regresión de las fases anteriores (F01) sigue pasando (100% de 9 pruebas exitosas).
- [x] La compilación es 100% limpia sin advertencias (`-Wall -Wextra -Wpedantic`).
- [x] Guardado, carga y límites de recursos se probaron exhaustivamente con datos reales de chunks y jugador.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] Las interfaces preparan limpiamente el streaming infinito y secciones verticales requeridas para la Fase 03.
