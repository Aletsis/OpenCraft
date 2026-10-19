#!/usr/bin/env bash
# Instala las dependencias de compilación en Ubuntu 24.04 (x86_64 o arm64) y compila SDL3 como
# biblioteca estática, de modo que los binarios publicados no dependan de un paquete SDL3 del sistema.
#
# Uso: scripts/ci/install-deps-linux.sh [prefijo_sdl3]   (por defecto /usr/local; pase un directorio
# propio, p. ej. $HOME/.local/sdl3, para poder guardarlo en caché y usarlo con CMAKE_PREFIX_PATH)
# Variables: SDL3_VERSION, SDL3_SHA256, SKIP_SDL3=1 (solo servidor y pruebas)
set -euo pipefail

SDL3_VERSION="${SDL3_VERSION:-3.4.18}"
SDL3_SHA256="${SDL3_SHA256:-9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3}"
PREFIX="${1:-/usr/local}"
SUDO=""
[[ "$(id -u)" -ne 0 ]] && SUDO="sudo"

export DEBIAN_FRONTEND=noninteractive
$SUDO apt-get update -qq
$SUDO apt-get install -y -qq --no-install-recommends \
    build-essential g++-14 cmake ninja-build pkg-config git curl ca-certificates file python3 python3-venv \
    libgles-dev libegl-dev libgl1-mesa-dri libegl-mesa0 \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev \
    libwayland-dev libxkbcommon-dev wayland-protocols libdecor-0-dev libdrm-dev libgbm-dev \
    libasound2-dev libpulse-dev libpipewire-0.3-dev libudev-dev libdbus-1-dev >/dev/null

if [[ "${SKIP_SDL3:-0}" == "1" ]]; then
    echo "SDL3 omitido (SKIP_SDL3=1)"
    exit 0
fi
if [[ -f "$PREFIX/lib/cmake/SDL3/SDL3Config.cmake" ]]; then
    echo "SDL3 ya instalado en $PREFIX"
    exit 0
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
curl -fsSL -o "$WORK/sdl3.tar.gz" \
    "https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz"
echo "$SDL3_SHA256  $WORK/sdl3.tar.gz" | sha256sum -c -
tar -xzf "$WORK/sdl3.tar.gz" -C "$WORK"
cmake -S "$WORK/SDL3-${SDL3_VERSION}" -B "$WORK/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_INSTALL_DOCS=OFF >/dev/null
cmake --build "$WORK/build" >/dev/null
INSTALL_SUDO="$SUDO"
mkdir -p "$PREFIX" 2>/dev/null && [[ -w "$PREFIX" ]] && INSTALL_SUDO=""
$INSTALL_SUDO cmake --install "$WORK/build" >/dev/null
echo "SDL3 $SDL3_VERSION (estática) instalada en $PREFIX"
