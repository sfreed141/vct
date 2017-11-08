#ifndef GLHELPER_H
#define GLHELPER_H

#include <GL/glew.h>

#define GL_DEBUG_PUSH(name) { if (GLEW_KHR_debug) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, (name)); }
#define GL_DEBUG_POP() { if (GLEW_KHR_debug) glPopDebugGroup(); }

class GLHelper {
public:
    static void printGLInfo();
    static void printGLExtensions();
    static void registerDebugOutputCallback();
    static void printUniformInfo(GLuint program);
    static void getMemoryUsage(GLint &totalMem, GLint &availableMem);
};

#endif
