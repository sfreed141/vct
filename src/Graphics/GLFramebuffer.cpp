#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GLFramebuffer.h"
#include <cassert>

GLFramebuffer::GLFramebuffer() {
	glGenFramebuffers(1, &this->handle);
	this->currentTarget = 0;
}

GLFramebuffer::~GLFramebuffer() {
	glDeleteFramebuffers(1, &handle);
	this->currentTarget = 0;
}

void GLFramebuffer::attachTexture(GLenum attachment, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint minFilter, GLint magFilter, GLint wrapS, GLint wrapT, const glm::vec4 *borderColor) {
	GLuint texture;
	glGenTextures(1, &texture);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

	if (borderColor != nullptr) {
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(*borderColor));
	}


	glFramebufferTexture2D(this->currentTarget, attachment, GL_TEXTURE_2D, texture, 0);
	this->textures.push_back(texture);
}

void GLFramebuffer::attachRenderbuffer(GLenum attachment, GLenum internalFormat, GLsizei width, GLsizei height) {
	GLuint rbo;
	glGenRenderbuffers(1, &rbo);

	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height);

	glFramebufferRenderbuffer(this->currentTarget, attachment, GL_RENDERBUFFER, rbo);
	this->renderbuffers.push_back(rbo);
}

void GLFramebuffer::bind(GLenum target) {
	this->currentTarget = target;
	glBindFramebuffer(target, this->handle);
}

void GLFramebuffer::bindTextures() {
	GLuint active = GL_TEXTURE0;
	for (auto texture : textures) {
		glActiveTexture(active++);
		glBindTexture(GL_TEXTURE_2D, texture);
	}
}

void GLFramebuffer::unbind() {
	glBindFramebuffer(this->currentTarget, 0);
}

void GLFramebuffer::unbindTextures() {
	GLuint active = GL_TEXTURE0;
	for (auto texture : textures) {
		glActiveTexture(active++);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glActiveTexture(GL_TEXTURE0);
}

GLenum GLFramebuffer::getStatus() const {
	return glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

GLuint GLFramebuffer::getHandle() const {
	return this->handle;
}

GLuint GLFramebuffer::getTexture(std::vector<GLuint>::size_type index) const {
	assert(index < this->textures.size());

	return this->textures[index];
}

GLuint GLFramebuffer::getRenderbuffer(std::vector<GLuint>::size_type index) const {
	assert(index < this->renderbuffers.size());

	return this->renderbuffers[index];
}