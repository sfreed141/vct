#include <Graphics/opengl.h>
#include <string>
#include "GLShader.h"
#include "GLHelper.h"

GLShader::GLShader() :
    handle(0)
    {}

GLShader::GLShader(GLenum shaderType, const std::string &shaderSource) {
    loadShader(shaderType, shaderSource);
}

GLShader::~GLShader() {
    glDeleteShader(handle);
}

void GLShader::loadShader(GLenum shaderType, const std::string &shaderSource) {
    this->handle = GLHelper::createShaderFromFile(shaderType, shaderSource);
}

GLuint GLShader::getHandle() const {
    return handle;
}
