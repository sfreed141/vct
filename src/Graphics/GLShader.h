#ifndef GLSHADER_H
#define GLSHADER_H

#include <cassert>
#include <string>

#include "opengl.h"
#include "GLHelper.h"

class GLShader {
public:
    GLShader(GLenum shaderType, const std::string &shaderSource, bool fromFile = true) {
        if (fromFile) {
            this->handle = GLHelper::createShaderFromFile(shaderType, shaderSource);
        }
        else {
            this->handle = GLHelper::createShaderFromString(shaderType, shaderSource.c_str());
        }
        this->type = shaderType;
        this->name = shaderSource;

        assert(this->name.size() <= MIN_MAX_LABEL_LENGTH);
        glObjectLabel(GL_SHADER, this->handle, this->name.size(), this->name.c_str());
    }

    ~GLShader() {
        glDeleteShader(this->handle);
    }

	GLShader(const GLShader &other) = delete;
	GLShader &operator=(const GLShader &other) = delete;

	GLShader(GLShader &&other) = delete;
	GLShader &operator=(GLShader &&other) = delete;
    
    GLuint getHandle() const { return handle; }
    GLenum getType() const { return type; }
    const std::string &getName() const { return name; }

private:
    GLuint handle;
    GLenum type;
    std::string name;

    const GLsizei MIN_MAX_LABEL_LENGTH = 256;
};

#endif