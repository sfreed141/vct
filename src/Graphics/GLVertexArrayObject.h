#ifndef GLVERTEXARRAYOBJECT_H
#define GLVERTEXARRAYOBJECT_H

#include <Graphics/opengl.h>

class GLVertexArrayObject {
public:
    GLVertexArrayObject();
    ~GLVertexArrayObject();

    void addAttribute(GLuint location, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *offset);

    void bind() const;
    void unbind() const;
private:
    GLuint handle;
};

#endif