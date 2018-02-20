#ifndef MESH_H
#define MESH_H

#include <Graphics/opengl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <map>

#include <tiny_obj_loader.h>

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

private:
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::map<std::string, GLuint> textures;
    std::vector<Drawable> drawables;
    std::vector<Vertex> vertices;

    GLuint vao;
};

#endif
