#include <Graphics/opengl.h>
#include <string>
#include <iostream>
#include "GLShaderProgram.h"
#include "GLShader.h"
#include "GLHelper.h"

GLShaderProgram::GLShaderProgram() :
    handle(0)
    {}

GLShaderProgram::GLShaderProgram(std::string computeSource) {
    linkProgram(computeSource);
}

GLShaderProgram::GLShaderProgram(std::string vertSource, std::string fragSource) {
    linkProgram(vertSource, fragSource);
}

GLShaderProgram::GLShaderProgram(std::string vertSource, std::string fragSource, std::string geomSource) {
	linkProgram(vertSource, fragSource, geomSource);
}

GLShaderProgram::~GLShaderProgram() {
    glDeleteProgram(handle);
}

void GLShaderProgram::linkProgram(std::string computeSource) {
	GLShader computeShader{ GL_COMPUTE_SHADER, computeSource };

	GLuint program;
	program = glCreateProgram();

	glAttachShader(program, computeShader.getHandle());

	glLinkProgram(program);

	GLHelper::checkShaderProgramStatus(program);

	this->handle = program;
}

void GLShaderProgram::linkProgram(std::string vertSource, std::string fragSource) {
    this->handle = createProgram(vertSource, fragSource);
}

void GLShaderProgram::linkProgram(std::string vertSource, std::string fragSource, std::string geomSource) {
	GLShader vertexShader{ GL_VERTEX_SHADER, vertSource };
	GLShader fragmentShader{ GL_FRAGMENT_SHADER, fragSource };
	GLShader geometryShader{ GL_GEOMETRY_SHADER, geomSource };

	GLuint program;
	program = glCreateProgram();

	glAttachShader(program, vertexShader.getHandle());
	glAttachShader(program, fragmentShader.getHandle());
	glAttachShader(program, geometryShader.getHandle());

	glLinkProgram(program);
	
	GLHelper::checkShaderProgramStatus(program);

	this->handle = program;
}

GLuint GLShaderProgram::getHandle() const {
    return handle;
}

void GLShaderProgram::bind() const {
    glUseProgram(handle);
}

void GLShaderProgram::unbind() const {
    glUseProgram(0);
}

GLint GLShaderProgram::uniformLocation(const GLchar *name) {
    return glGetUniformLocation(handle, name);
}

GLuint GLShaderProgram::createProgram(std::string vertSource, std::string fragSource) {
    GLShader vertexShader {GL_VERTEX_SHADER, vertSource};
    GLShader fragmentShader {GL_FRAGMENT_SHADER, fragSource};

    GLuint program;
    program = glCreateProgram();

    glAttachShader(program, vertexShader.getHandle());
    glAttachShader(program, fragmentShader.getHandle());

    glLinkProgram(program);

	GLHelper::checkShaderProgramStatus(program);

    return program;
}
