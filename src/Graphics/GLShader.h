#ifndef GLSHADER_H
#define GLSHADER_H

#include <Graphics/opengl.h>
#include <string>

class GLShader {
public:
    GLShader();
    GLShader(GLenum shaderType, const std::string &shaderSource);
    ~GLShader();

	GLShader(const GLShader &other) = delete;
	GLShader &operator=(const GLShader &other) = delete;

	// TODO add move constructor and move assignment
	GLShader(GLShader &&other) = delete;
	GLShader &operator=(GLShader &&other) = delete;

    void loadShader(GLenum shaderType, const std::string &shaderSource);
    GLuint getHandle() const;

private:
    GLuint handle;
};

#endif