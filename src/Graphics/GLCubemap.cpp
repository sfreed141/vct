#include "GLCubemap.h"
#include <iostream>
#include <stb_image.h>

GLCubemap::GLCubemap() : handle(0), texunit(GL_TEXTURE0) {}

GLCubemap::GLCubemap(const std::vector<std::string> &texnames) {
    this->load(texnames);
    this->texunit = GL_TEXTURE0;
}

GLCubemap::~GLCubemap() {
    glDeleteTextures(1, &handle);
}

void GLCubemap::load(const std::vector<std::string> &texnames) {
    if (handle != 0) {
        glDeleteTextures(1, &handle);
    }

    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

    int width, height, channels;
    unsigned char *image;

    for (int i = 0; i < 6; i++) {
        image = stbi_load(texnames[i].c_str(), &width, &height, &channels, 3);
        if (image == NULL) {
            std::cout
                << "ERROR::CUBEMAP::LOAD_FAILED::"
                << texnames[i]
                << std::endl;
        }
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);

        stbi_image_free(image);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void GLCubemap::setTextureUnit(GLenum texunit) {
    this->texunit = texunit;
}

void GLCubemap::bind() const {
    glActiveTexture(this->texunit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, this->handle);
}

void GLCubemap::unbind() const {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}