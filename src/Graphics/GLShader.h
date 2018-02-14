#ifndef GLSHADER_H
#define GLSHADER_H

#include <glad/glad.h>
#include <string>

class GLShader {
public:
    GLShader();
    GLShader(GLenum shaderType, std::string shaderSource);
    ~GLShader();

    void loadShader(GLenum shaderType, std::string shaderSource);
    GLuint getHandle() const;
    
    static std::string readText(std::string filename);
private:
    GLuint handle;

    static GLuint createShader(GLenum shaderType, std::string filename);
};

#endif