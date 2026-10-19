#include "render/GL.hpp"

#define OPENCRAFT_GL_DEFINE(type, name) type oc_##name = nullptr;
OPENCRAFT_GL_FUNCTIONS(OPENCRAFT_GL_DEFINE)
#undef OPENCRAFT_GL_DEFINE

namespace OpenCraft::GL {

bool load(ProcLoader loader, const char** missing) {
    if (!loader) return false;
#define OPENCRAFT_GL_LOAD(type, name)                                                                                  \
    oc_##name = reinterpret_cast<type>(loader(#name));                                                                 \
    if (!oc_##name) {                                                                                                  \
        if (missing) *missing = #name;                                                                                 \
        return false;                                                                                                  \
    }
    OPENCRAFT_GL_FUNCTIONS(OPENCRAFT_GL_LOAD)
#undef OPENCRAFT_GL_LOAD
    return true;
}

} // namespace OpenCraft::GL
