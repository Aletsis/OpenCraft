# Registro de Cambios (Changelog) — OpenCraft

Todas las modificaciones notables realizadas a través de las 16 fases de desarrollo de OpenCraft están documentadas en este archivo. El proyecto sigue las convenciones de [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/) y se adhiere a [Semantic Versioning](https://semver.org/).

---

## [Sin publicar]

### Añadido
- Windows x86_64 y ARM64: cliente, servidor y suite de pruebas, con paquete `.zip` que incluye ANGLE
  (OpenGL ES sobre Direct3D 11), SDL3 y las licencias de terceros. CI y release en runners de Windows.
- Opción `--gl auto|native|angle` (y `OPENCRAFT_GL`, `OPENCRAFT_ANGLE_DIR`) para elegir el backend de
  OpenGL ES; si ANGLE no puede crear el contexto se usa el controlador nativo.

### Cambiado
- OpenGL ES se carga en tiempo de ejecución a través de SDL en lugar de enlazar `libGLESv2`.
- La red usa una capa de sockets común para POSIX y Winsock.

### Corregido
- macOS se identificaba como iOS; los datos se guardan ahora en `~/Library/Application Support/OpenCraft`.
- Las copias de seguridad de mundos fallaban con «File exists» en algunas bibliotecas estándar.

## [1.0.0-rc1] - 2026-10-18

Primera versión candidata jugable de principio a fin en Linux de escritorio y primera versión publicada
del repositorio. Sustituye a una etiqueta interna anterior que la auditoría del 17 de octubre no
consideró publicable. El estado verificado y las limitaciones están en [docs/ESTADO_RC1.md](docs/ESTADO_RC1.md).

### Añadido
- Pantalla de título, selección, creación (nombre, semilla, supervivencia/creativo/extremo) y eliminación
  de mundos con copia de seguridad; pantallas de opciones, pausa, muerte y carga.
- HUD completo: barra rápida, corazones, hambre, armadura, oxígeno, experiencia, nombre del objeto en mano,
  panel de depuración (F3), ocultar interfaz (F1) y capturas PNG (F2).
- Inventario, mesa de trabajo, cofre, horno y paleta del modo creativo con transferencia rápida (Mayús+clic).
- Fuente de mapa de bits con acentos, ñ y signos de apertura; nombres de bloques y objetos en español.
- Atlas procedural completo (87 texturas de bloque y sprites propios de herramientas, armaduras, comida...).
- Render de criaturas, jugadores remotos y objetos en el suelo; contorno del bloque apuntado, grietas de
  minado y mano u objeto en primera persona.
- Iluminación real: luz del cielo y de bloques al generar/cargar, ciclo día/noche, luz suave y oclusión
  ambiental por vértice; niebla y color del cielo según la hora, el agua y la dimensión.
- Audio real con SDL3: efectos sintetizados con paneo 3D y volumen por categoría.
- Minado progresivo en supervivencia, ruptura instantánea en creativo, vuelo con doble salto, ataque a
  criaturas, salto automático opcional, soporte de mandos.
- Guiones de pruebas automáticas (`--script`) y opciones de línea de comandos (`--help`).
- Reglas `install()` de CMake con árbol reubicable, entrada de escritorio y metadatos AppStream.
- Opciones `OPENCRAFT_BUILD_CLIENT` y `OPENCRAFT_BUILD_TESTS`: el servidor y las pruebas compilan sin SDL3/GLES.
- Pruebas `TrialSpawnerSessionTickTest`, `ContentHashCanonicalTest` e `IncrementalSaveAndMobChunksTest`.
- Recetas de dispensador y raíl propulsor.
- Integración continua en GitHub Actions: formato con `clang-format` (versión fija), compilación y pruebas
  con GCC 14 y Clang 18 en x86_64 y ARM64, ASan + UBSan, cobertura mínima del 80 % y servidor en macOS
  (experimental). Las etiquetas `v*` publican paquetes Linux x86_64/ARM64 con SDL3 estática.
- Pruebas `AtlasMeshingAndTextTest`, `EffectsInputAndCommandsTest`, `SessionInventoryAndContainersTest`,
  `HoppersAndMachinesTest` y pruebas de humo del servidor dedicado (cobertura de líneas del 68 % al 83 %).
- Opción `--ticks <n>` del servidor dedicado para pruebas automáticas.

### Corregido
- La interfaz completa era invisible: el descarte de caras eliminaba todos los quads 2D.
- El mundo se veía en sombra: la luz nunca se calculaba y las secciones vacías devolvían luz 0.
- El jugador aparecía enterrado y la colisión lo desplazaba por el subsuelo; tocar una pared contaba como
  suelo/techo y los saltos hundían al jugador en el terreno.
- 60 tipos de bloque eran invisibles por falta de textura; varias texturas compartían índice por error.
- Clics más breves que un fotograma se perdían; la colocación enviaba el id del objeto como bloque.
- `PlaceBlockCommand` podía sobrescribir bloques existentes; ahora solo coloca en huecos.
- Escribir en zonas sin cargar creaba chunks vacíos que tapaban el terreno real.
- A05: el servidor aceptaba cualquier posición del cliente; el cliente aceptaba paquetes de cualquier remitente.
- A07: los generadores de desafío no avanzaban con el tick de la sesión.
- A09: los shaders solo se encontraban desde el directorio del proyecto; ahora junto al ejecutable,
  en la instalación o mediante `OPENCRAFT_ASSETS_DIR`. Los datos van a `XDG_DATA_HOME`.
- A12: el hash de red de los paquetes de contenido ignoraba ingredientes, formas y funciones.
- A13: escrituras con `fsync`, journal conservado si la recuperación falla, migración de las tres
  dimensiones sin perder criaturas y copia de seguridad antes de abrir un mundo de un esquema anterior.
- A15: el script de release ejecuta la suite en Debug antes de empaquetar y no fuerza `-DNDEBUG`; versión
  única desde CMake (`1.0.0-rc1`) en el binario, el paquete y Android.
- A11/A14/A16: manual reescrito, catálogo CSV válido con identificadores reales, enlaces rotos corregidos y
  retiradas las afirmaciones de paridad o estabilidad no respaldadas.
- Cada autoguardado reescribía todos los chunks cargados; ahora solo los modificados o con criaturas.
- Torbellino y Esqueleto del pantano no tenían IA; las criaturas se dibujaban mirando en una dirección equivocada.
- Las criaturas quedaban inmóviles: su colisión trataba el contacto con un bloque vecino como choque y
  no subían escalones de un bloque; los hostiles sin objetivo (p. ej. con el jugador en creativo) no se
  movían y la ruta A* se atascaba ante nodos a otra altura. Ahora pasean, saltan escalones, rodean muros
  y persiguen al jugador (`MobMovementStepsAndWanderTest`).
- `/tp` informaba de éxito sin mover a los jugadores remotos.
- `/summon` con una entidad desconocida invocaba un zombi y solo admitía cuatro tipos; ahora acepta todas las
  criaturas (con o sin espacio de nombres y con sus nombres anteriores) y rechaza las desconocidas.
- El stick izquierdo del mando movía al jugador hacia atrás al empujarlo hacia delante.
- `-DOPENCRAFT_BUILD_CLIENT=OFF` fallaba al configurar por unas reglas `install()` duplicadas.
- El efecto `wither` de comandos y paquetes anteriores se reconoce como Decaimiento.
- La simulación usaba `rand()`, cuya secuencia cambia según el sistema: la aparición de criaturas, su
  paseo y los objetos de las explosiones diferían entre Linux y macOS. Ahora usan un generador propio
  determinista (misma semilla, misma partida en cualquier plataforma).
- La regla `doMobSpawning` se aceptaba pero no tenía efecto.
- Los nombres de jugador se registraban con bytes basura en macOS; una prueba de red bloqueaba
  indefinidamente si el paquete por loopback no llegaba en el mismo instante.
- Pruebas que dependían de la carga de la máquina (`StreamingCacheAndMemoryTest`,
  `MobArchetypesAndPersistenceTest`) ahora son deterministas.

### Cambiado
- Nombres propios en lugar de términos de terceros: Acechador (rediseñado: criatura baja de piedra y
  musgo con seis patas y un ojo ámbar), cinabrio, el Abismo, el Confín, oricalco, Hidra ósea, Torbellino,
  Esqueleto del pantano, vara y polvo ígneos, entre otros. Los identificadores anteriores siguen
  funcionando como alias en mundos, comandos y paquetes (`IdAliasesTest`). Las clases, enumeraciones,
  funciones y archivos del código usan también los nombres nuevos.
- Licencia Apache 2.0 (titular: Alexis Barron) con archivos `LICENSE` y `NOTICE`; la documentación de
  referencia pasa a `docs/referencia/` y los textos describen el juego como proyecto independiente.
- Mundos, copias y capturas se guardan en `~/.local/share/opencraft/`; los mundos de `./saves` se copian
  automáticamente la primera vez.
- Los chunks generados que nunca se modifican no se guardan (se regeneran de forma determinista) y los
  desalojos se agrupan en una transacción.
- Supervivencia empieza con el inventario vacío.

### Limitaciones conocidas
- El cliente gráfico aún no puede unirse a partidas remotas (A04); Android e iOS no compilan todavía.

### Nota sobre las entradas anteriores
Las entradas 0.x describen el desarrollo por fases tal y como se declaró en su momento; la auditoría de
2026-10-17 encontró que varias de sus afirmaciones (pruebas de 8 horas, catálogo conciliado al 100 %) no
estaban respaldadas.

---

## [0.14.0] - (Fase 14: Optimización, Perfiles y Accesibilidad)
### Añadido
- Control táctil virtual adaptable con zona segura (notch/recorte de pantalla) en `TouchController`.
- Motor de audio espacial 3D con subtítulos accesibles e indicadores direccionales en `SpatialAudio`.
- Perfiles de rendimiento adaptativos (Bajo, Medio, Alto, Ultra) y benchmark automatizado de GPU/CPU.
- Sistema de localización multi-idioma (es_ES, en_US) con soporte para cadenas de texto externas.

---

## [0.13.0] - (Fase 13: Comandos Avanzados y Bloques de Comandos)
### Añadido
- Intérprete formal de comandos con selectores canónicos (`@p`, `@a`, `@r`, `@e`, `@s`).
- Soporte para bloques de comandos (Impulso, Cadena, Repetición) con condiciones de cinabrio.
- Ejecutor de funciones `.mcfunction` con presupuesto estricto de comandos por tick.

---

## [0.12.0] - (Fase 12: Paquetes de Contenido y Datos)
### Añadido
- Administrador de Content Packs y Data Packs (`ContentPackManager`) con verificación criptográfica de red.
- Árbol de avances canónicos y seguimiento de estadísticas generales del jugador.

---

## [0.11.0] - (Fase 11: Modos de Juego y Reglas del Mundo)
### Añadido
- Modos de juego Supervivencia, Creativo, Aventura, Espectador y Extremo (Hardcore) con borrado seguro de mundo al morir.
- Gestor de reglas de mundo (`GameRuleEngine`): `keepInventory`, `doDaylightCycle`, `mobGriefing`, etc.

---

## [0.10.0] - (Fase 10: Dimensiones y Jefes Finales)
### Añadido
- Dimensiones del Abismo y del Confín con generadores de terreno específicos.
- Portales de obsidiana y marcos de portal del Confín con detección de marcos y mapeo de coordenadas 8:1.
- Jefes Dragon del Confín (con cristales de curación) y Hidra ósea, junto con bloque-entidad de Faro (Beacon).

---

## [0.9.0] - (Fase 09: Automatización y Cinabrio Completa)
### Añadido
- Bloque `Crafter` para crafteo automático por pulsos de cinabrio.
- Tolvas (`Hopper`), Soltadores (`Dropper`), Dispensadores (`Dispenser`) y sistema de transporte por vagonetas y rieles.
- Pistones normales y pegajosos con empuje y tracción en cascada hasta 12 bloques.

---

## [0.8.0] - (Fase 08: Entidades Vivas y Criaturas)
### Añadido
- IA para criaturas pasivas (vacas, cerdos, ovejas, aldeanos) y hostiles (zombis, esqueletos, creepers, arañas).
- Sistema de cría de animales, comercio con aldeanos y reglas de desapariencia (`despawn`).

---

## [0.7.0] - (Fase 07: Audio, Partículas y Efectos de Estado)
### Añadido
- Sistema de pociones y alquimia (`BrewingStandBlockEntity`).
- Efectos de estado del jugador (Velocidad, Veneno, Regeneración, Fuerza, etc.).
- Motor de partículas procedimentales.

---

## [0.6.0] - (Fase 06: Ciclo de Vida y Agricultura)
### Añadido
- Crecimiento agrícola por etapas (trigo, zanahorias, patatas).
- Encantamientos mediante mesa de encantamientos y yunque con coste de niveles de experiencia.

---

## [0.5.0] - (Fase 05: Cinabrio Básica y Circuitos)
### Añadido
- Motor de cinabrio en grafo dirigido con propagación de señal hasta 15 niveles y repetidores.

---

## [0.4.0] - (Fase 04: Red y Multijugador Autoritativo)
### Añadido
- Servidor dedicado y cliente multijugador con replicación de snapshots comprimidos y predicción local de movimiento.

---

## [0.3.0] - (Fase 03: Generación Biómica y Estructuras)
### Añadido
- Generador biómico determinista con cuevas 3D (Simplex/Perlin noise) y estructuras procedimentales (árboles, mazmorras, templos).

---

## [0.2.0] - (Fase 02: Supervivencia, Crafteo y Fundición)
### Añadido
- Sistema de inventario con 36 ranuras, cofres y hornos de fundición interactivos.
- Tabla de crafteo 3x3 y libro de recetas básico.

---

## [0.1.0] - (Fase 01: Persistencia Transaccional y Chunks)
### Añadido
- Formato de guardado binario con journal atómico WAL, tolerancia a fallos y migración de esquemas.

---

## [0.0.1] - (Fase 00: Arquitectura Base y Simulación 20 TPS)
### Añadido
- Bucle de simulación a 20 TPS fijos con reloj determinista, registro global de bloques y canalización de comandos de juego.
