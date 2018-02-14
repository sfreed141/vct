#ifndef GLQUAD_H
#define GLQUAD_H

#include <glad/glad.h>

class GLQuad {
public:
	static void init();
	static void draw();
private:
	static GLuint vao;
};

#endif