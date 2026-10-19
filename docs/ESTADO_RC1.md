# Estado de la versión candidata 1.0.0-rc1

Fecha: 18 de octubre de 2026 · Etiqueta: `v1.0.0-rc1`

Este documento recoge **lo verificado**, el estado de cada hallazgo de la
auditoría del 17 de octubre de 2026 (informe interno `docs/auditoria/2026-10-17/REPORTE.md`, que no se publica en el repositorio) y las **limitaciones conocidas** con las
que se publica la versión candidata. Sustituye a las declaraciones de cierre de los expedientes por fase,
que se conservan como registro histórico.

## Alcance de la versión candidata

| Plataforma | Estado |
| :--- | :--- |
| Linux x86_64 (escritorio, X11/Wayland) | **Soportada y verificada** |
| Linux móvil (Phosh, Plasma Mobile) | Controles táctiles y escala de interfaz incluidos; sin verificar en un dispositivo |
| Linux ARM64 | Paquete generado y probado por CI (runner ARM64); sin verificar en hardware físico |
| macOS arm64 | Solo el servidor dedicado, experimental (el cliente necesita OpenGL ES 3) |
| Windows x86_64 / ARM64 | Paquete con ANGLE (Direct3D 11) generado y probado por CI en Windows; también verificado con compilación cruzada MinGW y Wine. Sin probar en hardware físico |
| Android / iOS | **No disponibles**: solo hay plantillas de manifiesto; falta el proyecto de compilación |

## Verificación realizada

- **Suite CTest**: 78/78 pruebas en Debug (aserciones activas), también en ejecución paralela, con GCC 14 y 16
  y con Clang 18. Con ASan y UBSan no se detectan errores de memoria ni comportamiento indefinido (los
  presupuestos de tiempo real se amplían con `OPENCRAFT_TIME_BUDGET_SCALE`); el cliente también se ejecutó
  con sanitizadores en el recorrido completo.
- **Cobertura de líneas de `src/`**: 82,8 % (umbral de CI: 80 %).
- **Integración continua** (GitHub Actions): formato, compilación y pruebas en x86_64 y ARM64, sanitizadores
  y cobertura en cada push; las etiquetas `v*` generan y publican los paquetes. El flujo completo se
  reprodujo en un contenedor Ubuntu 24.04 limpio (dependencias, SDL3 estática, cobertura y paquete).
- **Script de release** (`scripts/build_release_candidate.sh`): compilación Debug + CTest, compilación
  Release, instalación reubicable y prueba de humo del paquete instalado ejecutado desde otro directorio.
- **Recorridos automatizados con capturas** (`--script`, sin ventana con Mesa): título → crear mundo →
  carga → juego; F3; caminar y saltar; minar con grietas y recoger; colocar; inventario; paleta creativa;
  vuelo; noche; criaturas renderizadas; pausa → guardar y salir → lista de mundos; importación de un
  mundo de la versión anterior.
- **Audio**: mezcla real verificada con el controlador de disco de SDL (muestras no silenciosas).
- **Programas de diagnóstico de la auditoría** ejecutados de nuevo contra el código actual (tabla siguiente).

No se ha realizado: partida manual completa hasta el jefe final, sesión de 8 horas de servidor, cortes de
energía reales ni pruebas en ARM64, Android o iOS.

## Hallazgos de la auditoría

| Id | Hallazgo | Estado | Evidencia |
| :--- | :--- | :--- | :--- |
| A01 | Guardado de streaming que confirmaba éxito sin guardar | **Corregido** | `StreamingPersistenceTest`; diagnóstico: `reported_saved=0 dirty_after_failed_save=1` |
| A02 | Objetos tirados fuera de la transacción | **Corregido** | `EntitiesPersistenceAndTransactionTest`; diagnóstico: componentes 1 de 1 |
| A03 | Perfiles multijugador no persistidos | **Corregido** | `MultiplayerSessionAndHandshakeTest`; diagnóstico: jugador 42 restaurado |
| A04 | El cliente no recibe el mundo inicial | **Pendiente** | El cliente gráfico no puede unirse a partidas remotas; botones deshabilitados |
| A05 | Autoridad de movimiento y remitente | **Corregido** | Diagnóstico: teletransporte rechazado (`server_x=8`); el cliente filtra remitentes |
| A06 | Dimensiones y portales no integrados | **Corregido** | `DimensionTravelAndPersistenceTest` |
| A07 | Generador de desafío sin tick de sesión | **Corregido** | `TrialSpawnerSessionTickTest`; diagnóstico: `trial_cooldown=0` |
| A08 | Estrés y benchmarks sintéticos | **Parcial** | Las pruebas se describen como lo que son; no hay sesión real de 8 h ni benchmark en hardware |
| A09 | Recursos dependientes del directorio de ejecución | **Corregido** | Prueba de humo del paquete desde otro directorio |
| A10 | Plataformas móviles y audio | **Parcial** | Audio real (SDL3) y apertura de mandos; Android/iOS sin proyecto de compilación |
| A11 | Catálogo incompleto e incongruente | **Parcial** | CSV válido (10 columnas) e identificadores reales; sigue sin ser un inventario exhaustivo |
| A12 | Hash de paquetes de contenido incompleto | **Corregido** (hash) | `ContentHashCanonicalTest`; la negociación del hash en el handshake y la carga desde archivos siguen pendientes |
| A13 | Durabilidad y migración | **Corregido** | `fsync` de archivos y directorios, journal conservado si falla la recuperación, migración de las tres dimensiones, copia previa a abrir un mundo antiguo |
| A14 | Manual que no describía el ejecutable | **Corregido** | [MANUAL.md](MANUAL.md) reescrito sobre el programa actual |
| A15 | Distribución sin pruebas y con aserciones desactivadas | **Corregido** | El script ejecuta CTest en Debug antes de empaquetar; versión única desde CMake |
| A16 | Estados documentales incompatibles | **Corregido** | Este documento; los planes por fase se marcan como históricos; 0 enlaces locales rotos |

## Correcciones de jugabilidad incluidas en la RC

- Menú de título y gestión de mundos (crear, abrir, eliminar con copia), opciones persistentes, pausa,
  pantalla de muerte y de carga.
- El HUD y el inventario no se veían (la GPU descartaba toda la interfaz); ahora se muestran con fuente
  completa en español.
- El mundo se veía en sombra: la luz nunca se calculaba. Ahora hay luz de cielo, de bloques, ciclo
  día/noche, luz suave y oclusión ambiental.
- 60 tipos de bloque eran invisibles (texturas inexistentes en el atlas); el atlas está completo.
- El jugador aparecía enterrado y la colisión lo arrastraba por el subsuelo; al saltar junto a una pared
  se hundía en el suelo. Ambos problemas se corrigieron.
- Escribir en zonas no cargadas creaba chunks vacíos que tapaban el terreno real; ya no ocurre.
- Cada autoguardado reescribía todos los chunks cargados (unos 30 MB por guardado); ahora solo se
  escriben los modificados o con criaturas, y los generados sin cambios se regeneran desde la semilla.
- Torbellino y Esqueleto del pantano no tenían inteligencia artificial y se quedaban quietos; dispensador y raíl propulsor no
  tenían receta.
- Criaturas, objetos en el suelo, contorno de selección, grietas de minado y mano en primera persona.

## Nombres propios

Los bloques, objetos, criaturas y dimensiones con nombres de terceros se renombraron (Acechador,
cinabrio, el Abismo, el Confín, oricalco, Hidra ósea, Torbellino, Esqueleto del pantano...). Los mundos
guardados con los identificadores anteriores cargan sin cambios: la tabla de alias está en
`src/world/IdAliases.cpp`. Los símbolos internos del código también usan los nombres nuevos
(`DimensionType::Abyss`, `CinnabarEngine`, `BoneHydraEntity`...).

## Limitaciones conocidas

1. **Multijugador como cliente** (A04): no hay sincronización del mundo del anfitrión. *Multijugador* y
   *Abrir en LAN* aparecen deshabilitados. El servidor dedicado arranca, valida y guarda.
2. **Android e iOS** no se pueden compilar todavía.
3. **Sin consola de chat ni de comandos** en el cliente (el intérprete de comandos existe en la simulación).
4. **Sin vista en tercera persona**; los modelos de criaturas son cajas sencillas sin texturas propias.
5. **Paquetes de contenido** solo se cargan desde código (no hay cargador de archivos).
5b. **ANGLE en Linux** (`--gl angle`) es opcional y no se distribuye: con el ANGLE de vcpkg para Linux no
   se obtiene una pantalla EGL en X11, así que el juego vuelve al controlador nativo (comportamiento
   verificado). En Windows, ANGLE es el backend por defecto y se prueba en CI.
6. **Catálogo de paridad** no exhaustivo: no se declara un porcentaje de paridad con el juego de referencia.

## Siguientes pasos hacia 1.0.0

- Cliente multijugador: envío de chunks al conectar, movimiento simulado en el servidor, inventario remoto.
- Proyecto Android (Gradle + SDLActivity) y verificación en un dispositivo Linux móvil real.
- Sesión de servidor de varias horas con medición de memoria y tick, y análisis con ASan/UBSan.
- Partida manual completa de principio a fin en supervivencia.
