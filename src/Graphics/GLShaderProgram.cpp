#include <Graphics/opengl.h>
#include <initializer_list>
#include <string>
#include <iostream>
#include "GLShaderProgram.h"
#include "GLShader.h"
#include "GLHelper.h"
#include <common.h>

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
	linkStatus = GLHelper::checkShaderProgramStatus(handle);

	if (linkStatus) {
		GLint uniformCount, uniformMaxLength;
		glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniformCount);
		glGetProgramiv(handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformMaxLength);
		
		for (int i = 0; i < uniformCount; i++) {
			GLsizei length = 0;
			GLint location = 0, size = 0;
			GLenum type = GL_NONE;
			GLchar name[uniformMaxLength];
			glGetActiveUniform(handle, i, uniformMaxLength, &length, &size, &type, name); 
			location = glGetUniformLocation(handle, name);
			uniforms[name] = location;
		}
	}
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

GLint GLShaderProgram::uniformLocation(const GLchar *name) const {
	if (uniforms.count(name) == 0) {
		// This should only happen for non-active uniforms (e.g. optimized out)
		return -1;
	}

    return uniforms.at(name);
}
