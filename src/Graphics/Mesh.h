#ifndef MESH_H
#define MESH_H

#include <GL/glew.h>

#include <string>
#include <vector>
#include <map>

#include <tiny_obj_loader.h>

typedef struct {
    GLuint vao, buf;
    GLsizei vertexCount;
    size_t material_id;
} Drawable;

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
};

#endif
