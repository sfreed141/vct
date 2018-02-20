#include "Mesh.h"

#include <Graphics/opengl.h>
#include <Graphics/GLHelper.h>
#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cmath>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stb_image.h>

#include <unordered_map>

using namespace std;
using namespace tinyobj;

static const char *DEFAULT_TEXTURE = "default_texture.png";

void convertPathFromWindows(std::string &str) {
#ifndef _WIN32
    replace(begin(str), end(str), '\\', '/');
#endif
}

Mesh::Mesh(const std::string &meshname) {
    loadMesh(meshname);
}

// modified from https://github.com/syoyo/tinyobjloader/blob/master/examples/viewer/viewer.cc
void Mesh::loadMesh(const std::string &meshname) {
    string err;
    string basedir = meshname.substr(0, meshname.find_last_of('/') + 1);
    bool status = LoadObj(&attrib, &shapes, &materials, &err, meshname.c_str(), basedir.c_str());

    if (!err.empty()) {
        cerr << err << endl;
    }

    if (!status) {
        cerr << "Failed to load mesh: " << meshname << endl;
    }
    else {
        printf("Loaded mesh %s\n", meshname.c_str());
        printf("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
        printf("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
        printf("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
        printf("# of materials = %d\n", (int)materials.size());
        printf("# of shapes    = %d\n", (int)shapes.size());

        // Load materials and store name->id map
        for (material_t &mp : materials) {
            if (!mp.diffuse_texname.empty() && textures.find(mp.diffuse_texname) == textures.end()) {
                string texture_name = mp.diffuse_texname;
                convertPathFromWindows(texture_name);

                texture_name = basedir + texture_name;
                GLuint texture_id = GLHelper::createTextureFromImage(texture_name);
                
                textures.insert(make_pair(mp.diffuse_texname, texture_id));
                std::cerr << "loaded diffuse " << mp.diffuse_texname << std::endl;
            }

            if (!mp.bump_texname.empty() && textures.find(mp.bump_texname) == textures.end()) {
                string texture_name = mp.bump_texname;
                convertPathFromWindows(texture_name);

                texture_name = basedir + texture_name;
                int width, height, channels;
                unsigned char *image = stbi_load(texture_name.c_str(), &width, &height, &channels, STBI_default);
                if (image == nullptr) {
                    std::cerr << "ERROR::TEXTURE::LOAD_FAILED::" << texture_name << std::endl;
                    continue;
                }
                else if (channels == 3 || channels == 4) {
                    GLuint texture_id;
                    glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);
                    
                    glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                    glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
                    glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

                    if (GLAD_GL_EXT_texture_filter_anisotropic) {
                        float maxAnisotropy = 1.0f;
                        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
                        glTextureParameterf(texture_id, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
                    }

                    GLint levels = (GLint)std::log2(std::fmax(width, height)) + 1;
                    if (channels == 3) {
                        glTextureStorage2D(texture_id, levels, GL_RGB8, width, height);
                        glTextureSubImage2D(texture_id, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
                    }
                    else if (channels == 4) {
                        glTextureStorage2D(texture_id, levels, GL_RGBA8, width, height);
                        glTextureSubImage2D(texture_id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image); 
                    }

                    glGenerateTextureMipmap(texture_id);

                    textures.insert(make_pair(mp.bump_texname, texture_id));
                    std::cerr << "loaded normal map " << mp.bump_texname << std::endl;
                }
                else {
                    std::cerr << "Bump map " << texture_name << " not supported, convert it to normal map" << std::endl;
                }
                stbi_image_free(image);
            }
        }

        // Default material
        {
            material_t default_material;
            default_material.name = "default";
            default_material.diffuse_texname = DEFAULT_TEXTURE;
            materials.push_back(default_material);

            GLuint texture_id = GLHelper::createTextureFromImage(string(RESOURCE_DIR) + default_material.diffuse_texname);
            textures.insert(make_pair(default_material.diffuse_texname, texture_id));
        }

        drawables.resize(materials.size());
        for (size_t i = 0; i < drawables.size(); i++) {
            drawables[i].material_id = i;
        }

        unordered_map<string, size_t> vertexMap;
        vertexMap.clear();
        vertices.clear();
        for (const auto &shape : shapes) {
            size_t index_offset = 0;

            // Loop through each face
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                // Number of vertices per face (should always be 3 since triangulate=true)
                unsigned char fv = shape.mesh.num_face_vertices[f];
                assert(fv == 3);

                // Per face material_id
                size_t material_id = shape.mesh.material_ids[f];
                auto &d = drawables[material_id];

                // Store pointers to each vertex so we can process per-face attributes later
                vector<size_t> tri {fv, 0};

                // Loop through each vertex of the current face
                for (size_t v = 0; v < fv; v++) {
                    index_t index = shape.mesh.indices[index_offset + v];
                    // assert(index.vertex_index == index.normal_index && index.normal_index == index.texcoord_index);
                    string index_str =
                        to_string(index.vertex_index) + string("_")
                        + to_string(index.normal_index) + string("_")
                        + to_string(index.texcoord_index);
                    size_t vertIndex = 0;
                    if (vertexMap.count(index_str) == 0) {
                        // new vertex, add to map and load positions, normals, texcoords
                        vertIndex = vertices.size();
                        vertexMap.insert(make_pair(index_str, vertIndex));
                        vertices.push_back(Vertex{});
                        tri[v] = vertIndex;
                        
                        vertices[tri[v]].position[0] = attrib.vertices[3 * index.vertex_index];
                        vertices[tri[v]].position[1] = attrib.vertices[3 * index.vertex_index + 1];
                        vertices[tri[v]].position[2] = attrib.vertices[3 * index.vertex_index + 2];
                        if (index.normal_index >= 0) {
                            vertices[tri[v]].normal[0] = attrib.normals[3 * index.normal_index];
                            vertices[tri[v]].normal[1] = attrib.normals[3 * index.normal_index + 1];
                            vertices[tri[v]].normal[2] = attrib.normals[3 * index.normal_index + 2];
                        }
                        if (index.texcoord_index >= 0) {
                            vertices[tri[v]].texcoord[0] = attrib.texcoords[2 * index.texcoord_index];
                            vertices[tri[v]].texcoord[1] = attrib.texcoords[2 * index.texcoord_index + 1];
                        }
                    }
                    else {
                        // existing vertex 
                        vertIndex = vertexMap.at(index_str);
                        tri[v] = vertIndex;
                    }
                    d.indices.push_back(vertIndex);
                }

                // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
                glm::vec3 edge1 = vertices[tri[1]].position - vertices[tri[0]].position;
                glm::vec3 edge2 = vertices[tri[2]].position - vertices[tri[0]].position;
                glm::vec2 deltaUV1 = vertices[tri[1]].texcoord - vertices[tri[0]].texcoord;
                glm::vec2 deltaUV2 = vertices[tri[2]].texcoord - vertices[tri[0]].texcoord;
                float invDeterminant = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                glm::vec3 tangent, bitangent;
                tangent.x = invDeterminant * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
                tangent.y = invDeterminant * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
                tangent.z = invDeterminant * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
                bitangent.x = invDeterminant * (deltaUV2.x * edge1.x - deltaUV1.x * edge2.x);
                bitangent.y = invDeterminant * (deltaUV2.x * edge1.y - deltaUV1.x * edge2.y);
                bitangent.z = invDeterminant * (deltaUV2.x * edge1.z - deltaUV1.x * edge2.z);
                for (size_t v = 0; v < 3; v++) {
                    vertices[tri[v]].tangent += tangent;
                    vertices[tri[v]].bitangent += bitangent;
                }

                index_offset += fv;
            }
        }

        for (Vertex &v : vertices) {
            v.tangent = glm::normalize(v.tangent);
            v.bitangent = glm::normalize(v.bitangent);
        }

        for (Drawable &d : drawables) {
            glGenBuffers(1, &d.ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d.ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, d.indices.size() * sizeof(GLuint), d.indices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        GLuint buf = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &buf);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, buf);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, normal));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, texcoord));
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, tangent));
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, bitangent));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void Mesh::draw(GLuint program) const {
    GLint enableNormalMapLocation = glGetUniformLocation(program, "enableNormalMap");
    GLint enableNormalMap = 0;
    if  (enableNormalMapLocation >= 0)
        glGetUniformiv(program, enableNormalMapLocation, &enableNormalMap);

    glBindVertexArray(vao);
    for (const auto &d : drawables) {
        const auto &diffuse_texname = materials[d.material_id].diffuse_texname;
        const auto &bump_texname = materials[d.material_id].bump_texname;

        GLuint diffuse_texture = textures.count(diffuse_texname) == 0 ? textures.at(DEFAULT_TEXTURE) : textures.at(diffuse_texname);
        GLuint bump_texture;
        if (textures.count(bump_texname) == 0) {
            bump_texture = 0;
            glUniform1i(enableNormalMapLocation, false);
        }
        else {
            bump_texture = textures.at(bump_texname);
            glUniform1i(enableNormalMapLocation, enableNormalMap);
        }

        glBindTextureUnit(0, diffuse_texture);
        glBindTextureUnit(5, bump_texture);
        glUniform1f(glGetUniformLocation(program, "shine"), materials[d.material_id].shininess);
        // glUniform3fv(glGetUniformLocation(program, "material.diffuse"), materials[d.material_id].shininess);
        // glUniform3fv(glGetUniformLocation(program, "material.ambient"), materials[d.material_id].shininess);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d.ebo);
        glDrawElements(GL_TRIANGLES, d.indices.size(), GL_UNSIGNED_INT, 0);
        // glDrawElements(GL_TRIANGLES, d.indices.size(), GL_UNSIGNED_INT, d.indices.data());
    }

    if  (enableNormalMapLocation >= 0)
        glUniform1i(enableNormalMapLocation, enableNormalMap);

    glBindTextureUnit(0, 0);
    glBindTextureUnit(5, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
