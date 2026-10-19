#pragma once

// Acceso a OpenGL ES 3.0. Las funciones no se enlazan con libGLESv2: se cargan en tiempo de ejecución a
// través de SDL (GL::load) desde el controlador del sistema o desde ANGLE, según el backend elegido.
// Incluya este archivo en lugar de <GLES3/gl3.h>.

#ifndef GL_GLES_PROTOTYPES
#define GL_GLES_PROTOTYPES 0
#endif
#include <GLES3/gl3.h>

#define OPENCRAFT_GL_FUNCTIONS(X)     X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
    X(PFNGLATTACHSHADERPROC, glAttachShader) \
    X(PFNGLBINDBUFFERPROC, glBindBuffer) \
    X(PFNGLBINDTEXTUREPROC, glBindTexture) \
    X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
    X(PFNGLBLENDFUNCPROC, glBlendFunc) \
    X(PFNGLBUFFERDATAPROC, glBufferData) \
    X(PFNGLCLEARPROC, glClear) \
    X(PFNGLCLEARCOLORPROC, glClearColor) \
    X(PFNGLCOMPILESHADERPROC, glCompileShader) \
    X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
    X(PFNGLCREATESHADERPROC, glCreateShader) \
    X(PFNGLCULLFACEPROC, glCullFace) \
    X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers) \
    X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
    X(PFNGLDELETESHADERPROC, glDeleteShader) \
    X(PFNGLDELETETEXTURESPROC, glDeleteTextures) \
    X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays) \
    X(PFNGLDEPTHFUNCPROC, glDepthFunc) \
    X(PFNGLDEPTHMASKPROC, glDepthMask) \
    X(PFNGLDETACHSHADERPROC, glDetachShader) \
    X(PFNGLDISABLEPROC, glDisable) \
    X(PFNGLDRAWARRAYSPROC, glDrawArrays) \
    X(PFNGLDRAWELEMENTSPROC, glDrawElements) \
    X(PFNGLENABLEPROC, glEnable) \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
    X(PFNGLGENBUFFERSPROC, glGenBuffers) \
    X(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap) \
    X(PFNGLGENTEXTURESPROC, glGenTextures) \
    X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
    X(PFNGLGETERRORPROC, glGetError) \
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
    X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
    X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
    X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
    X(PFNGLGETSTRINGPROC, glGetString) \
    X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
    X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
    X(PFNGLPOLYGONOFFSETPROC, glPolygonOffset) \
    X(PFNGLREADPIXELSPROC, glReadPixels) \
    X(PFNGLSHADERSOURCEPROC, glShaderSource) \
    X(PFNGLTEXIMAGE2DPROC, glTexImage2D) \
    X(PFNGLTEXPARAMETERIPROC, glTexParameteri) \
    X(PFNGLUNIFORM1FPROC, glUniform1f) \
    X(PFNGLUNIFORM1IPROC, glUniform1i) \
    X(PFNGLUNIFORM2FPROC, glUniform2f) \
    X(PFNGLUNIFORM3FPROC, glUniform3f) \
    X(PFNGLUNIFORM4FPROC, glUniform4f) \
    X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) \
    X(PFNGLUSEPROGRAMPROC, glUseProgram) \
    X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
    X(PFNGLVIEWPORTPROC, glViewport) \
    X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
    X(PFNGLPIXELSTOREIPROC, glPixelStorei)

#define OPENCRAFT_GL_DECLARE(type, name) extern type oc_##name;
OPENCRAFT_GL_FUNCTIONS(OPENCRAFT_GL_DECLARE)
#undef OPENCRAFT_GL_DECLARE

#define glActiveTexture oc_glActiveTexture
#define glAttachShader oc_glAttachShader
#define glBindBuffer oc_glBindBuffer
#define glBindTexture oc_glBindTexture
#define glBindVertexArray oc_glBindVertexArray
#define glBlendFunc oc_glBlendFunc
#define glBufferData oc_glBufferData
#define glClear oc_glClear
#define glClearColor oc_glClearColor
#define glCompileShader oc_glCompileShader
#define glCreateProgram oc_glCreateProgram
#define glCreateShader oc_glCreateShader
#define glCullFace oc_glCullFace
#define glDeleteBuffers oc_glDeleteBuffers
#define glDeleteProgram oc_glDeleteProgram
#define glDeleteShader oc_glDeleteShader
#define glDeleteTextures oc_glDeleteTextures
#define glDeleteVertexArrays oc_glDeleteVertexArrays
#define glDepthFunc oc_glDepthFunc
#define glDepthMask oc_glDepthMask
#define glDetachShader oc_glDetachShader
#define glDisable oc_glDisable
#define glDrawArrays oc_glDrawArrays
#define glDrawElements oc_glDrawElements
#define glEnable oc_glEnable
#define glEnableVertexAttribArray oc_glEnableVertexAttribArray
#define glGenBuffers oc_glGenBuffers
#define glGenerateMipmap oc_glGenerateMipmap
#define glGenTextures oc_glGenTextures
#define glGenVertexArrays oc_glGenVertexArrays
#define glGetError oc_glGetError
#define glGetProgramInfoLog oc_glGetProgramInfoLog
#define glGetProgramiv oc_glGetProgramiv
#define glGetShaderInfoLog oc_glGetShaderInfoLog
#define glGetShaderiv oc_glGetShaderiv
#define glGetString oc_glGetString
#define glGetUniformLocation oc_glGetUniformLocation
#define glLinkProgram oc_glLinkProgram
#define glPolygonOffset oc_glPolygonOffset
#define glReadPixels oc_glReadPixels
#define glShaderSource oc_glShaderSource
#define glTexImage2D oc_glTexImage2D
#define glTexParameteri oc_glTexParameteri
#define glUniform1f oc_glUniform1f
#define glUniform1i oc_glUniform1i
#define glUniform2f oc_glUniform2f
#define glUniform3f oc_glUniform3f
#define glUniform4f oc_glUniform4f
#define glUniformMatrix4fv oc_glUniformMatrix4fv
#define glUseProgram oc_glUseProgram
#define glVertexAttribPointer oc_glVertexAttribPointer
#define glViewport oc_glViewport
#define glBufferSubData oc_glBufferSubData
#define glPixelStorei oc_glPixelStorei

namespace OpenCraft::GL {

using ProcLoader = void* (*)(const char* name);

// Carga todas las funciones con el cargador dado (p. ej. SDL_GL_GetProcAddress). Devuelve false e indica
// en missing la primera que falte si el contexto no ofrece OpenGL ES 3.0 completo.
bool load(ProcLoader loader, const char** missing = nullptr);

} // namespace OpenCraft::GL
