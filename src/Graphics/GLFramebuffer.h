#ifndef GLFRAMEBUFFER_H
#define GLFRAMEBUFFER_H

#include <Graphics/opengl.h>
#include <glm/glm.hpp>
#include <vector>

class GLFramebuffer {
public:
	GLFramebuffer();
	~GLFramebuffer();

	void attachTexture(GLenum attachment, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint minFilter = GL_NEAREST, GLint magFilter = GL_NEAREST, GLint wrapS = GL_REPEAT, GLint wrapT = GL_REPEAT, const glm::vec4 *borderColor = nullptr);
	void attachRenderbuffer(GLenum attachment, GLenum internalFormat, GLsizei width, GLsizei height);

	void bind(GLenum target = GL_FRAMEBUFFER);
	void bindTextures();
	void unbind();
	void unbindTextures();

	GLenum getStatus() const;
	GLuint getHandle() const;
	GLuint getTexture(std::vector<GLuint>::size_type index) const;
	GLuint getRenderbuffer(std::vector<GLuint>::size_type index) const;
private:
	GLuint handle;
	GLenum currentTarget;
	std::vector<GLuint> textures;
	std::vector<GLuint> renderbuffers;
};

#endif