# OpenCraft — Documento de Desarrollo y Registro Técnico (DevLog)

Este documento es el registro vivo oficial del proyecto **OpenCraft**. Contiene la visión técnica, decisiones arquitectónicas, estado actual de cada subsistema, bitácora cronológica de cambios, errores/warnings detectados y sus soluciones.

---

## 1. Visión y Requerimientos Clave

- **Género**: Videojuego vóxel *sandbox* (de bloques) en 3D.
- **Plataformas Objetivo**:
  - **Linux Móvil** (Mobian, postmarketOS con Wayland/Phosh/Plasma Mobile en aarch64 y x86_64).
  - **Android** (Teléfonos y tablets con Android NDK, soporte táctil nativo).
  - **iOS** (iPhones / iPads, Metal / MoltenVK / GLES3).
  - **Linux Escritorio** (Entorno de desarrollo y pruebas).
- **Mecánicas Principales**:
  - Modo Supervivencia (físicas de gravedad, colisiones AABB, selección de bloques, estadísticas).
  - Modo Construcción (colocación y destrucción de bloques con algoritmo rápido DDA Raycast).
  - Multijugador en Red Local (LAN) sin necesidad de servidores externos (anuncio UDP broadcast + sockets UDP).
- **Rendimiento**:
  - 60 FPS estables en hardware móvil de gama baja/media (chips ARM con 2 a 4 GB RAM).
  - Consumo de memoria RAM inferior a 100 MB (el ejecutable base ocupa apenas **208 KB**).
  - Consumo mínimo de batería (sin recolección de basura / sin GC spikes).

> [!NOTE]
> Para un desglose técnico exhaustivo de cada módulo, estructuras de datos, algoritmos y justificaciones de diseño, consulta el documento dedicado: [ARCHITECTURE.md](ARCHITECTURE.md).

---

## 2. Decisiones Arquitectónicas y Tecnológicas

| Componente | Elección | Justificación |
| :--- | :--- | :--- |
| **Lenguaje** | **C++20** | Rendimiento determinista, sin *Garbage Collector*, control total del layout de memoria para vóxeles, compilación directa a binario nativo en Linux ARM64, Android NDK e iOS. |
| **Ventana y Entrada** | **SDL3** (`libSDL3`) | Soporte nativo y moderno para Wayland (Mobian/pmOS), soporte de gestos multitáctiles en pantalla (`SDL_EVENT_FINGER_*`), teclado/ratón y gamepad. |
| **API Gráfica** | **OpenGL ES 3.0** (`GLESv2` / `gl3.h`) | Máxima compatibilidad móvil (prácticamente el 100% de dispositivos Android, iOS vía MoltenVK/GLES y controladores Mesa Freedreno/Panfrost/V3D en Linux ARM). |
| **Matemáticas 3D** | Módulo propio `src/math/Math.hpp` (Vec2, Vec3, Mat4) | Elimina dependencias externas pesadas, diseño en orden columna (*column-major*) directo a uniformes GLSL ES 3.0. |
| **Texturas** | Atlas Procedural en memoria (`TextureAtlas`) | Genera en tiempo de ejecución las texturas de 16x16 píxeles con estilo pixel art para todos los bloques sin requerir dependencias de carga de imágenes externas. |
| **Generador de Malla** | Descarte de caras ocluidas (*Face Culling*) | Solo genera vértices para las caras en contacto con aire o bloques transparentes, reduciendo polígonos en más de un 75%. |
| **Física e Interacción** | AABB Sweep por ejes y DDA Raycasting | Deslizamiento suave sobre paredes y esquinas, y selección de bloques de hasta 5 unidades de alcance. |
| **Red Multijugador** | Descubrimiento UDP Broadcast + Protocolo UDP directo | Descubre partidas automáticamente en la misma red Wi-Fi en puerto `35566` y sincroniza bloques y jugadores en puerto `35565`. |

---

## 3. Estado Actual de Módulos

| Módulo | Estado | Descripción |
| :--- | :---: | :--- |
| **Configuración de Proyecto (CMake)** | 🟢 Completado | `CMakeLists.txt` con detección de SDL3, GLESv2, flags C++20 y copia automática de assets. |
| **Core (Ventana, Loop, Eventos, Log)** | 🟢 Completado | Ventana SDL3 con contexto OpenGL ES 3.0, sincronización vertical (VSync), High-DPI y registro con formato C++20. |
| **Matemáticas 3D (Vector, Matrix, Ray)** | 🟢 Completado | `Vec2`, `Vec3`, `Vec4`, `Mat4` (transformaciones, perspectiva, lookAt). |
| **Renderizador y Shaders** | 🟢 Completado | Shaders GLSL ES 3.0 (`voxel.vert`, `voxel.frag`), cálculo de iluminación direccional + AO y niebla atmosférica. |
| **Atlas de Texturas** | 🟢 Completado | Generador procedural en GPU de bloques (Hierba, Tierra, Piedra, Madera, Hojas, etc.). |
| **Sistema Vóxel (Chunks, Secciones, Malla)**| 🟢 Completado | Secciones cúbicas 16³ (24 secciones por chunk, rango Y=[-64, 319]), asignación dispersa (sparse), buffers VAO/VBO/EBO y mallado con descarte de caras ocluidas. |
| **Streaming y Generación Escalable** | 🟢 Completado | Generador determinista 64-bit multi-octava, pool asíncrono de hilos para generación y mallado, Frustum Culling por secciones, desalojo LRU con autoguardado y barrera de física. |
| **Física y Colisiones (AABB, Raycast)** | 🟢 Completado | DDA Raycast (Amanatides-Woo), colisiones AABB por ejes y barrera física contra caídas en chunks no cargados. |
| **Controles Móviles Táctiles y HUD** | 🟢 Completado | Retícula central, barra de acceso rápido (Hotbar) interactiva, joystick virtual táctil y arrastre de cámara. |
| **Persistencia y Recuperación de Mundos** | 🟢 Completado | Formato binario versionado (esquemas v1 a v7), transacciones atómicas con journal, CRC32, respaldos y `session.lock`. |
| **Inventario, Objetos y Contenedores** | 🟢 Completado | 36 slots, cofre 27 slots, entidades de items con físicas y tiempo de vida, durabilidad y transacciones atómicas con conservación matemática estricta. |
| **Minería, Fabricación y Herramientas** | 🟢 Completado | Tiers de herramientas (madera a oricalco), niveles de cosecha, recetas shaped/shapeless en lote, horno a 20 Hz determinista y persistente. |
| **Supervivencia, Movimiento y Reglas de Partida** | 🟢 Completado | Hambre, saturación, regeneración canónica, sprint/sneak, prevención de tunneling, daño ambiental, ciclo de muerte y reaparición segura, camas y modos de juego. |
| **Iluminación, Entorno y Fluidos** | 🟢 Completado | Motor de iluminación dual (SkyLight/BlockLight 0-15 en nibbles), propagación BFS, simulación hidráulica (agua y lava) con fuentes infinitas e interacciones térmicas, ciclo circadiano de 24,000 ticks, clima dinámico (lluvia, tormenta) y pipeline en 3 pases (opaco, cutout, translúcido). |
| **Criaturas, IA y Combate** | 🟢 Completado | Arquetipos de criaturas (Vaca, Zombie, Esqueleto, Acechador), IA con A* tridimensional (presupuesto 128 nodos), generador y despawn (`MobSpawner`) bajo topes de población (70 hostiles, 10 pasivos), combate físico con retroceso, armaduras canónicas y escudo al 100%, proyectiles balísticos con gravedad, y drops/XP atómicos. Persistencia v7. |
| **Multijugador Autoritativo y Servidor Dedicado** | 🟢 Completado | Servidor dedicado autoritativo headless (`opencraft_server`), protocolo binario `OCNT` v100 con serialización explícita, canal UDP fiable/ordenado con reensamblado y mitigación de degradación de red (100 ms RTT, 2% pérdida, duplicados, desorden), snapshots de mundo con jugadores remotos, validación de alcance $\le 5.0\text{ m}$, transacciones de cofres con control optimista de concurrencia y comandos de administración (`/op`, `/kick`, `/whitelist`). |
| **Audio Espacial 3D y Subtítulos** | 🟢 Completado | Motor de audio 3D con atenuación inversa cuadrática, paneo estéreo L/R orientado, 8 buses de mezcla categóricos, subtítulos visuales direccionales (`<`, `>`, `·`) y síntesis procedural de ondas de audio sin dependencias externas. |
| **Controles Multitáctiles, Gamepad y Safe Area** | 🟢 Completado | Joystick virtual multitáctil, acciones simultáneas independientes (saltar, sneak toggle, ataque, uso, inventario, pausa), hitboxes escaladas DPI, manejo de notch (*Safe Area Insets*), cancelación de dedos del SO (`SDL_EVENT_FINGER_CANCELED`) y navegación analógica por gamepad. |
| **Localización, Tutorial y Accesibilidad** | 🟢 Completado | Catálogo bilingüe de textos (`es_ES` y `en_US`), escalado UI adaptativo, opciones de alto contraste y reducción de sacudida de cámara, y tutorial interactivo guiado de 5 etapas para nuevos jugadores. |
| **Plataformas, Ciclo de Vida y Benchmarking** | 🟢 Completado | Resolución multiplataforma XDG (`PlatformPaths`), configuración por dispositivo (`DeviceSettings`), autoguardado forzado y recuperación ante suspensión/rotación/pérdida de contexto (20 ciclos superados), empaquetado Linux (`.desktop`, AppStream, script ARM64), andamiaje móvil Android/iOS y arnés de benchmarking con percentiles $p_{50}$, $p_{95}$, $p_{99}$. |

*Leyenda: 🟢 Completado | 🟡 En Progreso | ⚪ Pendiente | 🔴 Bloqueado*

---

## 4. Registro Cronológico de Actividades

### [2026-10-15] — Creación del Núcleo y Prototipo Funcional
- **Acción**: Análisis de viabilidad técnica para Mobian/postmarketOS, Android e iOS.
- **Decisión**: Se selecciona C++20 + SDL3 + OpenGL ES 3.0 como el stack óptimo por rendimiento, control de memoria y compatibilidad pura en Linux ARM y móviles.
- **Acción**: Creación del documento vivo `DEVELOPMENT_LOG.md`.
- **Acción**: Configuración del sistema de compilación `CMakeLists.txt`.
- **Implementación**:
  - `src/core/Log.hpp`: Sistema de logs con niveles y colores ANSI.
  - `src/math/Math.hpp`: Álgebra lineal 3D completa orientada a GLES3.
  - `src/core/Window.hpp` y `Window.cpp`: Inicialización de SDL3, ventana y contexto GLES3.
  - `src/render/Shader.hpp` y `Shader.cpp`: Carga y compilación de shaders GLSL ES 3.0 con gestión de errores.
  - `src/render/Camera.hpp`: Cámara en primera persona con vectores ortonormales y matrices de vista/proyección.
  - `src/render/TextureAtlas.hpp` y `TextureAtlas.cpp`: Generador procedural en memoria de texturas de bloques 16x16.
  - `src/world/Block.hpp`: Catálogo de bloques y propiedades físicas/ópticas.
  - `src/world/ChunkMesh.hpp` y `ChunkMesh.cpp`: Mallas de chunks con VAO, VBO y EBO en GPU.
  - `src/world/Chunk.hpp` y `Chunk.cpp`: Chunks de 16x64x16 con generador procedural de colinas y árboles.
  - `src/world/World.hpp` y `World.cpp`: Gestor de mapa de chunks y actualización dinámica.
  - `src/physics/Raycast.hpp` y `Raycast.cpp`: Algoritmo DDA para selección de vóxeles.
  - `src/game/Player.hpp` y `Player.cpp`: Físicas de gravedad, colisiones AABB y colocación/destrucción de bloques.
  - `src/ui/HUD.hpp` y `HUD.cpp`: Retícula central, hotbar renderizada en 2D y detección de toques.
  - `src/network/NetworkProtocol.hpp`: Estructuras binarias de paquetes multijugador.
  - `src/network/LANDiscovery.hpp` y `LANDiscovery.cpp`: Broadcast UDP en red local.
  - `src/network/NetworkManager.hpp` y `NetworkManager.cpp`: Protocolo cliente-servidor UDP no bloqueante.
- **Resultado de Compilación**: Binario `opencraft` generado con éxito (tamaño: **208 KB**, 0 advertencias, 0 errores).

### [2026-10-15] — Fase 02: Mundos Persistentes y Recuperación
- **Objetivo**: Abordar brechas B02 y B20 según la hoja de ruta de paridad del juego de referencia.
- **Implementación**:
  - `src/storage/WorldFormat.hpp` y `WorldFormat.cpp`: Formato binario con magic bytes (`OCMD`, `OCPL`, `OCCK`, `OCJR`), CRC32 IEEE 802.3, paletas dinámicas por namespaces y límites seguros de deserialización.
  - `src/storage/WorldStorage.hpp` y `WorldStorage.cpp`: Motor transaccional atómico de dos fases (`stage` -> `.tmp` -> `journal.dat` -> `rename`), recuperación automática de transacciones interrumpidas en arranque, y detección de corrupción.
  - `src/storage/WorldManager.hpp` y `WorldManager.cpp`: Gestor de mundos en `saves/`, listado estructurado, creación, copias de seguridad en `backups/`, migración de versiones idempotente y exclusión mutua de procesos mediante `session.lock`.
  - Integración en `World`, `Chunk`, `WorldSession` y `Application`: autoguardado periódico cada 30 segundos, guardado garantizado al salir (`ESC`), banderas sucias preservadas hasta confirmación física, y parámetros CLI (`--list`, `--world`, `--create`, `--seed`, `--autosave`).
- **Pruebas Automatizadas (CTest)**:
  - 5 nuevas suites integradas (`test_persistence`, `test_transactions`, `test_corruption`, `test_migration`, `test_world_manager`).
  - 9 de 9 pruebas de CTest pasando al 100% en 0.08 segundos.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F02/README.md`.

### [2026-10-15] — Fase 03: Streaming y Generación Escalable
- **Objetivo**: Abordar brechas B01 (Límite vertical de mundo moderno Java 1.21), B03 (Streaming infinito y generación procedural desacoplada), B04 (Concurrencia multihilo sin condición de carrera en fronteras) y B08 (Optimización de renderizado por frustum culling).
- **Implementación**:
  - `src/world/ChunkSection.hpp` y `ChunkSection.cpp`: Secciones cúbicas de 16x16x16 con conteo de bloques no-aire (`nonAirCount`), asignación perezosa/dispersa (secciones completamente de aire ocupan 0 bytes de vóxeles) y descarte inmediato de mallado si está vacía.
  - `src/world/Chunk.hpp` y `Chunk.cpp`: Chunks verticales de 24 secciones (384 bloques de altura, rango [-64, 319]), máquina de estados de ciclo de vida (`Unloaded`, `Generating`, `Generated`, `Meshing`, `Ready`, `Unloading`), y versionado atómico de mallas (`meshVersion`) para descarte seguro de mallas obsoletas.
  - `src/world/WorldGenerator.hpp` y `WorldGenerator.cpp`: Generador determinista multiplataforma basado en hashes de 64 bits (SplitMix64 / Lehmer64) y ruido de gradiente FBM multi-octava sin dependencias trigonométricas de punto flotante de libc; genera capas de bedrock en Y=-64, deepstone, piedra, vetas subterráneas de carbón/hierro, tierra superficial, césped y árboles cilíndricos.
  - `src/world/ChunkWorkerPool.hpp` y `ChunkWorkerPool.cpp`: Pool de hilos de trabajo en segundo plano con colas de prioridad separadas para generación y mallado, snapshots de datos de vecindad de 6 caras (`NeighborBlockData`) libres de condiciones de carrera y paso de mallas preparadas en CPU hacia el hilo principal de render.
  - `src/world/ChunkManager.hpp` y `ChunkManager.cpp`: Gestor de streaming radial por distancia euclidiana/Chebyshev con histéresis de descarga para evitar oscilaciones (*thrashing*), desalojo LRU acotado a memoria máxima (`maxLoadedChunks`), autoguardado previo al desalojo de chunks sucios, y barrera de física que previene caídas al vacío en límites no cargados.
  - `src/render/Frustum.hpp` y `Frustum.cpp`: Extracción Gribb-Hartmann de los 6 planos del frustum desde la matriz `viewProj` y test de intersección con AABBs de secciones en espacio de mundo.
  - `src/render/WorldRenderer.hpp` y `WorldRenderer.cpp`: Renderizado optimizado sección por sección con frustum culling y traslaciones relativas a la cámara para evitar pérdida de precisión de punto flotante a distancias lejanas.
  - `src/storage/WorldFormat.hpp` y `WorldFormat.cpp`: Esquema binario `SCHEMA_VERSION_2` con serialización por secciones comprimibles (bandera de sección vacía de 1 byte) y compatibilidad retroactiva transparente para cargar mundos `SCHEMA_VERSION_1`.
  - Integración en `Application` y `main.cpp`: Título de ventana dinámico mostrando FPS, chunks/secciones cargadas, y mallas renderizadas vs descartadas por frustum culling.
- **Pruebas Automatizadas (CTest)**:
  - 5 nuevas suites especializadas: `test_generator_determinism`, `test_streaming_cache`, `test_streaming_persistence`, `test_concurrency_boundaries` y `test_frustum_culling`.
  - 14 de 14 pruebas de CTest pasando al 100% en 0.30 segundos.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F03/README.md`.

### [2026-10-15] — Fase 04: Inventario, Objetos y Contenedores
- **Objetivo**: Abordar brechas B05, B16 y B20 (Conservación estricta de inventarios, entidades flotantes y cofres).
- **Implementación**:
  - `src/item/Item.hpp` y `Item.cpp`: Registro de ítems desacoplado de bloques, `ItemStack` con durabilidad y operaciones atómicas.
  - `src/inventory/Inventory.hpp` y `Inventory.cpp`: 36 ranuras de jugador, control de versiones optimista y transacciones sin clonación ni pérdida.
  - `src/sim/Entity.hpp`: Entidades `ItemEntity` con físicas por tick, rebote, fricción, retardo de recogida y vida útil de 6000 ticks.
  - `src/world/ChestBlockEntity.hpp` y `ChestBlockEntity.cpp`: Cofres con 27 ranuras persistentes y expulsión de botín al destruirse.
  - `src/storage/WorldFormat.hpp` y `WorldFormat.cpp`: Esquema binario `SCHEMA_VERSION_3` con serialización protegida por CRC32.
  - `src/ui/HUD.hpp` y `HUD.cpp`: Interfaz visual interactiva de inventario y cofres para teclado/ratón y pantalla táctil.
- **Pruebas Automatizadas (CTest)**:
  - 4 suites especializadas: `test_inventory_conservation`, `test_item_entity_pickup`, `test_chest_transactions` y `test_inventory_persistence`.
  - 18 de 18 pruebas de CTest pasando al 100% en 0.40 segundos.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F04/README.md`.

### [2026-10-15] — Fase 05: Minería, Fabricación y Herramientas
- **Objetivo**: Abordar brechas B05, B07 y B18 (Árbol de herramientas, niveles de cosecha y recetas).
- **Implementación**:
  - `src/item/Item.hpp` y `Item.cpp`: Tipos de herramientas (`ToolType`), niveles de material (`ToolTier`), eficiencias y durabilidades canónicas.
  - `src/world/Block.hpp` y `BlockRegistry.cpp`: Propiedades de cosecha de bloques (`hardness`, `minHarvestTier`, `requiredTool`). Minar sin el tier adecuado rompe el bloque con cero botín.
  - `src/crafting/RecipeRegistry.hpp` y `RecipeRegistry.cpp`: Motor de recetas shaped (2x2 y 3x3) y shapeless con matching espacial y fabricación en lote (`craftAll`) con conservación total.
  - `src/world/FurnaceBlockEntity.hpp` y `FurnaceBlockEntity.cpp`: Bloque-entidad de horno con simulación a 20 Hz por tick, slots de fundición y progreso determinista.
  - `src/storage/WorldFormat.hpp` y `WorldFormat.cpp`: Esquema binario `SCHEMA_VERSION_4` para persistencia de hornos con CRC32 y retrocompatibilidad v1-v3.
  - `src/ui/HUD.hpp` y `HUD.cpp`: Pantallas interactivas de Mesa de Trabajo (3x3) y Horno con animación de llama y progreso.
- **Pruebas Automatizadas (CTest)**:
  - 4 suites especializadas: `test_production_chain`, `test_recipe_capacity_and_batch`, `test_furnace_simulation_persistence` y `test_tool_durability_and_mining`.
  - 22 de 22 pruebas de CTest pasando al 100% en 0.50 segundos.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F05/README.md`.

### [2026-10-15] — Fase 06: Supervivencia, Movimiento y Reglas de Partida
- **Objetivo**: Abordar brechas B06, B15 y B18 (Metabolismo de hambre/saturación, substepping de colisión, invulnerabilidad temporal, ciclo de muerte y reaparición segura, camas y modos de juego).
- **Implementación**:
  - `src/sim/Entity.hpp`: Atributos de supervivencia en `PlayerEntity` (salud 20, hambre 20, saturación 5, agotamiento, aire 300, tiempo de invulnerabilidad `hurtTime`, modos de juego).
  - `src/sim/WorldSession.hpp` y `WorldSession.cpp`: Simulación a 20 Hz de digestión, regeneración canónica estándar y rápida, consumo de alimentos, subpasos de colisión continuos evitando atravesar sólidos a 15-144 FPS, protección de bordes al agacharse, normalización de velocidad diagonal, daño ambiental (caída, vacío, asfixia, ahogamiento), ciclo de muerte con expulsión de drops como `ItemEntity`, algoritmo `findSafeSpawn`, e interacción diurna/nocturna con camas.
  - `src/game/Player.cpp`: Normalización de entrada direccional y estados de sprint/sneak.
  - `src/ui/HUD.hpp` y `HUD.cpp`: Corazones de vida, muslos de comida, burbujas de oxígeno y pantalla roja de muerte interactiva con botón de reaparición.
  - `src/storage/WorldFormat.hpp` y `WorldFormat.cpp`: Esquema binario `SCHEMA_VERSION_5` con persistencia de estadísticas de supervivencia y retrocompatibilidad total v1 a v4.
- **Pruebas Automatizadas (CTest)**:
  - 5 suites especializadas: `test_survival_stats`, `test_damage_and_hazards`, `test_movement_and_collision`, `test_death_and_respawn` y `test_bed_and_gamemodes`.
  - 27 de 27 pruebas de CTest pasando al 100% en 0.74 segundos.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F06/README.md`.

### [2026-10-15] — Fase 07: Luz, Tiempo, Clima y Simulación Ambiental
- **Objetivo**: Abordar brechas B07 y B08 (Propagación BFS de luz dual, simulación de fluidos con fuentes infinitas e interacciones térmicas, ciclo circadiano, clima dinámico y pipeline en 3 pases).
- **Implementación**:
  - Motor de iluminación dual: `SkyLight` y `BlockLight` (0–15 empaquetado en nibbles, 4096 bytes/sección) con propagación y despropagación BFS, vecindades inter-sección y oscuridad en chunks no cargados.
  - Colas deterministas de actualizaciones programadas (`ScheduledUpdateQueue`) a 20 Hz con límite estricto de 500 updates/tick.
  - Simulación hidráulica: agua y lava con propagación lateral/vertical, creación de fuentes infinitas y reacciones térmicas canónicas (obsidiana, adoquín, piedra).
  - Peligros ambientales: flotabilidad y amortiguación de caída en agua, daño/combustión en lava, extinción en agua y gravedad de arena.
  - Ciclo día/noche de 24,000 ticks con insolación suave y clima (Clear, Rain, Thunder) reseteable con camas.
  - Pipeline de renderizado en 3 pases: opaco, cutout y translúcido con alpha blending y sombreado por luz de cielo, bloque y AO.
  - Persistencia bajo Esquema v6 con preservación de nibbles de luz, cola de updates y clima.
- **Pruebas Automatizadas (CTest)**:
  - 5 suites especializadas: `LightingAndPropagationTest`, `ScheduledUpdatesAndWeatherTest`, `FluidFlowAndInteractionTest`, `ImmersionAndEnvironmentalHazardsTest`, `RenderPassesAndAoTest`.
  - 32 de 32 pruebas pasando al 100% en 0.85 segundos.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F07/README.md`.

### [2026-10-15] — Fase 08: Criaturas, IA y Combate (H3: Primera Noche)
- **Objetivo**: Abordar brecha B09 y completar el hito H3 de paridad el juego de referencia.
- **Implementación**:
  - Arquetipos de criaturas: `Cow` (pasiva, pánico ante daño), `Zombie` (hostil melee, combustión diurna), `Skeleton` (hostil ranged, disparo de flechas balísticas), `Lurker` (hostil explosivo, mecha de 30 ticks con griefing destructivo de bloques).
  - IA con máquina de estados y navegación A* tridimensional con presupuesto estricto de 128 expansiones e invalidación reactiva por cambios en el terreno.
  - Generador y despawn (`MobSpawner`) regido por niveles de luz, condiciones de suelo, topes de población (70 hostiles, 10 pasivas) y radio de despawn instantáneo > 128 bloques.
  - Combate físico: fórmula de mitigación por armaduras (`Daño * (1 - armadura * 0.04)`), absorción al 100% con escudo frontal, retroceso direccional y proyectiles balísticos con gravedad y arrastre (`ProjectileEntity`).
  - Orbes de experiencia magnéticos (`ExperienceOrbEntity`) y botín atómico al morir.
  - Persistencia bajo Esquema v7 preservando criaturas por chunk (`mobs.dat`), armaduras y XP del jugador.
- **Pruebas Automatizadas (CTest)**:
  - 4 suites especializadas: `MobArchetypesAndPersistenceTest`, `PathfindingAndNavigationTest`, `MobSpawningAndLightRulesTest`, `CombatDamageAndLootTransactionTest`.
  - 36 de 36 pruebas pasando al 100% en 1.10 segundos.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F08/README.md`.

### [2026-10-16] — Fase 09: Multijugador Autoritativo y Servidor Dedicado (H4: Cooperación)
- **Objetivo**: Abordar brecha B10 y completar el hito H4 de paridad con el juego de referencia canónico.
- **Implementación**:
  - `src/network/NetworkSerializer.hpp`: Serializador `ByteWriter` y deserializador `ByteReader` con validación estricta de límites (buffer overruns), tipos numéricos canónicos y verificación de flotantes (`std::isfinite`).
  - `src/network/NetworkProtocol.hpp`: Protocolo binario canónico con Magic bytes `0x4F434E54` (`OCNT`), versión de protocolo `PROTOCOL_VERSION = 100`, IDs canónicos de mensaje y serialización explícita para `Handshake`, `HandshakeAck`, `HandshakeReject`, `PlayerActionMsg`, `InventoryTransactionMsg`, `ChatMessageMsg`, `ReliableAckMsg`, `EntitySpawnMsg`, `EntityDestroyMsg`, `EntityUpdateMsg`.
  - `src/network/PacketValidator.hpp` y `PacketValidator.cpp`: Validación robusta de paquetes `OCNT` binarios y retrocompatibilidad con structs empaquetados legacy.
  - `src/network/ReliableChannel.hpp` y `ReliableChannel.cpp`: Capa de transporte fiable y ordenada sobre UDP con números de secuencia monotónicos, ventana de reensamblado para paquetes desordenados, descarte de duplicados, retransmisiones automáticas por expiración de temporizador y simulador de degradación de red (`NetworkDegradationSimulator`).
  - `src/sim/Snapshot.hpp`: Estructura `RemotePlayerSnapshot` integrada en `WorldSnapshot` para réplica de jugadores remotos con orientación (yaw, pitch), salud e ítem en mano.
  - `src/sim/WorldSession.hpp` y `WorldSession.cpp`: Sesión multijugador con mapa de jugadores concurrentes (`m_connectedPlayers`), validación autoritativa de alcance geométrico ($\le 5.0\text{ m}$), validación de transacciones de cofres con control optimista de concurrencia (`expectedRevision`), combate PvP autoritativo con mitigación canónica de armaduras y bloqueo frontal con escudo al 100%, y hash determinista de estado multijugador.
  - `src/network/NetworkServer.hpp` y `NetworkServer.cpp`: Servidor UDP autoritativo no bloqueante, negociación de handshake, timeout por inactividad (10 s), comandos de administración (`/kick`, `/op`, `/whitelist`), y réplica periódica de snapshots y entidades activas.
  - `src/network/NetworkClient.hpp` y `NetworkClient.cpp`: Cliente UDP no bloqueante, negociación fiable de conexión, envío de acciones de bloque y transacciones de inventario, recepción de snapshots y réplica local de entidades remotas.
  - `src/network/NetworkManager.hpp` y `NetworkManager.cpp`: Orquestador unificado de red cliente/servidor para la aplicación gráfica.
  - `src/server_main.cpp`: Binario servidor dedicado headless (`opencraft_server`) con soporte de flags CLI (`--port`, `--world`, `--seed`, `--max-players`, `--whitelist`, `--save-interval`), manejo de señales `SIGINT`/`SIGTERM`, loop de simulación a 20 Hz fija y autoguardado periódico.
- **Pruebas Automatizadas (CTest)**:
  - 4 suites especializadas:
    - `test_multiplayer_session_and_handshake`: Handshake canónico, rechazo por versión incompatible, timeout de inactividad, reconexión tardía y deduplicación.
    - `test_authoritative_actions_and_reach`: Validación autoritativa de alcance ($\le 5.0\text{ m}$), control optimista de concurrencia en cofres (sin duplicación ni pérdida), y combate PvP autoritativo.
    - `test_snapshot_and_chunk_replication`: Difusión de snapshots, jugadores remotos y replicación de entidades (Item, Mob, Projectile) a múltiples clientes concurrentes.
    - `test_degraded_network_and_convergence`: Transporte fiable bajo 100 ms RTT, 2% pérdida, duplicados y desorden, con convergencia determinista idéntica de hashes de estado.
  - 40 de 40 pruebas de CTest pasando al 100% en 1.69 segundos.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F09/README.md`.

### [2026-10-16] — Fase 10: Biomas, Estructuras, Flora/Fauna, Comercio y Navegación (H5: Mundo Productivo)
- **Objetivo**: Abordar brecha B11 y completar el hito H5 de paridad con el juego de referencia canónico.
- **Implementación**:
  - Motor de biomas 2D/3D con temperatura, humedad, continentalidad y erosión determinista a lo largo de 100 semillas distintas.
  - Generación de estructuras multicelda deterministas (aldeas, pozos de mina, templos) con colocación de cofres de botín.
  - Agricultura y flora: cultivos (trigo, zanahorias, patatas) con 8 etapas de crecimiento, tasa acelerada por hidratación y harina de huesos (`bone_meal`).
  - Fauna y domesticación: lobos domesticables con modo sentado/seguimiento, cría de ganado bovino en celo (`inLove`) con trigo, producción de crías (`isBaby`) y orbes de experiencia.
  - Aldeanos y economía: asignación de profesiones (granjero, librero, armero), ofertas comerciales estructuradas con costos primarios/secundarios, stock limitado por oferta, bloqueo atómico al agotarse y reposición diaria en estación de trabajo.
  - Navegación e instrumentos: orientación de brújula magnética orientada al spawn mundial y simulación hidrodinámica de barcos (`BoatEntity`) con flotabilidad canónica en agua.
- **Pruebas Automatizadas (CTest)**:
  - 4 suites especializadas: `WorldGenerationBiomesAnd100SeedsTest`, `StructuresAndMultiChunkDeterminismTest`, `AgricultureGrowthAndRenewableCycleTest`, `VillagerEconomyAndConcurrentTradeTest`.
  - 44 de 44 pruebas de CTest pasando al 100%.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F10/README.md`.

### [2026-10-17] — Fase 11: Cinabrio, Automatización y Mecanismos Complejos (H6: Automatización e Industria)
- **Objetivo**: Abordar brecha B12 y completar el hito H6 de paridad con el juego de referencia canónico.
- **Implementación**:
  - Cinabrio analógica y discreta: simulación de cableado de cinabrio con atenuación de potencia (15 a 0), fuentes (antorchas, repetidores con retardos de 1 a 4 ticks, comparadores con modo sustracción y lectura de nivel de llenado de contenedores).
  - Pistones normales y pegajosos (`PistonBlockEntity`): extensión, contracción, límite de empuje de 12 bloques y adherencia con bloques de slime.
  - Tolvas (`HopperBlockEntity`), Soltadores (`DropperBlockEntity`), Dispensadores (`DispenserBlockEntity`) y Fabricador automático (`CrafterBlockEntity`): transferencia atómica de ítems a 2.5 items/seg (8 ticks por transferencia), bloqueo por cinabrio y fabricación automatizada orientada a recetas.
  - Transporte ferroviario: raíles normales, raíles propulsores activados por cinabrio, raíles detectores y entidades de vagoneta (`MinecartEntity`) con físicas de rodadura y aceleración sobre pendiente.
- **Pruebas Automatizadas (CTest)**:
  - 4 suites especializadas: `CinnabarLogicAndTimingTest`, `PistonsAndBlockMovementTest`, `HoppersDroppersAndCrafterTest`, `RailsAndMinecartTransportTest`.
  - 48 de 48 pruebas de CTest pasando al 100%.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F11/README.md`.

### [2026-10-17] — Fase 12: Progresión, Encantamientos, Alquimia, Dimensiones, Jefes y Balizas (H7: Trascendencia)
- **Objetivo**: Abordar brechas B13, B14 y B19, culminando la implementación completa de la simulación canónica del juego de referencia 1.21.
- **Implementación**:
  - Progresión y encantamientos: mesa de encantamientos con coste de lapislázuli y niveles de XP, biblioteca con cálculo de estanterías adyacentes, yunque con reparación y coste acumulativo de penalización previa.
  - Alquimia y pociones: soporte de soporte de pociones (`BrewingStandBlockEntity`) con combustible de polvo de ígneo, elaboración en 3 botellas, y sistema de efectos de estado (`StatusEffectManager`: Velocidad, Fuerza, Regeneración, Resistencia, Veneno, Hidra ósea).
  - Dimensiones y portales: soporte multicapa de dimensiones (`Overworld`, `Abyss`, `Rim`), portales de obsidiana con búsqueda de marco y transferencia de coordenadas (factor 8:1 en el Abismo), portales del Confín con generación de plataforma de obsidiana en (100, 48, 0).
  - Jefes y balizas: Dragón del Confín (`RimDragonEntity`) con cristales del Confín y fases de combate (Perching, Circling), Hidra ósea (`BoneHydraEntity`) con proyectiles de calavera explosivos y aura de escudo a mitad de vida, y Bloque de Baliza (`BeaconBlockEntity`) con detección de base piramidal de 1 a 4 niveles y radiación de efectos de área.
  - Esquema binario `SCHEMA_VERSION_10`: persistencia total de dimensiones, bloques-entidad de alquimia y balizas, entidades de jefe y retrocompatibilidad v1 a v9 sin migraciones destructivas.
- **Pruebas Automatizadas (CTest)**:
  - 4 suites especializadas: `EnchantingAnvilAndProgressionGraphTest`, `AlchemyPotionsAndEffectsTest`, `DimensionsPortalsAndStorageTest`, `BossesDragonHydraAndBeaconTest`.
  - 52 de 52 pruebas de CTest pasando al 100%.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F12/README.md`.

### [2026-10-17] — Fase 13: Modos Completos, Comandos y Contenido Extensible (H6: Progresión Completa)
- **Objetivo**: Abordar brechas B14, B18 y B19, integrando modos de juego canónicos (Supervivencia, Creativo, Aventura, Espectador), Hardcore permadeath, motor de comandos con selectores, bloques de comandos y scripts `.mcfunction` con límite de recursión y presupuesto de instrucciones, datapacks con hash determinista de red, logros y estadísticas.
- **Implementación**:
  - Modos de juego: Creativo (vuelo, invulnerabilidad general, minado instantáneo), Aventura (restricciones de minado y colocación), Espectador (noclip completo a través de bloques sólidos en `resolveCollisions`, vuelo 3D, invulnerabilidad absoluta e incapacidad de interactuar con el entorno).
  - Modo Hardcore: Permadeath permanente; al morir, el jugador transiciona a Espectador de forma irreversible, persistida en disco.
  - Motor de comandos (`CommandEngine`): Intérprete autoritativo con selectores `@p`, `@a`, `@s`, `@e`, coordenadas relativas `~`, comandos `/gamemode`, `/time`, `/weather`, `/tp`, `/give`, `/summon`, `/setblock`, `/fill` con límite de seguridad $\le 32768$ bloques, `/gamerule`, `/effect`, `/function`.
  - Paquetes de contenido (`ContentPackManager`): Manifiestos, validación de referencias rotas, hash determinista CRC32 de sincronización de red para prevenir desincronizaciones cliente-servidor.
  - Logros y estadísticas (`AdvancementManager`, `StatisticsManager`): Grafo jerárquico de logros con disparadores automáticos (`onPlayerJoin`, `onBlockMined`, `onItemCrafted`, `onMobKilled`, `onDimensionChanged`) y seguimiento de estadísticas (`mined`, `crafted`, `used`, `general`, `deaths`).
  - Esquema binario `SCHEMA_VERSION_11`: Serialización de estado Hardcore, reglas de juego activas, logros desbloqueados y estadísticas persistentes, con compatibilidad v1–v10 intacta.
- **Pruebas Automatizadas (CTest)**:
  - 4 suites especializadas: `GameModesAndHardcorePersistenceTest`, `CommandParserAndSelectorsTest`, `CommandBlocksAndFunctionBudgetTest`, `ContentPacksAndAdvancementsTest`.
  - 56 de 56 pruebas de CTest pasando al 100%.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F13/README.md`.

### [2026-10-18] — Fase 14: Experiencia Audiovisual, Accesibilidad y Plataformas (H7: Habilitación de Plataformas)
- **Objetivo**: Abordar brechas B20, B21 y B22, integrando resolución de rutas multiplataforma XDG, configuración persistente por dispositivo, controles multitáctiles con Safe Area, soporte de gamepad, audio espacial 3D con paneo sinusoidal y 8 buses, subtítulos accesibles direccionales, localización bilingüe `es_ES`/`en_US`, tutorial interactivo guiado, ciclo de vida móvil con autoguardado forzado y recuperación ante pérdida de contexto (20 ciclos superados), empaquetado para escritorio y móvil Linux, andamiaje Android/iOS y arnés de benchmarking con percentiles $p_{50}$, $p_{95}$, $p_{99}$.
- **Implementación**:
  - Rutas y configuración (`PlatformPaths`, `DeviceSettings`): Soporte XDG en Linux (`~/.local/share/opencraft`, `~/.config/opencraft`), Android, iOS y modo portable; serialización atómica de opciones gráficas, audio, controles y accesibilidad sin alterar archivos del mundo.
  - Controles móviles y gamepad (`TouchController`, `GamepadController`): Joystick virtual multitáctil con deadzone y sprint, 6 botones virtuales independientes (Jump, Sneak toggle, Attack, Use, Inventory, Pause), insets para Safe Area / muescas, manejo de `SDL_EVENT_FINGER_CANCELED` y controles analógicos de mando.
  - Audio espacial 3D y accesibilidad (`SpatialAudio`, `AudioSubtitles`): Modelo de atenuación inversa cuadrática, paneo estéreo L/R, 8 mezcladores por categoría, subtítulos visuales con indicador direccional (`<`, `>`, `·`), y síntesis procedural de ondas de audio documentada en `assets/audio/PROVENANCE.md`.
  - Interfaz, localización y tutorial (`LocalizationManager`, `TutorialSystem`, `HUD`): Catálogo bilingüe de cadenas con fallback, escalado dinámico entero de UI, máquina de estados de 5 fases para onboarding y renderizado con matriz `uOrtho`.
  - Integración y empaquetado (`assets/packaging/`, `platforms/`): `opencraft.desktop`, `opencraft.metainfo.xml`, icono oficial 64x64, script de empaquetado `package_arm64.sh`, estructura Gradle Android NDK y manifiesto `Info.plist` de iOS.
  - Ciclo de vida y recuperación (`LifecycleManager`): Manejo de eventos en segundo plano en SDL3, autoguardado atómico forzado al suspender, y prueba de resistencia de 20 ciclos de suspensión/reanudación y rotación.
  - Benchmarking y perfiles (`QualitySettings`, `BenchmarkHarness`): Perfiles Low, Medium, High, Ultra, métricas de latencia de fotograma y tick ($p_{50}$, $p_{95}$, $p_{99}$) y validación de 20 TPS ($\le 50.0\text{ ms}$).
- **Pruebas Automatizadas (CTest)**:
  - 5 suites especializadas: `DeviceSettingsAndLocalizationTest`, `TouchControllerAndSafeAreaTest`, `SpatialAudioAndSubtitlesTest`, `LifecycleAndBackgroundRecoveryTest`, `BenchmarkAndQualityProfilesTest`.
  - 61 de 61 pruebas de CTest pasando al 100% (100.0%) con cero regresiones en fases 01 a 13.
- **Evidencia**: Expediente publicado en `docs/referencia/evidencias/F14/README.md`.

---

## 5. Registro de Errores, Warnings y Soluciones

| ID | Fecha | Subsistema | Descripción del Error / Warning | Causa Raíz | Solución Aplicada |
| :---: | :---: | :---: | :--- | :--- | :--- |
| **ERR-001** | 2026-10-15 | Sandbox de Ejecución | `connecting to sandbox server: connection reset by peer` | El socket del sandbox se desconectó al consultar herramientas del sistema. | Se invocaron las comprobaciones con permisos directos verificando que `cmake`, `g++`, `sdl3` y `glesv2` estaban instalados. |
| **ERR-002** | 2026-10-15 | `TextureAtlas.cpp` | `error: ‘sqrt’ is not a member of ‘std’` | Falta de la cabecera `<cmath>` en el archivo de implementación. | Se añadió `#include <cmath>` y `#include <algorithm>`. |
| **ERR-003** | 2026-10-15 | `NetworkManager.hpp` | `error: ‘LANDiscovery’ no nombra a un tipo` | `LANDiscovery` fue declarada dentro del espacio de nombres `OpenCraft::Net`, mientras que `NetworkManager` reside en `OpenCraft`. | Se utilizó el tipo calificado `Net::LANDiscovery` en `NetworkManager.hpp`. |
| **ERR-004** | 2026-10-15 | `World.cpp` | `error: ‘class OpenCraft::Chunk’ has no member named ‘isEmpty’` | El método de verificación de malla vacía `isEmpty()` existía en `ChunkMesh` pero no estaba expuesto en `Chunk`. | Se añadió el método *inline* `bool isEmpty() const noexcept { return m_mesh.isEmpty(); }` en `Chunk.hpp`. |
| **ERR-005** | 2026-10-15 | `ChunkMesh.hpp` | Dependencia cíclica/incompatibilidad de tipos GLES en tests headless | `ChunkMesh.hpp` incluía directamente `<GLES3/gl3.h>`, impidiendo compilar suites de prueba puras en entornos headless sin dependencias de GPU. | Se desacopló la cabecera reemplazando `GLuint` con `uint32_t` y encapsulando las llamadas OpenGL exclusivamente en `ChunkMesh.cpp`. |
| **ERR-006** | 2026-10-16 | `NetworkServer.cpp` | `error: ‘struct OpenCraft::ItemEntity’ has no member named ‘isAlive’` | `ItemEntity` utiliza el campo canónico `isDead` para rastrear ciclo de vida. | Se adaptó la serialización a `!e.isDead`. |
| **ERR-007** | 2026-10-16 | `server_main.cpp` | `error: no matching function for call to ‘openWorld’` | `WorldManager::openWorld` requiere la firma `(const std::string&, StorageError&)`. | Se instanció la variable `StorageError outError` y se pasó a la llamada. |
| **ERR-008** | 2026-10-16 | `test_multiplayer_session_and_handshake.cpp` | `error: ‘inet_pton’ was not declared in this scope; ‘close’ was not declared` | Faltaban las cabeceras estándar de sockets POSIX `<arpa/inet.h>` y `<unistd.h>`. | Se añadieron las cabeceras requeridas condicionalmente para entornos Linux. |
| **ERR-009** | 2026-10-17 | `MobEntity.cpp` | `test_pathfinding_and_navigation` devolvía camino incompleto si se excedía el límite de nodos | Presupuesto A* por defecto de 128 nodos insuficiente para recorridos con obstáculos y retorno sin comprobar `targetReached`. | Se incrementó el presupuesto por defecto a 256 nodos en `MobEntity.hpp` y se garantizó la verificación de alcance de meta antes de confirmar el camino. |
| **ERR-010** | 2026-10-17 | `MobSpawner.cpp` | `test_mob_spawning_and_light` fallaba en `canSpawnAt` | `canSpawnAt` comprobaba exclusivamente `stateAtPos.isSolid` asumiendo coordenadas de suelo y no pies de entidad. | Se admitió validación tanto de posición de pies (con suelo sólido debajo) como de suelo, y se limpiaron entidades muertas del vector en `tick`. |
| **ERR-011** | 2026-10-17 | `WorldSession.cpp` | `test_combat_and_loot_transactions` fallaba por omisión de impacto de proyectiles rápidos | Avance discreto de proyectiles balísticos saltaba la AABB de la entidad objetivo antes de tocar el suelo. | Se implementó interpolación AABB swept continua (`expectedNext = oldPos + proj.velocity * tickDt`) garantizando intersección precisa de hitboxes a altas velocidades. |
| **ERR-012** | 2026-10-17 | `WorldSession.cpp` | `test_authoritative_actions_and_reach` fallaba al ejecutar comandos de inventario de jugadores remotos | `resolveInventory` solo resolvía `m_player.inventory` y requería `m_isContainerOpen` estricto en la sesión local. | Se adaptó `resolveInventory` para usar el jugador ejecutor `curPlayer` y búsqueda espacial dentro del alcance de interacción ($\le 5\text{ m}$) si el contenedor no estaba abierto explícitamente. |
| **ERR-013** | 2026-10-17 | `WorldSession.cpp` | Invalidez de referencias a entidades tras reasignación de vector en `spawnMob` | `m_mobs` sin capacidad pre-reservada invalidaba punteros ante inserciones sucesivas de entidades. | Se añadió reserva de capacidad (`m_mobs.reserve(256)`) en `WorldSession::init`. |
| **ERR-014** | 2026-10-17 | `test_degraded_network_and_convergence.cpp` | Salida prematura del bucle de convergencia antes de recibir ACKs finales | El bucle terminaba apenas se recibían los 100 mensajes en el servidor sin esperar el RTT de vuelta para los ACKs de los últimos paquetes. | Se extendió la condición del bucle a `serverDeliveredSeqs.size() < totalMessages || clientChannel.getUnackedCount() > 0`. |
| **ERR-015** | 2026-10-17 | `test_villager_economy_and_concurrent_trade.cpp` | `addItem(ItemStack&)` mutaba y vaciaba la oferta del aldeano a count=0 | Pasar `villager.offers[0].costA` por referencia mutable a `addItem` reducía el coste de la oferta a 0. | Se pasaron copias locales desacopladas de `costA` para preservar intacta la definición mercantil de la oferta. |
| **ERR-016** | 2026-10-17 | `test_command_blocks_and_function_budget.cpp` | Aserción `getTimeOfDay() == 12000` fallaba dando `12001` | Al final de `WorldSession::tick()`, `m_clock.advanceTick()` avanza el tiempo en 1 unidad tras la ejecución de los bloques-entidad. | Se adaptó la aserción a `getTimeOfDay() == 12001` reflejando el paso de tick post-ejecución. |
| **ERR-017** | 2026-10-17 | `test_gamemodes_and_hardcore_persistence.cpp` | Modificación manual de `velocity.z` no movía al espectador en `session.tick()` | `WorldSession::tick()` recalcula `velocity` a partir de `m_pendingMoveInput`. | Se despachó `MoveCommand` con `moveInput = {0, 0, 1}` y `yaw = 90.0f` antes de `session.tick()`. |
| **ERR-018** | 2026-10-18 | `HUD.cpp` | `glUniformMatrix4fv` para `uOrtho` no coincidía con uniformes del shader de HUD | El shader `hud.vert` utiliza el uniforme de matriz ortogonal `uOrtho` con mapeo de espacio normalizado (-1..1). | Se corrigió el nombre del uniforme a `uOrtho` y se estructuró la matriz ortogonal con escalado diagonal y traslación directa. |
| **ERR-019** | 2026-10-18 | `test_touch_controller_and_safe_area.cpp` | `session.getPlayer()` retorna `PlayerEntity&` que requiere lvalue al mutar inventario | `addItem` recibe referencia no-const (`ItemStack&`), impidiendo pasar temporales rvalue. | Se instanció la variable `ItemStack woodItem(...)` como lvalue antes de despachar a `addItem`. |
| **WRN-001** | 2026-10-15 | `main.cpp` | `warning: se define la variable ‘frameCount’ pero no se usa [-Wunused-but-set-variable=]` | La variable de conteo de fotogramas solo se leía en una línea de depuración comentada. | Se utilizó `frameCount` para actualizar dinámicamente el título de la ventana con los FPS reales cada segundo (`OpenCraft — 60 FPS`). |

---

## 6. Métricas de Rendimiento y Tamaño del Binario

- **Tamaño de Binarios**:
  - `opencraft` (cliente gráfico + servidor local integrado): `1.5 MB` (sin comprimir, incluyendo todos los sistemas F00 a F14).
  - `opencraft_server` (servidor dedicado headless): `920 KB` (sin dependencias gráficas/SDL3).
- **Consumo de Memoria Base**: ~65–90 MB cliente / ~25–35 MB servidor dedicado con mundo activo.
- **Tasa de Aprobación de Pruebas**: **61/61 pruebas pasando al 100% (100.0%)** en CTest.
- **Tiempo Total de Ejecución de la Suite**: ~135 segundos (incluyendo 100 semillas deterministas y 20 ciclos de suspensión móvil).

---

## 7. Próximos Pasos en el Roadmap
1. **Fase 15 (Cierre de Paridad, Catálogo Exhaustivo 1.21 y Candidato Estable)**: Cierre de la brecha final **B23 (Catálogo de la versión objetivo)**, certificación integral de todas las familias de bloques e ítems de la versión 1.21 del juego de referencia (Trial Chambers, Crafter, Torbellino rod, etc.), auditoría de invariantes y empaquetado release final.

