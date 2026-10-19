#!/usr/bin/env bash
# Compila con instrumentación gcov, ejecuta la suite y exige la cobertura mínima de líneas sobre src/.
#
# Uso: scripts/ci/coverage.sh [directorio_build]   (por defecto build-coverage)
# Variables: COVERAGE_MIN (por defecto 80), GCOV (por defecto gcov), GCOVR (por defecto gcovr)
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="${1:-$ROOT/build-coverage}"
MIN="${COVERAGE_MIN:-80}"
GCOV="${GCOV:-gcov}"
GCOVR="${GCOVR:-gcovr}"
JOBS="$(nproc 2>/dev/null || echo 2)"

cmake -S "$ROOT" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOPENCRAFT_BUILD_CLIENT=OFF \
    -DCMAKE_CXX_FLAGS="--coverage -O0" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
find "$BUILD" -name '*.gcda' -delete
cmake --build "$BUILD" -j"$JOBS"
( cd "$BUILD" && ctest -j"$JOBS" --timeout 900 --output-on-failure )

mkdir -p "$BUILD/coverage"
"$GCOVR" -r "$ROOT" --object-directory "$BUILD" --gcov-executable "$GCOV" --filter "$ROOT/src/" \
    --gcov-ignore-parse-errors=negative_hits.warn_once_per_file \
    --txt "$BUILD/coverage/summary.txt" \
    --cobertura "$BUILD/coverage/cobertura.xml" \
    --html-details "$BUILD/coverage/index.html" \
    --markdown-summary "$BUILD/coverage/summary.md" \
    --print-summary --fail-under-line "$MIN"
echo "Cobertura de líneas >= ${MIN}% (informe en $BUILD/coverage/index.html)"
