#ifndef MESH_H
#define MESH_H

#include <Graphics/opengl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <map>

#include <tiny_obj_loader.h>

struct Material {
    glm::vec3 ambient, diffuse, specular;
    float shininess;

    bool hasAmbientMap, hasDiffuseMap, hasSpecularMap, hasAlphaMap;
    bool hasNormalMap;

    Material() :
        ambient(glm::vec3(0)), diffuse(glm::vec3(0)), specular(glm::vec3(0)),
        shininess(32.0f),
        hasAmbientMap(false), hasDiffuseMap(false), hasSpecularMap(false), hasAlphaMap(false),
        hasNormalMap(false)
        {}
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

    void draw(GLuint program) const;

    void loadMesh(const std::string &meshname);

    const glm::vec3 &getMin() const { return min; }
    const glm::vec3 &getMax() const { return max; }
    glm::vec3 getExtents() const { return max - min; }
    float getRadius() const { return radius; }

private:
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::map<std::string, GLuint> textures;
    std::vector<Drawable> drawables;
    std::vector<Vertex> vertices;

    glm::vec3 min, max;
    float radius;

    GLuint vao;
};

#endif
