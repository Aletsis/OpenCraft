> **Documento histórico de planificación.** El estado vigente de la versión 1.0.0-rc1, con lo verificado y las limitaciones conocidas, está en [ESTADO_RC1.md](../ESTADO_RC1.md).

# OpenCraft frente al juego de referencia: diagnóstico y brechas

Fecha de revisión: 15 de octubre de 2026. Estado: análisis estático del repositorio; no certificación de funcionamiento en dispositivos. [Plan de ejecución](PLAN_MAESTRO.md).

## Conclusión

OpenCraft es actualmente un prototipo de construcción vóxel con terreno local, movimiento, edición de bloques e infraestructura LAN parcial. El salto hacia el juego de referencia requiere principalmente sistemas de juego, persistencia y contenido interconectado, además de mejoras del motor. No basta con añadir texturas o aumentar el número de bloques.

La base aprovechable es C++20, SDL3, OpenGL ES 3.0, raycast DDA, colisiones AABB, atlas procedural y división del terreno en chunks. La prioridad móvil del proyecto se mantiene. No asignamos un porcentaje de paridad: contar por igual una textura, un sistema de inventario y una dimensión produciría una medida engañosa.

## Referencia y límites de la comparación

Se propone **la versión 1.21 del juego de referencia** como primera referencia congelada de mecánicas y catálogo; Bedrock como referencia complementaria de interacción táctil. Es una decisión de planificación, no una afirmación de que 1.21 sea la versión actual. La publicación oficial de 1.21 (notas oficiales de la versión) permite fijar un objetivo verificable. Las versiones posteriores tendrán una evaluación separada; no se incorporan silenciosamente al alcance.

“Igualar” significa cubrir capacidades jugables y el catálogo de la referencia con reglas verificadas, recursos propios y diferencias explícitas. No supone que OpenCraft use Java, se conecte al protocolo del juego de referencia, lea sus mundos ni reproduzca sus fallos. La equivalencia exacta de semillas, tiempos y circuitos exige pruebas específicas; no se deduce de una apariencia similar.

El acceso directo a https://es.wiki-comunitaria/ fue denegado por robots.txt. Se localizaron resultados indexados de la wiki, pero no se usan sus extractos antiguos para fijar reglas sensibles a versión. Las fuentes oficiales enlazadas sirven para confirmar familias de mecánicas; el inventario exhaustivo por versión es un entregable de F00. Las cantidades y umbrales del plan son propuestas propias de OpenCraft.

## Evidencia local

| Código inspeccionado | Qué permite afirmar |
| --- | --- |
| [main.cpp](../../src/main.cpp), `world.init(3)` | Se generan 7 × 7 = 49 chunks al inicio; ventana y bucle de entrada/render/red comparten `main`. |
| [World.cpp](../../src/world/World.cpp), `init/update/render` | Actualiza mallas sucias; no carga ni descarga chunks al moverse. Los chunks ausentes se consultan como aire. |
| [Chunk.hpp](../../src/world/Chunk.hpp), [Chunk.cpp](../../src/world/Chunk.cpp) | Chunks de 16 × 64 × 16; altura basada en senos/cosenos, sin semilla configurable. Árboles en una posición local prefijada cuando se cumple la condición. |
| [Block.hpp](../../src/world/Block.hpp) | 12 identificadores contando aire: 11 tipos no vacíos. No hay estados, dureza, botín ni componentes de interacción. |
| [Player.hpp](../../src/game/Player.hpp), [Player.cpp](../../src/game/Player.cpp) | Hotbar de nueve IDs sin cantidades; rotura inmediata y colocación ilimitada. Salud, hambre y vuelo declarados pero sin ciclo funcional asociado. |
| [HUD.cpp](../../src/ui/HUD.cpp), [main.cpp](../../src/main.cpp) | Mira, hotbar y entrada táctil parcial. No hay botones táctiles explícitos de saltar/romper/colocar. Las zonas táctiles de hotbar no se calculan desde los rectángulos dibujados. |
| [NetworkManager.cpp](../../src/network/NetworkManager.cpp), [NetworkProtocol.hpp](../../src/network/NetworkProtocol.hpp) | Descubrimiento y paquetes UDP iniciales; handshake incompleto y sin transferencia inicial del mundo. |
| [Chunk.cpp](../../src/world/Chunk.cpp), [voxel.frag](../../assets/shaders/voxel.frag) | Caras ocultas descartadas, sombreado por orientación y niebla. Las llamadas a `addFace` pasan AO constante 1; no se observa oclusión ambiental por vecinos. |
| [CMakeLists.txt](../../CMakeLists.txt), árbol `src/` y `assets/` | Un ejecutable, cuatro shaders, sin suite de pruebas registrada ni proyectos Android/iOS presentes. No se encontró sistema de guardado, audio, recetas o entidades de criaturas. |

El área inicial ocupa 112 × 112 bloques horizontalmente, de coordenada -48 a 63 en X/Z, y 64 de altura. Es una región finita precargada, aunque el modelo de coordenadas podría ampliarse.

## Matriz de capacidades y trabajo necesario

“Parcial” significa que existe código relevante pero falta una parte esencial. “Ausente” significa no localizado en el árbol inspeccionado, no que sea imposible añadirlo. Las referencias MC describen capacidades generales; la columna de brecha es diagnóstico propio del código.

| ID | Capacidad de referencia | Estado OpenCraft | Brecha concreta | Fase |
| --- | --- | --- | --- | --- |
| B01 | Construcción y minería | Parcial | Estados/orientación, formas no cúbicas, dureza, herramientas, drops, catálogo completo | F01, F04–05, F15 |
| B02 | Mundos persistentes | Ausente | Crear, listar, guardar, cargar, respaldar, migrar y recuperar mundos | F02 |
| B03 | Exploración procedural | Parcial | Semillas, streaming, secciones verticales, coordenadas grandes, generación versionada | F03 |
| B04 | Inventario y objetos | Parcial | Stacks, metadatos, equipo, contenedores, entidades de objetos, transacciones | F04 |
| B05 | Fabricación y transformación | Ausente | Recetas, cuadrículas, hornos, combustible y estaciones especializadas | F05, F15 |
| B06 | Supervivencia | Declaraciones sin mecánica | Salud, hambre, saturación, daño, muerte, reaparición, descanso, dificultad | F06 |
| B07 | Iluminación y tiempo | Parcial visual | Luz de cielo/bloque, antorchas, ciclo día/noche, clima | F07 |
| B08 | Fluidos y bloques activos | Agua como tipo | Flujo, lava, inmersión, fuego, gravedad de bloques, ticks programados | F07 |
| B09 | Criaturas y combate | Ausente | Entidades, IA, spawn/despawn, pathfinding, proyectiles, armadura y botín | F08 |
| B10 | Cooperación y servidores | 🟢 Completada (F09) | Servidor dedicado autoritativo, protocolo binario OCNT v100, canal fiable, snapshots y réplica de entidades | F09 |
| B11 | Biomas, cuevas y estructuras | 🟢 Completada (F10) | Generador v3 con 6 biomas continuos (T,H,C,R), cuevas 3D, minerales (esmeralda en montaña), estructuras multi-chunk y botín determinista | F03, F10 |
| B12 | Agricultura y fauna | 🟢 Completada (F10) | Farmland hidratado, cultivos (trigo/zanahoria/patata), ciclo renovable de semillas, pesca, fauna (oveja/cerdo/gallina/lobo), reproducción y domesticación | F10 |
| B13 | Aldeanos y asentamientos | 🟢 Completada (F10) | Aldeas, profesiones (granjero/bibliotecario/armero), comercio concurrente atómico con esmeraldas y reposición periódica | F10, F15 |
| B14 | Automatización | 🟢 Completada (F11) | Cinabrio 0-15, fuerte/débil, quasi-conectividad, pistones y límite 12 bloques, tolvas 8 ticks, dispensador/soltador y crafter 1.21 | F11 |
| B15 | Transporte y navegación | 🟢 Parcial (F11) | Barcos hidrostáticos, rieles (normal, propulsor, detector, activador) y vagonetas (pasajeros, cofre, tolva con absorción) | F06, F10–12 |
| B16 | Progresión avanzada | 🟢 Completada (F12) | Experiencia, encantamientos, reparación, pociones y efectos | F12 |
| B17 | Dimensiones y final | 🟢 Completada (F12) | Abismo/End equivalentes, portales, jefes, recompensas y exploración posterior | F12 |
| B18 | Modos y herramientas de creación | 🟢 Completada (F13) | Creativo completo, Aventura, Espectador, Extremo, comandos y reglas | F06, F13 |
| B19 | Personalización de contenido | 🟢 Completada (F13) | Paquetes de recursos/datos, validación, compatibilidad y documentación | F13 |
| B20 | Experiencia de usuario | 🟢 Completada (F14) | Menús, configuración, tutorial, accesibilidad, audio espacial, partículas y localización | F02, F04, F14 |
| B21 | Plataformas móviles | 🟢 Completada (F14) | Multitáctil completo, Safe Area, ciclo de vida 20x, Linux ARM64, Android e iOS | F00, F14 |
| B22 | Calidad y rendimiento | 🟢 Completada (F14) | Arnés de métricas p50/p95/p99, objetivo de 20 TPS, empaquetado y suites de prueba (sin benchmark de hardware real) | F00, F03, F14–15 |
| B23 | Catálogo de la versión objetivo | 🟡 En curso (F15) | Inventario exhaustivo y pruebas de cada familia, incluidas novedades 1.21 | F00, F15 |

El juego de referencia articula construcción libre y supervivencia como experiencias distintas; esto fundamenta B01/B06/B18. Descripción oficial (notas oficiales de la versión). Fabricar combina inventario, ingredientes y recetas; B04/B05 deben constituir un flujo transaccional completo. Guía de fabricación (notas oficiales de la versión). El horno aporta transformación de recursos, aparte de la cuadrícula. Horno (notas oficiales de la versión).

Los biomas dan variedad al entorno y las estructuras aportan destinos de exploración, motivando B11. Biomas (notas oficiales de la versión), estructuras (notas oficiales de la versión). Automatizar combina señales y máquinas, no solo un bloque rojo, de ahí B14. Introducción a cinabrio (notas oficiales de la versión).

La mejora del equipo, la alquimia y el viaje al Confín sostienen progresión más allá de construir un refugio, motivando B16/B17. Encantamientos (notas oficiales de la versión), ingrediente de alquimia (notas oficiales de la versión), End (notas oficiales de la versión). La referencia 1.21 incluye además cámaras de pruebas, Torbellino/Esqueleto del pantano, maza y crafter: deben figurar en el catálogo final. Notas de 1.21 (notas oficiales de la versión).

## Diferencias entre README y código que afectan al plan

1. **Supervivencia:** tener variables `health`/`hunger` no equivale a recibir daño, alimentarse o morir. Se debe presentar como pendiente hasta F06.
2. **LAN:** `sendBlockModify` solo tiene declaración y definición; las acciones de `Player` no lo llaman. `HandshakeAck` está enumerado sin manejo. El cliente conserva su ID inicial, no hay snapshot del mundo y no se dibujan jugadores remotos. No puede darse por terminada la cooperación.
3. **Robustez de red:** la recepción valida solo el tamaño de cabecera antes de interpretar paquetes mayores; falta validar remitente, longitudes, IDs y permisos. F01 debe contener este riesgo antes de ampliar pruebas LAN; F09 completa el protocolo.
4. **Móvil:** existen eventos táctiles, pero no un recorrido completo sin teclado ni paquetes de plataforma verificados. El soporte declarado no prueba instalación funcional.
5. **Gráficos:** sombreado por cara no equivale a luz propagada ni a AO local. La transparencia de texturas tampoco demuestra un pase translúcido ordenado del mundo.
6. **Rendimiento:** ~208 KB, <70 MB y 60 FPS son afirmaciones del README, no mediciones repetidas en esta revisión. El motor con más sistemas deberá medir tamaño instalado, CPU/GPU, memoria y temperatura sobre hardware identificado.

## Prioridad recomendada

Primero establecer una referencia verificable, separar simulación y presentación y proteger el estado guardado. Después construir el ciclo recoger → fabricar → sobrevivir → guardar. Luego conectar esa experiencia en red, ampliar exploración y automatización y cerrar progresión y catálogo. El [plan maestro](PLAN_MAESTRO.md) detalla dependencias, puertas de calidad y los archivos independientes de las 16 fases.
