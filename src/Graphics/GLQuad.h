#ifndef GLQUAD_H
#define GLQUAD_H

#include <Graphics/opengl.h>

class GLQuad {
public:
    static void init();
    static void draw(GLenum mode = GL_TRIANGLES);
private:
    static GLuint vao;
};

#endif