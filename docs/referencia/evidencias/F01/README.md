# Expediente de Evidencia — Fase 01: Núcleo de simulación y modelo de datos

- **Fecha:** 15 de octubre de 2026
- **Estado de Fase 01:** COMPLETADA
- **Compilación:** GCC 16 (C++20), CMake 3.20+, Linux x86_64
- **Dependencias de entrada:** F00 (Criterios y calidad)
- **Brechas abordadas:** B01 (Construcción y minería básica autoritativa), B08 (Preparación de estados de bloque), B10 (Contención de red e intenciones), B22 (Calidad, determinismo y CTest sin GPU).

---

## 1. Resumen de Implementación Realizada

1. **Biblioteca de Simulación Pura (`opencraft_sim`)**:
   - Totalmente desacoplada de SDL3 y OpenGL ES (`GLES3/gl3.h`).
   - Contiene el modelo de mundo espacial (`World`, `Chunk`), físicas de raycast DDA (`VoxelRaycast`), colisiones (`Math::AABB`), entidades (`PlayerEntity`, `EntityId`), registro de bloques y reloj de tick fijo.
   - Permite ejecución headless en CI y pruebas deterministas sin ventana ni GPU.

2. **Reloj de Simulación de 20 Hz Fijo e Interpolación Visual (`SimulationClock`, `Snapshot`)**:
   - `TICK_RATE = 20.0` (50 ms por tick lógico).
   - Acumulador de tiempo delta con límite de recuperación (*max accumulator clamp* a 250 ms = 5 ticks) para evitar congelamientos tras pausas o caídas de tasa de cuadros.
   - Cálculo del factor alfa de interpolación y `WorldSnapshot` con posiciones previas y actuales para renderizado visual suave a cualquier FPS (30, 60, 144+ FPS).

3. **Catálogo de Bloques Extensible con Namespaces y Estados (`BlockRegistry`)**:
   - Identificadores estables basados en cadenas (`opencraft:stone`, `opencraft:grass_block`, etc.).
   - Representación interna en `BlockStateId` (`uint16_t`), superando el límite de 8 bits (> 255 estados soportados).
   - Mapeo bidireccional estable entre nombres con namespace y estados.
   - Preservación de compatibilidad con `BlockID` legacy para atlas procedural y shaders.

4. **Comandos de Juego Autoritativos y Eventos Confirmados (`GameCommand`, `GameEvent`, `WorldSession`)**:
   - La entrada del usuario ya no muta el mundo directamente: emite intenciones como `MoveCommand`, `BreakBlockCommand`, `PlaceBlockCommand` y `SelectSlotCommand`.
   - `WorldSession` valida alcance ($\le 5.0$ bloques), colisión contra AABB del jugador, límites del mundo ($0 \le y < 64$) e inquebrantabilidad (Bedrock).
   - Las mutaciones válidas emiten un único evento confirmado `BlockChangedEvent` y marcan tanto el chunk modificado como los vecinos limítrofes como sucios.

5. **Saneamiento y Contención Estricta de Red (`PacketValidator`, `NetworkManager`)**:
   - Validación binaria previa a cualquier deserialización:
     - Verificación de tamaño exacto por tipo de paquete.
     - Saneamiento y terminación nula garantizada de nombres de jugador.
     - Verificación de números finitos (`std::isfinite`) en posiciones y rotaciones.
     - Verificación de límites de coordenadas en mutaciones de bloques.
   - Conmutador de seguridad: red LAN desactivada por defecto hasta F09. Cuando está activa, las mutaciones de red se enrutan mediante `BreakBlockCommand` y `PlaceBlockCommand` en la sesión autoritativa local.

6. **Controlador de Aplicación (`Application`)**:
   - Extracción completa del bucle principal de `main.cpp`.
   - `WorldRenderer` maneja las mallas GPU (`ChunkMesh`), descarte de caras ocluidas (*face culling*) y subida de búferes.

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
1/4 Test #1: SimulationDeterminismTest .........   Passed    0.01 sec
    Start 2: BlockRegistryTest
2/4 Test #2: BlockRegistryTest .................   Passed    0.00 sec
    Start 3: NetworkPacketSecurityTest
3/4 Test #3: NetworkPacketSecurityTest .........   Passed    0.01 sec
    Start 4: GameCommandsAndChunkBordersTest
4/4 Test #4: GameCommandsAndChunkBordersTest ...   Passed    0.01 sec

100% tests passed out of 4

Total Test time (real) = 0.03 sec
```

### Detalle de Casos de Prueba

| Prueba | Acción Ejecutada | Resultado Obtenido | Estado |
|---|---|---|---|
| **Determinismo (1.000 ticks)** | Reproducción de 1.000 ticks con renders simulados a 30, 60 y 144 FPS | Hash idéntico en las 3 ejecuciones: `0x66b2c80acc9d1279` | **APROBADO** |
| **Registro > 255 estados** | Registro de 312 estados, serialización de fixture lógica y reordenamiento inverso de IDs numéricos | Identidad de nombres con namespace preservada sin truncamiento | **APROBADO** |
| **Seguridad de paquetes** | Fuzzing de tamaños 0 a 1024 bytes, truncamientos forzados, NaNs/Infs y nombres desbordados | 100% de paquetes inválidos rechazados limpiamente sin fallos de segmentación ni lecturas fuera de límites | **APROBADO** |
| **Comandos y bordes de chunk** | Colocación y rotura en $lx=15$ y $lx=0$, validación de distancia $>5.0$, colisión con jugador y Bedrock | Emisión de exactamente 1 evento por acción válida, chunks vecinos marcados como sucios y rechazos seguros | **APROBADO** |

---

## 3. Puerta de Salida de Fase 01

- [x] Todas las pruebas específicas anteriores pasan y se conservan sus evidencias reproducibles.
- [x] La regresión de la base técnica no tiene fallos; la compilación es 100% limpia sin advertencias (`-Wall -Wextra -Wpedantic`).
- [x] No hay pérdida de datos, mutaciones arbitrarias de red ni bloqueadores en el alcance entregado.
- [x] Las interfaces preparan limpiamente la persistencia transaccional requerida para la Fase 02.
