#include <Graphics/opengl.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include "GLShader.h"

GLShader::GLShader() :
    handle(0)
    {}

GLShader::GLShader(GLenum shaderType, std::string shaderSource) {
    loadShader(shaderType, shaderSource);
}

GLShader::~GLShader() {
    glDeleteShader(handle);
}

void GLShader::loadShader(GLenum shaderType, std::string shaderSource) {
    this->handle = createShader(shaderType, shaderSource);
}

GLuint GLShader::getHandle() const {
    return handle;
}

GLuint GLShader::createShader(GLenum shaderType, std::string filename) {
    GLint success;
    GLchar infoLog[512];

    GLuint shader;
    shader = glCreateShader(shaderType);

    std::string str = GLShader::readText(filename);
    const char *shaderString = str.c_str();

    glShaderSource(shader, 1, &shaderString, NULL);
    glCompileShader(shader);
    
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout
            << "ERROR::SHADER::"
            << (shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
            << "::COMPILATION_FAILED\n"
            << infoLog
            << std::endl;
    }

    return shader;
}

std::string GLShader::readText(std::string filename) {
    std::ifstream ifs {filename};
    
    std::stringstream buffer;
    buffer << ifs.rdbuf();

    ifs.close();

    return buffer.str();
}
