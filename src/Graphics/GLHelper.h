#ifndef GLHELPER_H
#define GLHELPER_H

#include <Graphics/opengl.h>
#include <string>
#include <vector>

#define GL_DEBUG_PUSH(name) { if (GLAD_GL_KHR_debug) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, (name)); }
#define GL_DEBUG_POP() { if (GLAD_GL_KHR_debug) glPopDebugGroup(); }

class GLHelper {
public:
    static void printGLInfo();
    static void printGLExtensions();
    static void registerDebugOutputCallback();
    static void printUniformInfo(GLuint program);
    static void getMemoryUsage(GLint &totalMem, GLint &availableMem);

    static GLuint createTextureFromImage(const std::string &imagename);
    static GLuint createCubemap(const std::vector<std::string> &imagenames);
    static std::string readText(const std::string &filename);
    static GLuint createShaderFromFile(GLenum shaderType, const std::string &filename);
    static GLuint createShaderFromString(GLenum shaderType, const char *shaderText);
    static bool checkShaderStatus(GLuint shader);
    static bool checkShaderProgramStatus(GLuint program);
    static bool checkFramebufferComplete(GLuint fbo);
    
    static GLenum shaderTypeFromExtension(const std::string &filename);
};

#endif
