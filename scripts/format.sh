#!/usr/bin/env bash
# Aplica (por defecto) o comprueba (--check) el formato de todo el código C++ con clang-format.
# Uso: scripts/format.sh [--check]
set -euo pipefail
cd "$(dirname "$0")/.."

CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"
REQUIRED="$(sed -n 's/^clang-format==//p' scripts/requirements-ci.txt)"
FOUND="$("$CLANG_FORMAT" --version | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
if [[ "$FOUND" != "$REQUIRED" ]]; then
    echo "Aviso: clang-format $FOUND en lugar de $REQUIRED; instale la versión fijada con" >&2
    echo "  pip install -r scripts/requirements-ci.txt" >&2
fi

mapfile -t FILES < <(git ls-files -- 'src/*.cpp' 'src/*.hpp' 'src/*.h' 'tests/*.cpp' 'tests/*.hpp')
if [[ "${1:-}" == "--check" ]]; then
    "$CLANG_FORMAT" --dry-run --Werror "${FILES[@]}"
    echo "Formato correcto en ${#FILES[@]} archivos."
else
    "$CLANG_FORMAT" -i "${FILES[@]}"
    echo "Formateados ${#FILES[@]} archivos."
fi
