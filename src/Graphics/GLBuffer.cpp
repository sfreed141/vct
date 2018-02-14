#include <glad/glad.h>
#include "GLBuffer.h"

GLBuffer::GLBuffer() {
    glGenBuffers(1, &handle);
}

GLBuffer::~GLBuffer() {
    glDeleteBuffers(1, &handle);
    handle = 0;
}

void GLBuffer::loadData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {
    this->target = target;
    glBindBuffer(target, handle);
    glBufferData(target, size, data, usage);
}

void GLBuffer::bind() const {
    glBindBuffer(target, handle);
}

void GLBuffer::unbind() const {
    glBindBuffer(target, 0);
}