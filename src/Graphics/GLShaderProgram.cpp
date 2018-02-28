#include <Graphics/opengl.h>
#include <initializer_list>
#include <string>
#include <iostream>
#include "GLShaderProgram.h"
#include "GLShader.h"
#include "GLHelper.h"

GLShaderProgram::GLShaderProgram() {
	handle = glCreateProgram();
}

GLShaderProgram::GLShaderProgram(std::initializer_list<const std::string> shaderFiles) : GLShaderProgram() {
	attachAndLink(shaderFiles);
}

GLShaderProgram::~GLShaderProgram() {
    glDeleteProgram(handle);
}

GLShaderProgram &GLShaderProgram::attachShader(const std::string &shaderFile) {
	return attachShader(GLHelper::shaderTypeFromExtension(shaderFile), shaderFile);
}

GLShaderProgram &GLShaderProgram::attachShader(GLenum shaderType, const std::string &shaderFile) {
	GLShader shader { shaderType, shaderFile };
	glAttachShader(handle, shader.getHandle());

	return *this;
}

void GLShaderProgram::linkProgram() {
	glLinkProgram(handle);
	GLHelper::checkShaderProgramStatus(handle);
}

void GLShaderProgram::attachAndLink(std::initializer_list<const std::string> shaderFiles) {
	for (const auto &s : shaderFiles) {
		attachShader(s);
	}
	linkProgram();
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

void GLShaderProgram::setObjectLabel(const std::string &label) {
	glObjectLabel(GL_PROGRAM, handle, label.size(), label.c_str());
}

GLint GLShaderProgram::uniformLocation(const GLchar *name) {
	if (uniforms.count(name) == 0) {
		uniforms[name] = glGetUniformLocation(handle, name);
	}

    return uniforms[name];
}
