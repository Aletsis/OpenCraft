#!/usr/bin/env bash
set -euo pipefail

# Script de empaquetado para Linux ARM64 / x86_64
echo "=== Empaquetador de Distribución OpenCraft ==="

BUILD_DIR="${1:-build}"
DIST_DIR="dist/opencraft"

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/bin"
mkdir -p "$DIST_DIR/share/opencraft/assets"
mkdir -p "$DIST_DIR/share/applications"
mkdir -p "$DIST_DIR/share/icons/hicolor/64x64/apps"
mkdir -p "$DIST_DIR/share/metainfo"

# 1. Copiar binarios
cp "$BUILD_DIR/opencraft" "$DIST_DIR/bin/"
cp "$BUILD_DIR/opencraft_server" "$DIST_DIR/bin/"
chmod +x "$DIST_DIR/bin/"*

# 2. Copiar assets y shaders
cp -r assets/shaders "$DIST_DIR/share/opencraft/assets/"
cp -r assets/audio "$DIST_DIR/share/opencraft/assets/"
if [ -d "assets/textures" ]; then
    cp -r assets/textures "$DIST_DIR/share/opencraft/assets/"
fi

# 3. Copiar integración de escritorio
cp assets/packaging/opencraft.desktop "$DIST_DIR/share/applications/"
cp assets/packaging/opencraft.png "$DIST_DIR/share/icons/hicolor/64x64/apps/"
cp assets/packaging/opencraft.metainfo.xml "$DIST_DIR/share/metainfo/"

# 4. Crear lanzador portable
cat << 'EOF' > "$DIST_DIR/opencraft.sh"
#!/usr/bin/env sh
DIR="$(cd "$(dirname "$0")" && pwd)"
export XDG_DATA_DIRS="$DIR/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
exec "$DIR/bin/opencraft" "$@"
EOF
chmod +x "$DIST_DIR/opencraft.sh"

# 5. Generar tarball
TAR_NAME="opencraft-$(uname -m).tar.gz"
tar -czf "$TAR_NAME" -C dist opencraft
echo "Paquete generado exitosamente: $TAR_NAME ($(du -h "$TAR_NAME" | cut -f1))"
