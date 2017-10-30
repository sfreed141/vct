#ifndef GLHELPER_H
#define GLHELPER_H

#include <GL/glew.h>

class GLHelper {
public:
    static void printGLInfo();
    static void printGLExtensions();
    static void registerDebugOutputCallback();
    static void printUniformInfo(GLuint program);
};

#endif
