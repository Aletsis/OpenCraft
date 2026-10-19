#!/usr/bin/env bash
# Empaqueta OpenCraft para Windows desde un shell MSYS2 (UCRT64 o CLANGARM64): ejecutables, recursos,
# ANGLE (libEGL/libGLESv2), las DLL de las que dependen y sus licencias; hace pruebas de humo y genera
# opencraft-<versión>-windows-<arquitectura>.zip con su .sha256.
#
# Uso: scripts/ci/package-windows.sh <build_release> <directorio_salida> <x86_64|arm64>
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$(cd "$1" && pwd)"
OUT="$2"
ARCH="$3"
: "${MINGW_PREFIX:?Ejecute este script dentro de un shell MSYS2 (UCRT64 o CLANGARM64)}"

VERSION="$(cat "$BUILD/version.txt")"
PACKAGE="opencraft-${VERSION}-windows-${ARCH}"
STAGE="$OUT/$PACKAGE"
rm -rf "$STAGE" "$OUT/$PACKAGE.zip" "$OUT/$PACKAGE.zip.sha256"
mkdir -p "$STAGE/licenses"

cp "$BUILD/opencraft.exe" "$BUILD/opencraft_server.exe" "$STAGE/"
mkdir -p "$STAGE/assets"
cp -r "$ROOT/assets/shaders" "$ROOT/assets/audio" "$STAGE/assets/"
cp "$ROOT/LICENSE" "$ROOT/NOTICE" "$ROOT/README.md" "$ROOT/CHANGELOG.md" "$ROOT/docs/MANUAL.md" "$STAGE/"

# ANGLE: OpenGL ES 3 sobre Direct3D 11 (el juego lo usa por defecto en Windows)
cp "$MINGW_PREFIX/bin/libEGL.dll" "$MINGW_PREFIX/bin/libGLESv2.dll" "$STAGE/"

# Dependencias de MSYS2 (SDL3, runtime de C++, zlib...), de forma transitiva
for _ in 1 2 3 4; do
    for bin in "$STAGE"/*.exe "$STAGE"/*.dll; do
        ldd "$bin" | awk '{print $3}' | grep -i "^${MINGW_PREFIX}/bin/.*\.dll$" || true
    done | sort -u | while read -r dll; do
        [[ -f "$STAGE/$(basename "$dll")" ]] || cp "$dll" "$STAGE/"
    done
done

# Licencias de los componentes de terceros que se distribuyen en binario
for pkg in sdl3 angleproject libwinpthread zlib libgcc "libstdc++" gcc-libs libc++ libunwind; do
    src="$MINGW_PREFIX/share/licenses/$pkg"
    [[ -d "$src" ]] && cp -r "$src" "$STAGE/licenses/$pkg"
done
cp "$ROOT/third_party/khronos/LICENSE.md" "$STAGE/licenses/khronos-headers.md"

echo "Contenido de $PACKAGE:"
ls -1 "$STAGE"

# Pruebas de humo desde un directorio ajeno
SMOKE="$OUT/smoke-windows"
rm -rf "$SMOKE" && mkdir -p "$SMOKE"
"$STAGE/opencraft.exe" --version
( cd "$SMOKE" && "$STAGE/opencraft_server.exe" --port 35992 --world ci --ticks 20 | tail -1 )
cat > "$SMOKE/smoke.txt" <<SCRIPT
waitready
wait 2
shot smoke.png
quit
SCRIPT
if ( cd "$SMOKE" && APPDATA="$(cygpath -w "$SMOKE/appdata")" SDL_VIDEODRIVER=offscreen SDL_AUDIO_DRIVER=dummy \
        timeout 300 "$STAGE/opencraft.exe" --create Humo --seed 1 --script smoke.txt > smoke.log 2>&1 ); then
    test -s "$SMOKE/smoke.png" || { cat "$SMOKE/smoke.log"; echo "La prueba de humo no generó la captura"; exit 1; }
    grep -a "Contexto Gráfico" "$SMOKE/smoke.log" || true
    echo "Prueba de humo gráfica correcta"
else
    cat "$SMOKE/smoke.log"
    exit 1
fi

( cd "$OUT" && zip -qr9 "$PACKAGE.zip" "$PACKAGE" && sha256sum "$PACKAGE.zip" > "$PACKAGE.zip.sha256" )
echo "Paquete: $OUT/$PACKAGE.zip"
cat "$OUT/$PACKAGE.zip.sha256"
