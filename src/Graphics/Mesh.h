#ifndef MESH_H
#define MESH_H

#include <Graphics/opengl.h>
#include <Graphics/GLShaderProgram.h>
#include <Graphics/GLTexture2D.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <tiny_obj_loader.h>
#include <common.h>

struct Material {
    glm::vec3 ambient, diffuse, specular;
    float shininess;

    float roughness;
    float metallic;

    GLuint ambient_map, diffuse_map, specular_map, alpha_map;
    GLuint normal_map, roughness_map, metallic_map;

    Material() :
        ambient(glm::vec3(0)), diffuse(glm::vec3(0)), specular(glm::vec3(0)),
        shininess(32.0f), roughness(1.0f), metallic(0.0f),
        ambient_map(0), diffuse_map(0), specular_map(0), alpha_map(0),
        normal_map(0), roughness_map(0), metallic_map(0)
        {}

    Material(tinyobj::material_t m) : Material() {
        for (int i = 0; i < 3; i++) {
            this->ambient[i] = ambient[i];
            this->diffuse[i] = diffuse[i];
            this->specular[i] = specular[i];
        }
        this->shininess = shininess;
        this->roughness = roughness;
        this->metallic = metallic;
    }

    void writeUBO(unsigned int offset) const {
        glBufferSubData(GL_UNIFORM_BUFFER, offset +  0, 12, &ambient);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 16, 12, &diffuse);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 32, 12, &specular);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 44,  4, &shininess);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 48,  4, &ambient_map);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 52,  4, &diffuse_map);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 56,  4, &specular_map);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 60,  4, &alpha_map);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 64,  4, &normal_map);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 68,  4, &roughness_map);
        glBufferSubData(GL_UNIFORM_BUFFER, offset + 72,  4, &metallic_map);
    }

    static GLint getAlignment() {
        static GLint uboOffsetAlignment = 0;
        if (uboOffsetAlignment == 0) {
            glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboOffsetAlignment);
            LOG_DEBUG("uboOffsetAlignment: ", uboOffsetAlignment);
        }

        return std::max((GLint)Material::glslSize, uboOffsetAlignment);
    }

    static const unsigned int glslSize = 80;
};

struct Vertex {
    glm::vec3 position, normal;
    glm::vec2 texcoord;
    glm::vec3 tangent, bitangent;
};

struct Drawable {
    size_t material_id;
    std::vector<GLuint> indices;
    GLuint ebo;
};

class Mesh {
public:
    Mesh(const std::string &meshname);

    void draw(GLShaderProgram &program, GLenum mode = GL_TRIANGLES) const;

    void loadMesh(const std::string &meshname);

    const glm::vec3 &getMin() const { return min; }
    const glm::vec3 &getMax() const { return max; }
    glm::vec3 getExtents() const { return max - min; }
    float getRadius() const { return radius; }

private:
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::vector<Material> mats; // TODO temporary, will remove material_t

    std::map<std::string, GLTexture2D> textures;
    std::vector<Drawable> drawables;
    std::vector<Vertex> vertices;

    glm::vec3 min, max;
    float radius;

    GLuint vao, materialUBO;
};

#endif
