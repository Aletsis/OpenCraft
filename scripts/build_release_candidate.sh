#!/usr/bin/env bash
# Construye, verifica y empaqueta una versión candidata de OpenCraft para Linux.
#
#   1. Compila en Debug (aserciones activas) y ejecuta la suite completa de CTest.
#   2. Compila en Release (sin forzar -DNDEBUG a mano: lo aplica CMAKE_BUILD_TYPE).
#   3. Prueba de humo sin ventana: arranque del cliente, creación de mundo y captura.
#   4. Instala en un árbol reubicable (bin/ + share/opencraft/assets) y genera tar.gz + SHA-256.
#
# Uso: scripts/build_release_candidate.sh [directorio_de_trabajo]
# Solo borra los subdirectorios que crea dentro del directorio de trabajo (por defecto ./dist).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="${1:-$ROOT/dist}"
JOBS="$(nproc 2>/dev/null || echo 2)"
ARCH="$(uname -m)"

DEBUG_DIR="$WORK/build-debug"
RELEASE_DIR="$WORK/build-release"
SMOKE_DIR="$WORK/smoke"

step() { printf '\n==> %s\n' "$*"; }

mkdir -p "$WORK"
rm -rf "$DEBUG_DIR" "$RELEASE_DIR" "$SMOKE_DIR"

step "Compilación Debug y suite de pruebas"
cmake -S "$ROOT" -B "$DEBUG_DIR" -DCMAKE_BUILD_TYPE=Debug > "$WORK/configure-debug.log"
cmake --build "$DEBUG_DIR" -j"$JOBS" > "$WORK/build-debug.log" 2>&1 || { tail -40 "$WORK/build-debug.log"; exit 1; }
( cd "$DEBUG_DIR" && ctest -j"$JOBS" --timeout 300 --output-on-failure ) | tee "$WORK/ctest.log"

step "Compilación Release"
cmake -S "$ROOT" -B "$RELEASE_DIR" -DCMAKE_BUILD_TYPE=Release -DOPENCRAFT_BUILD_TESTS=OFF > "$WORK/configure-release.log"
cmake --build "$RELEASE_DIR" -j"$JOBS" > "$WORK/build-release.log" 2>&1 || { tail -40 "$WORK/build-release.log"; exit 1; }
VERSION="$(cat "$RELEASE_DIR/version.txt")"
PACKAGE="opencraft-${VERSION}-linux-${ARCH}"
STAGE="$WORK/$PACKAGE"
rm -rf "$STAGE" "$WORK/$PACKAGE.tar.gz" "$WORK/$PACKAGE.tar.gz.sha256"

step "Instalación en árbol reubicable: $PACKAGE"
cmake --install "$RELEASE_DIR" --prefix "$STAGE" > "$WORK/install.log"
test -x "$STAGE/bin/opencraft" && test -x "$STAGE/bin/opencraft_server"
test -f "$STAGE/share/opencraft/assets/shaders/voxel.vert"

step "Prueba de humo del paquete instalado"
"$STAGE/bin/opencraft" --version
"$STAGE/bin/opencraft_server" --help > /dev/null
mkdir -p "$SMOKE_DIR"
cat > "$SMOKE_DIR/smoke.txt" <<EOF
waitready
wait 2
shot $SMOKE_DIR/smoke.png
key escape
wait 0.3
quit
EOF
# Desde un directorio ajeno: los recursos deben resolverse junto al ejecutable (A09)
if ( cd "$SMOKE_DIR" && env SDL_VIDEODRIVER=offscreen SDL_AUDIO_DRIVER=dummy \
        XDG_DATA_HOME="$SMOKE_DIR/data" XDG_CONFIG_HOME="$SMOKE_DIR/config" \
        timeout 120 "$STAGE/bin/opencraft" --create "Humo" --seed 1 --script "$SMOKE_DIR/smoke.txt" \
        > "$SMOKE_DIR/smoke.log" 2>&1 ); then
    test -s "$SMOKE_DIR/smoke.png" || { echo "La prueba de humo no generó la captura"; exit 1; }
    test -d "$SMOKE_DIR/data/opencraft/worlds/humo" || { echo "La prueba de humo no guardó el mundo"; exit 1; }
    echo "Prueba de humo correcta (captura en $SMOKE_DIR/smoke.png)"
else
    if grep -q "Fallo crítico al inicializar la ventana" "$SMOKE_DIR/smoke.log"; then
        echo "AVISO: sin controlador de vídeo sin ventana (offscreen) en esta máquina; se omite la prueba gráfica"
    else
        tail -30 "$SMOKE_DIR/smoke.log"
        exit 1
    fi
fi

step "Empaquetado"
tar -C "$WORK" -czf "$WORK/$PACKAGE.tar.gz" "$PACKAGE"
( cd "$WORK" && sha256sum "$PACKAGE.tar.gz" > "$PACKAGE.tar.gz.sha256" )

step "Listo"
echo "Paquete:   $WORK/$PACKAGE.tar.gz"
echo "Checksum:  $(cat "$WORK/$PACKAGE.tar.gz.sha256")"
echo "Commit:    $(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo desconocido)"
