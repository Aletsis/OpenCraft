# Manual de uso y operación — OpenCraft 1.0.0-rc1

Este manual describe el ejecutable que se distribuye en la versión candidata 1.0.0-rc1. Las funciones que
aún no están disponibles se indican como tales; el detalle está en [ESTADO_RC1.md](ESTADO_RC1.md).

---

## 1. Requisitos

| | Mínimo verificado |
| :--- | :--- |
| Sistema | Linux x86_64 (verificado en Arch Linux). ARM64 compila con el mismo CMake, sin verificar en hardware |
| Gráficos | OpenGL ES 3.0 (Mesa en Intel/AMD, o controladores con soporte GLES 3) |
| Bibliotecas | SDL3 y GLESv2 para el cliente; el servidor solo necesita la biblioteca estándar |
| Compilador | C++20 con `<format>`: verificado con GCC 16.2 |
| Disco | ~10 MB para el programa; cada mundo ocupa de 1 a 50 MB según lo explorado |

## 2. Compilación e instalación

```bash
# Dependencias (Arch): sudo pacman -S cmake gcc sdl3 mesa pkgconf
# Dependencias (Debian/Ubuntu con SDL3 disponible): sudo apt install cmake g++ pkg-config libsdl3-dev libgles-dev

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build --prefix /usr/local     # opcional
```

Opciones de CMake:

| Opción | Por defecto | Efecto |
| :--- | :--- | :--- |
| `OPENCRAFT_BUILD_CLIENT` | `ON` | Cliente gráfico (requiere SDL3 y GLES) |
| `OPENCRAFT_BUILD_TESTS` | `ON` | Suite CTest (78 pruebas) |

### 2.1 Paquete de la versión candidata

```bash
scripts/build_release_candidate.sh [directorio]   # por defecto dist/
```

El script compila en Debug y ejecuta toda la suite CTest con las aserciones activas, compila en Release,
instala en un árbol reubicable (`bin/`, `share/opencraft/assets`, entrada de escritorio, icono y
metadatos AppStream), hace una prueba de humo sin ventana del paquete instalado desde otro directorio
y genera `opencraft-<versión>-linux-<arquitectura>.tar.gz` con su `.sha256`. Si cualquier paso falla, no
se genera el paquete.

## 3. Primer arranque

```bash
opencraft            # o ./build/opencraft
```

1. **Pantalla de título** → *Un jugador*.
2. **Seleccionar mundo** → *Crear mundo nuevo*.
3. Escriba el nombre, elija el **modo** (Supervivencia, Creativo o Extremo) y, si quiere, una **semilla**
   (un número o cualquier texto; vacío = aleatoria). *Crear mundo nuevo*.
4. Tras la pantalla de carga aparecerá sobre la superficie, en un lugar seguro.

Para volver a un mundo, selecciónelo en la lista y pulse *Jugar mundo seleccionado* (o haga doble clic).
*Eliminar* pide confirmación y guarda antes una copia completa en la carpeta de copias de seguridad.

### 3.1 Opciones de línea de comandos

| Opción | Descripción |
| :--- | :--- |
| `--world <id>` | Abrir directamente un mundo existente |
| `--create <nombre>` | Crear un mundo y entrar en él (`--seed <s>`, `--creative`) |
| `--list` | Listar los mundos guardados y salir |
| `--width`, `--height`, `--fullscreen` | Tamaño de la ventana |
| `--autosave <segundos>` | Intervalo de autoguardado (por defecto 30) |
| `--script <archivo>` | Guion de pruebas automáticas (ver §8) |
| `--version`, `--help` | Versión y ayuda |

Variables de entorno: `OPENCRAFT_ASSETS_DIR` (carpeta de recursos si no está junto al ejecutable),
`XDG_DATA_HOME` y `XDG_CONFIG_HOME` (ubicación de datos y opciones).

## 4. Cómo se juega

### 4.1 Controles

| Acción | Teclado y ratón | Táctil | Mando |
| :--- | :--- | :--- | :--- |
| Moverse | W A S D | Joystick izquierdo | Stick izquierdo |
| Mirar | Ratón | Arrastrar en la mitad derecha | Stick derecho |
| Saltar / nadar | Espacio (mantener) | Saltar | A |
| Volar (creativo) | Doble Espacio; Espacio sube, Mayús baja | — | — |
| Agacharse (no cae por bordes) | Mayús | Agachar | B |
| Correr | Ctrl izquierdo | — | — |
| Romper / atacar | Clic izquierdo; mantener para minar | Romper | Gatillo derecho |
| Usar / colocar / comer | Clic derecho | Usar | Gatillo izquierdo |
| Copiar bloque (creativo) | Clic central | — | — |
| Barra rápida | 1–9 o rueda | Tocar la ranura | LB / RB |
| Inventario | E | Inv. | Y |
| Soltar objeto | Q · Ctrl+Q suelta la pila | — | — |
| Menú de pausa | Esc | II | Start |
| Ocultar interfaz / captura / depuración | F1 / F2 / F3 | — | — |
| Pantalla completa | F11 | — | — |

Los controles táctiles aparecen al tocar la pantalla (en Linux móvil, desde el inicio). Pueden ocultarse en
*Opciones → Controles táctiles*. *Salto automático* sube escalones de un bloque al avanzar.

### 4.2 Inventario y fabricación

- **E** abre el inventario con la cuadrícula de fabricación 2×2. Una **mesa de trabajo** (clic derecho)
  da la cuadrícula 3×3; un **horno** acepta material arriba y combustible abajo.
- Clic izquierdo toma o deja una pila; clic derecho toma la mitad o deja una unidad; **Mayús+clic** la mueve
  al otro contenedor (o entre barra rápida y mochila). Clic fuera del panel suelta lo que lleva el cursor.
- En **creativo** el inventario muestra la paleta de todos los bloques y objetos (botones < > para pasar
  de página). Clic: pila completa en el cursor; clic derecho: una unidad; Mayús+clic: directo al inventario.

### 4.3 Modos de juego

| Modo | Características |
| :--- | :--- |
| Supervivencia | Salud, hambre, oxígeno, daño por caída, minado con tiempo según la herramienta |
| Creativo | Sin daño ni hambre, paleta completa, ruptura instantánea y vuelo |
| Extremo | Supervivencia con la dificultad máxima y una sola vida; al morir solo se puede observar |

La noche llega a los 10 minutos de juego (ciclo de 20 minutos). Las criaturas hostiles aparecen en la
oscuridad; duerma en una cama para fijar su punto de reaparición.

### 4.4 Opciones

*Opciones* (en el título o en pausa): distancia de render (2–16 chunks), campo de visión, brillo,
sensibilidad, escala de interfaz, invertir ratón, controles táctiles, salto automático, subtítulos, alto
contraste, sincronía vertical y volumen. Clic izquierdo avanza el valor y clic derecho lo retrocede. Se
guardan en `~/.config/opencraft/settings.cfg`.

## 5. Datos, copias de seguridad y recuperación

| Contenido | Ruta |
| :--- | :--- |
| Mundos | `~/.local/share/opencraft/worlds/<id>/` |
| Copias de seguridad | `~/.local/share/opencraft/backups/<id>_backup_<fecha>/` |
| Capturas (F2) | `~/.local/share/opencraft/screenshots/` |
| Opciones | `~/.config/opencraft/settings.cfg` |

- El juego guarda cada 30 segundos, al pausar en segundo plano y al salir con *Guardar y salir al título*.
  Cada guardado es una transacción con journal: los archivos se escriben y sincronizan (`fsync`) antes de
  confirmarse; si el equipo se apaga a mitad, al abrir el mundo se completa o se descarta la transacción.
- Un mundo solo puede estar abierto en una instancia (`session.lock`).
- Para **restaurar una copia**, copie la carpeta de `backups/` a `worlds/` con otro nombre.
- Un mundo de una versión anterior se abre con compatibilidad hacia atrás; antes de actualizar su formato
  se guarda automáticamente una copia completa en `backups/`.
- Los mundos de versiones anteriores ubicados en `./saves` se copian a la carpeta nueva al arrancar.

## 6. Servidor dedicado

```bash
opencraft_server --port 35565 --world mi_servidor --seed 12345
```

| Opción | Por defecto |
| :--- | :--- |
| `--port <n>` | 35565 (UDP) |
| `--world <nombre>` | `dedicated_server_world` (en `./saves/` del directorio de ejecución) |
| `--seed <n>` | 123456789 |
| `--max-players <n>` | 16 |
| `--whitelist` | desactivada |
| `--save-interval <s>` | 30 |

El servidor simula el mundo a 20 ticks por segundo, valida alcance y movimiento de los clientes y guarda
de forma transaccional. Se detiene con Ctrl+C (guarda antes de salir). **En esta versión candidata el
cliente gráfico todavía no puede unirse a una partida remota** (ver [ESTADO_RC1.md](ESTADO_RC1.md)).

## 7. Solución de problemas

| Síntoma | Causa y solución |
| :--- | :--- |
| «No se encontraron los recursos del juego» | Falta `assets/shaders`. Ejecute el paquete instalado o defina `OPENCRAFT_ASSETS_DIR` |
| «El mundo está abierto en otra ventana» | Cierre la otra instancia. Si se cerró de forma anormal, el bloqueo caduca al comprobar el proceso |
| No se puede cargar un mundo | Datos dañados: restaure una copia desde `backups/` |
| Sin sonido | No hay dispositivo de audio; el juego continúa en silencio. Revise el registro (`Audio no disponible`) |
| Rendimiento bajo | Reduzca la distancia de render en *Opciones*; F3 muestra FPS y chunks pendientes |

## 8. Guiones de prueba automáticas (QA)

`opencraft --script guion.txt` ejecuta instrucciones línea a línea para reproducir recorridos y obtener
capturas sin intervención (también sin ventana con `SDL_VIDEODRIVER=offscreen`):

```text
waitready              # esperar a que termine la carga del mundo
key e                  # pulsar una tecla (down/up/hold <tecla> <segundos>)
click 50% 40%          # clic en coordenadas de ventana (píxeles o porcentaje); rclick = derecho
press 640 360          # mantener pulsado el ratón (release para soltar)
look 90 -30            # girar la cámara
type Mi mundo          # escribir texto en el campo con foco
wait 1.5               # esperar segundos
shot salida/captura.png
time 14000             # fijar la hora del mundo (ticks)
summon opencraft:zombie 5 0
quit
```
