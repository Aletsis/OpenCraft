#pragma once

#include <cstdlib>

namespace OpenCraft::TestSupport {

// Multiplicador de los presupuestos de tiempo real de las pruebas. Vale 1 por defecto; los trabajos de CI
// con sanitizadores o máquinas compartidas lo amplían con OPENCRAFT_TIME_BUDGET_SCALE (p. ej. 20).
inline double timeBudgetScale() {
    const char* v = std::getenv("OPENCRAFT_TIME_BUDGET_SCALE");
    if (!v) return 1.0;
    double s = std::atof(v);
    return s >= 1.0 ? s : 1.0;
}

} // namespace OpenCraft::TestSupport
