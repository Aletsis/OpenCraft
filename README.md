# OpenCraft

[![CI](https://github.com/Aletsis/OpenCraft/actions/workflows/ci.yml/badge.svg)](https://github.com/Aletsis/OpenCraft/actions/workflows/ci.yml)
[![Release](https://github.com/Aletsis/OpenCraft/actions/workflows/release.yml/badge.svg)](https://github.com/Aletsis/OpenCraft/releases)
[![Licencia](https://img.shields.io/badge/licencia-Apache%202.0-blue.svg)](LICENSE)

**OpenCraft** es un juego *sandbox* de bloques en 3D escrito en C++20 con SDL3 y OpenGL ES 3.0. Está pensado
para ser ligero y funcionar tanto en escritorio como en Linux móvil (Phosh, Plasma Mobile, postmarketOS).

Versión actual: **1.0.0-rc1** (candidata a lanzamiento). Consulte el [estado de la versión](docs/ESTADO_RC1.md)
para ver qué está verificado y qué limitaciones conocidas tiene.

---

## Qué incluye

- **Menú de mundos**: crear (nombre, semilla y modo), abrir, eliminar con copia de seguridad previa.
- **Modos**: supervivencia, creativo (paleta de objetos y vuelo con doble salto) y extremo (una sola vida).
- **Mundo infinito y determinista**: biomas, cuevas, árboles, estructuras, Abismo y Confín con portales.
- **Iluminación**: luz del cielo con ciclo día/noche, luz de antorchas y lava, luz suave y oclusión ambiental.
- **Supervivencia**: salud, hambre, oxígeno, experiencia, armadura, minado progresivo con herramientas,
  fabricación 2×2 y 3×3, hornos, cofres, criaturas pasivas y hostiles, cultivos, cinabrio y vagonetas.
- **Interfaz**: barra rápida, inventario, contenedores, panel de depuración (F3), capturas (F2), opciones
  persistentes (distancia de render, FOV, brillo, sensibilidad, escala de interfaz, accesibilidad).
- **Audio**: efectos sintetizados con posición 3D (romper, colocar, pasos, daño, explosiones, interfaz).
- **Controles**: teclado y ratón, pantalla táctil (joystick virtual y botones) y mando.
- **Guardado transaccional**: journal con `fsync`, copias de seguridad y migración de esquemas.
- **Servidor dedicado** sin interfaz gráfica (`opencraft_server`).

## Requisitos de compilación

- Compilador C++20 con soporte de `<format>` (verificado con GCC 14 y 16 y con Clang 18)
- CMake 3.20 o superior y `pkg-config`
- SDL3 (`libsdl3-dev` / `sdl3`) y OpenGL ES 3.0 (`libgles-dev` / `mesa`), solo para el cliente gráfico

El servidor dedicado y las pruebas no necesitan SDL3 ni GLES:
`cmake -B build -DOPENCRAFT_BUILD_CLIENT=OFF`.

## Compilar y ejecutar

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/opencraft
```

Pruebas automáticas (en Debug, con aserciones activas):

```bash
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug -j$(nproc)
ctest --test-dir build-debug --output-on-failure
```

Paquete reubicable de la versión candidata (compila, prueba, verifica y empaqueta en `dist/`):

```bash
scripts/build_release_candidate.sh
```

Instalación en el sistema: `cmake --install build --prefix /usr/local`. El ejecutable encuentra sus
recursos junto a él (`bin/../share/opencraft/assets`), así que el árbol instalado puede moverse.

## Descargas

Cada etiqueta `vX.Y.Z` publica una [GitHub Release](https://github.com/Aletsis/OpenCraft/releases) con:

| Paquete | Contenido | Requisitos |
| :--- | :--- | :--- |
| `opencraft-<versión>-linux-x86_64.tar.gz` | Cliente, servidor y recursos | glibc 2.39+ (Ubuntu 24.04, Debian 13, Fedora 40 o posteriores) y OpenGL ES 3 (Mesa) |
| `opencraft-<versión>-linux-aarch64.tar.gz` | Lo mismo para ARM64 (Raspberry Pi 5, teléfonos Linux) | Igual que el anterior |
| `opencraft-<versión>-windows-x86_64.zip` | Cliente, servidor, recursos y ANGLE | Windows 10/11 de 64 bits con Direct3D 11 |
| `opencraft-<versión>-windows-arm64.zip` | Lo mismo para Windows en ARM (Snapdragon) | Windows 11 ARM64 |
| `opencraft-server-<versión>-macos-arm64.tar.gz` | Solo el servidor dedicado (experimental) | macOS 15 en Apple Silicon |

En Linux, SDL3 va enlazada dentro del ejecutable y se usa el OpenGL ES del sistema. En Windows, el juego
traduce OpenGL ES a Direct3D 11 con ANGLE (incluido); `--gl native` usa en su lugar el controlador de la
tarjeta. Android e iOS todavía no están disponibles (ver
[limitaciones](docs/ESTADO_RC1.md#limitaciones-conocidas)). Cada paquete incluye su suma `.sha256`.

## Desarrollo e integración continua

Cada push y cada pull request ejecutan [CI](.github/workflows/ci.yml):

| Trabajo | Qué comprueba |
| :--- | :--- |
| Formato | `clang-format` con la versión fijada en `scripts/requirements-ci.txt` |
| Compilación y pruebas | Cliente, servidor y suite CTest con GCC 14 y Clang 18 en x86_64 y con GCC 14 en ARM64 |
| ASan + UBSan | Suite completa con sanitizadores de memoria y comportamiento indefinido |
| Cobertura | Cobertura de líneas de `src/` **≥ 80 %** (informe HTML como artefacto) |
| macOS | Servidor y suite en macOS arm64 (experimental, no bloquea) |

Los mismos pasos se ejecutan en local:

```bash
pip install -r scripts/requirements-ci.txt   # clang-format y gcovr con versión fija
scripts/format.sh                            # aplica el formato (--check solo comprueba)
scripts/ci/coverage.sh                       # suite con cobertura y umbral del 80 %
scripts/build_release_candidate.sh           # paquete completo con prueba de humo
```

Para publicar una versión: actualice `project(... VERSION ...)` y `OPENCRAFT_VERSION_SUFFIX` en
`CMakeLists.txt` y el `CHANGELOG.md`, y suba la etiqueta (`git tag -a v1.0.0-rc1 -m "..." && git push origin
v1.0.0-rc1`). El flujo [Release](.github/workflows/release.yml) vuelve a ejecutar las pruebas, comprueba que la
etiqueta coincide con la versión, genera los paquetes y crea la release (marcada como *prerelease* si la
etiqueta lleva sufijo, p. ej. `-rc1`).

## Controles

| Acción | Teclado y ratón | Táctil | Mando |
| :--- | :--- | :--- | :--- |
| Moverse | W A S D | Joystick izquierdo | Stick izquierdo |
| Mirar | Ratón | Arrastrar en la mitad derecha | Stick derecho |
| Saltar / nadar | Espacio (mantener) | Botón Saltar | A |
| Volar (creativo) | Doble pulsación de Espacio; Mayús para bajar | — | — |
| Agacharse | Mayús | Botón Agachar | B |
| Correr | Ctrl izquierdo | — | — |
| Romper / atacar | Clic izquierdo (mantener para minar) | Botón Romper | Gatillo derecho |
| Usar / colocar | Clic derecho | Botón Usar | Gatillo izquierdo |
| Elegir bloque (creativo) | Clic central | — | — |
| Barra rápida | 1–9 o rueda del ratón | Tocar la ranura | LB / RB |
| Inventario | E | Botón Inv. | Y |
| Soltar objeto | Q (Ctrl+Q: la pila) | — | — |
| Pausa | Esc | Botón II | Start |
| Ocultar interfaz | F1 | — | — |
| Captura de pantalla | F2 | — | — |
| Depuración | F3 | — | — |
| Pantalla completa | F11 | — | — |

En el inventario: clic izquierdo toma o deja la pila, clic derecho divide o deja una unidad,
Mayús+clic la transfiere rápidamente y hacer clic fuera del panel suelta el objeto del cursor.

## Dónde se guardan los datos

| Contenido | Ruta (Linux) |
| :--- | :--- |
| Mundos | `~/.local/share/opencraft/worlds/` |
| Copias de seguridad | `~/.local/share/opencraft/backups/` |
| Capturas | `~/.local/share/opencraft/screenshots/` |
| Opciones | `~/.config/opencraft/settings.cfg` |

Las rutas respetan `XDG_DATA_HOME` y `XDG_CONFIG_HOME`. Los mundos de versiones anteriores que estuvieran
en `./saves` se copian automáticamente a la carpeta nueva la primera vez (los originales no se tocan).

## Documentación

- [Manual de uso y operación](docs/MANUAL.md)
- [Estado de la versión 1.0.0-rc1](docs/ESTADO_RC1.md): verificación y limitaciones conocidas
- [Registro de cambios](CHANGELOG.md)
- [Arquitectura](ARCHITECTURE.md) y [bitácora de desarrollo](DEVELOPMENT_LOG.md)
- [Comparación con el juego de referencia y plan de fases](docs/referencia/PLAN_MAESTRO.md)
  (documentos históricos de planificación)

## Licencia

Copyright 2026 Alexis Barron. Distribuido bajo la [Licencia Apache 2.0](LICENSE); consulte también
[NOTICE](NOTICE).

OpenCraft es un proyecto independiente; no está afiliado, patrocinado ni aprobado por Mojang Studios ni
por Microsoft. Todas las texturas y sonidos se generan en el propio programa.
