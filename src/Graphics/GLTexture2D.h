#ifndef GLTEXTURE2D_H
#define GLTEXTURE2D_H

#include "opengl.h"
#include <string>

class GLTexture2D {
public:
    GLTexture2D() { glCreateTextures(GL_TEXTURE_2D, 1, &handle); }
    GLTexture2D(GLuint handle) : handle(handle) {}
    ~GLTexture2D() { glDeleteTextures(1, &handle); }

    GLTexture2D(const GLTexture2D &other) = delete;
    GLTexture2D &operator=(const GLTexture2D &other) = delete;

    GLTexture2D(GLTexture2D &&other) :
        handle(other.handle),
        width(other.width), height(other.height),
        components(other.components), levels(other.levels), format(other.format)
    {
        other.handle = 0;
    }

    GLTexture2D &operator=(GLTexture2D &&other) {
        if (this != &other) {
            glDeleteTextures(1, &handle);

            handle = other.handle;
            width = other.width;
            height = other.height;
            components = other.components;
            levels = other.levels;
            format = other.format;

            other.handle = 0;
        }

        return *this;
    }

    void setParameteri(GLenum pname, GLint param) { glTextureParameteri(handle, pname, param); }
    void setParameterf(GLenum pname, GLfloat param) { glTextureParameterf(handle, pname, param); }

    void bindTextureUnit(GLuint unit) { glBindTextureUnit(unit, handle); }

    GLuint handle = 0;
    unsigned int width, height, components, levels;
    GLenum format;
};

#endif
