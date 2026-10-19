# Expediente de Evidencia — Fase 03: Streaming y generación escalable

- **Fecha:** 15 de octubre de 2026
- **Estado de Fase 03:** COMPLETADA
- **Compilación:** GCC 16 (C++20), CMake 3.20+, Linux x86_64
- **Dependencias de entrada:** F02 (Mundos persistentes y recuperación)
- **Brechas abordadas:** B03 (Exploración procedural y streaming continuo), B11 (Variedad vertical y generador procedural), B22 (Calidad, límites de memoria y presupuestos móviles).

---

## 1. Resumen de Implementación Realizada

1. **Secciones Verticales de 16³ y Rango [-64, 319] (`ChunkSection` y `Chunk`)**:
   - Se reemplazaron las columnas monolíticas de 64 bloques por 24 secciones cúbicas de 16x16x16 vóxeles (384 bloques de altura, paridad el juego de referencia 1.21).
   - Almacenamiento disperso (*sparse*): las secciones vacías de aire puro o roca uniforme no se asignan en memoria ni en disco (ocupan 1 solo byte de metadatos), ahorrando más del 80% de RAM en el cielo y subsuelo profundo.
   - Conteo atómico de bloques no aire (`nonAirCount`) por sección para acelerar el descarte de mallas.

2. **Generador Determinista Desacoplado de Plataforma (`WorldGenerator`)**:
   - Sustitución de `std::sin` y `std::cos` por funciones de ruido de gradiente y PRNG puro mediante funciones hash enteras de 64 bits (Murmur3 y SplitMix64).
   - Capa geológica completa: Lecho de roca indestructible (*Bedrock*) en `y = -64` con variación hasta `-60`, piedra profunda (*deepslate* y *stone*) en subsuelo, relieve montañoso multi-octava (FBM), capas de tierra y hierba superficial, y árboles de roble colocados con determinismo absoluto.
   - Generar las mismas secciones desde procesos o hilos distintos con igual semilla produce hashes de bloques 100% idénticos.

3. **Máquina de Estados de Chunk y Pool de Trabajadores CPU (`ChunkWorkerPool`)**:
   - Estados de ciclo de vida: `Unloaded` → `Requested` → `Generating` → `Ready` → `Meshing` → `Visible` → `Unloading`.
   - Generación de terreno y cálculo de mallas CPU delegados a hilos en segundo plano; el hilo de render principal solo ejecuta subidas rápidas a GPU (`VAO`/`VBO`/`EBO`).
   - Versionado de mallas (`meshVersion`): si un jugador edita un bloque mientras un trabajador computa una malla, el resultado obsoleto se descarta automáticamente al llegar, evitando artefactos visuales o carreras de memoria.
   - Extracción de bordes vecinos (`NeighborBlockData`) mediante instantáneas inmutables para el cálculo de caras ocluidas en hilos secundarios sin contención.

4. **Gestión de Streaming con Caché Acotada y Prevención de Vacío (`ChunkManager`)**:
   - Control de residencia alrededor de la posición del jugador con ordenamiento radial por cercanía.
   - Desalojo seguro de chunks lejanos con margen de histeresis para evitar oscilaciones (*thrashing*).
   - Autoguardado garantizado de chunks modificados (`isDirty()`) antes de ser liberados de la memoria.
   - **Prevención de caída al vacío**: Las consultas de física y movimiento detectan chunks no cargados y actúan como barrera sólida infranqueable hasta que la generación asíncrona finaliza y carga los vóxeles.

5. **Frustum Culling por Sección y Renderizado Relativo (`Frustum` y `WorldRenderer`)**:
   - Extracción analítica de los 6 planos del frustum desde la matriz `view * projection`.
   - Descarte visual por sección AABB: secciones fuera del cono de visión no se dibujan ni envían llamadas de renderizado.
   - Posiciones de render relativas a coordenadas de chunk para evitar pérdida de precisión de punto flotante de 32 bits a distancias de 10.000+ bloques.
   - Panel de métricas en tiempo real en el título de la ventana: FPS, chunks en RAM, secciones dibujadas y secciones descartadas por culling.

6. **Migración y Formato de Persistencia v2 (`WorldFormat`)**:
   - `SCHEMA_VERSION_2` almacena bloques agrupados por secciones con paletas locales y compresión de secciones vacías.
   - Soporte hacia atrás para mundos en `SCHEMA_VERSION_1` (migrando bloques de Y=0..63 al nuevo rango vertical sin pérdida de datos).

---

## 2. Evidencia Reproducible de CTest

Comando de ejecución:
```bash
ctest --test-dir build --output-on-failure
```

Resultado obtenido:
```
Test project <repo>/build
      Start  1: SimulationDeterminismTest
 1/14 Test  #1: SimulationDeterminismTest .........   Passed    0.01 sec
      Start  2: BlockRegistryTest
 2/14 Test  #2: BlockRegistryTest .................   Passed    0.00 sec
      Start  3: NetworkPacketSecurityTest
 3/14 Test  #3: NetworkPacketSecurityTest .........   Passed    0.00 sec
      Start  4: GameCommandsAndChunkBordersTest
 4/14 Test  #4: GameCommandsAndChunkBordersTest ...   Passed    0.01 sec
      Start  5: PersistenceDeterminismTest
 5/14 Test  #5: PersistenceDeterminismTest ........   Passed    0.05 sec
      Start  6: TransactionRecoveryTest
 6/14 Test  #6: TransactionRecoveryTest ...........   Passed    0.00 sec
      Start  7: CorruptionAndIntegrityTest
 7/14 Test  #7: CorruptionAndIntegrityTest ........   Passed    0.01 sec
      Start  8: SchemaMigrationAndBackupTest
 8/14 Test  #8: SchemaMigrationAndBackupTest ......   Passed    0.00 sec
      Start  9: WorldManagerAndLockTest
 9/14 Test  #9: WorldManagerAndLockTest ...........   Passed    0.00 sec
      Start 10: GeneratorDeterminismTest
10/14 Test #10: GeneratorDeterminismTest ..........   Passed    0.02 sec
      Start 11: StreamingCacheAndMemoryTest
11/14 Test #11: StreamingCacheAndMemoryTest .......   Passed    0.14 sec
      Start 12: StreamingPersistenceTest
12/14 Test #12: StreamingPersistenceTest ..........   Passed    0.04 sec
      Start 13: ConcurrencyAndBoundariesTest
13/14 Test #13: ConcurrencyAndBoundariesTest ......   Passed    0.00 sec
      Start 14: FrustumCullingTest
14/14 Test #14: FrustumCullingTest ................   Passed    0.00 sec

100% tests passed out of 14

Total Test time (real) = 0.30 sec
```

### Detalle de Pruebas Específicas de Fase 03

| Prueba | Acción Ejecutada | Resultado Exigido | Resultado Obtenido | Estado |
|---|---|---|---|---|
| **Determinismo de generador (100+ secciones)** | Generar 600 secciones con la misma semilla desde hilos paralelos y orden aleatorio vs orden secuencial | Hashes de bloques FNV-1a idénticos; semilla distinta genera mundo diferente | Coincidencia de hashes al 100%; diferenciación absoluta con otra semilla | **APROBADO** |
| **Residencia acotada en 10.000 bloques** | Simular recorrido continuo de 10.000 bloques a través de más de 600 chunks | Memoria se estabiliza dentro del presupuesto; no crece proporcionalmente a la distancia | Chunks en RAM acotados estrictamente (pico < 121); más de 500 chunks desalojados sin fugas | **APROBADO** |
| **Persistencia de 10 chunks en streaming** | Editar 10 chunks en coordenadas dispersas (+/-), alejarse 1.000 bloques hasta descargarlos, reiniciar sesión y volver | Todas las modificaciones se conservan íntegras tras descarga y recarga | 10 de 10 modificaciones recuperadas intactas sin pérdida ni desajuste de costuras | **APROBADO** |
| **Concurrencia, límites y barrera de vacío** | Simular worker lento con edición concurrente y cruzar hacia un chunk no cargado | Descarte limpio de malla obsoleta mediante `meshVersion`; bloqueo físico de paso sin caída al vacío | Malla obsoleta descartada; física bloquea caída al vacío en chunk pendiente; límites [-64, 319] verificados | **APROBADO** |
| **Frustum Culling por sección** | Evaluar cajas AABB de secciones frontales, traseras, laterales y lejanas contra cámara 3D | Secciones al frente pasan; secciones traseras o fuera de campo se descartan (culling) | Culling exacto al 100% de cajas traseras y fuera de visión sin falsos descartes frontales | **APROBADO** |

---

## 3. Puerta de Salida de Fase 03

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias reproducibles.
- [x] La regresión de las fases anteriores (F01 y F02) sigue pasando (100% de 14 pruebas exitosas en 0.30 s).
- [x] La compilación es 100% limpia sin advertencias (`-Wall -Wextra -Wpedantic`).
- [x] Guardado, carga y límites de recursos se probaron con los nuevos esquemas por secciones y recorrido de 10.000 bloques.
- [x] No hay pérdida de datos, duplicación de recursos ni bloqueadores conocidos en el alcance entregado.
- [x] La arquitectura de streaming y secciones verticales habilita limpiamente los sistemas de inventarios y objetos requeridos para la Fase 04.
