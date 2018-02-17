#ifndef GLSHADER_H
#define GLSHADER_H

#include <Graphics/opengl.h>
#include <string>

class GLShader {
public:
    GLShader();
    GLShader(GLenum shaderType, const std::string &shaderSource);
    ~GLShader();

    void loadShader(GLenum shaderType, const std::string &shaderSource);
    GLuint getHandle() const;

private:
    GLuint handle;
};

#endif