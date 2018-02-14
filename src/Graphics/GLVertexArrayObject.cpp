#include <Graphics/opengl.h>
#include "GLVertexArrayObject.h"

GLVertexArrayObject::GLVertexArrayObject() {
    glGenVertexArrays(1, &handle);
}

GLVertexArrayObject::~GLVertexArrayObject() {
    glDeleteVertexArrays(1, &handle);
}

void GLVertexArrayObject::addAttribute(GLuint location, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *offset) {
    this->bind();
    glVertexAttribPointer(location, size, type, normalized, stride, offset);
    glEnableVertexAttribArray(location);
}

void GLVertexArrayObject::bind() const {
    glBindVertexArray(handle);
}

void GLVertexArrayObject::unbind() const {
    glBindVertexArray(0);
}
