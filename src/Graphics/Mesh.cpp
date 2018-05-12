#include "Mesh.h"

#include <Graphics/opengl.h>
#include <Graphics/GLHelper.h>
#include <Graphics/GLTexture2D.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <chrono>
#include <limits>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stb_image.h>

#include <unordered_map>
#include <common.h>

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
        LOG_ERROR(err);
    }

    if (!status) {
        LOG_ERROR("Failed to load mesh: ", meshname);
    }
    else {
        auto start = chrono::high_resolution_clock::now();

        // Load materials and store name->id map
        for (material_t &mp : materials) {
            // Material m;
            // for (int i = 0; i < 3; i++) {
            //     m.ambient[i] = mp.ambient[i];
            //     m.diffuse[i] = mp.diffuse[i];
            //     m.specular[i] = mp.specular[i];
            // }
            // m.shininess = mp.shininess;
            // m.hasAmbientMap = !mp.diffuse_texname.empty();
            // m.hasDiffuseMap = !mp.ambient_texname.empty();
            // m.hasSpecularMap = !mp._texname.empty();
            // m.hasAlphaMap = !mp.diffuse_texname.empty();
            // m.hasNormalMap = !mp.diffuse_texname.empty();

            auto loadImage = [&] (const std::string &path) {
                string texture_name = path;
                convertPathFromWindows(texture_name);

                texture_name = basedir + texture_name;
                GLuint texture_id = GLHelper::createTextureFromImage(texture_name);
                GLTexture2D texture {texture_id};

                textures.insert(make_pair(path, std::move(texture)));
            };

            if (!mp.diffuse_texname.empty() && textures.find(mp.diffuse_texname) == textures.end()) {
                loadImage(mp.diffuse_texname);
                LOG_INFO("loaded diffuse ", mp.diffuse_texname);
            }

            if (!mp.specular_texname.empty() && textures.find(mp.specular_texname) == textures.end()) {
                loadImage(mp.specular_texname);
                LOG_INFO("loaded specular ", mp.specular_texname);
            }

            if (!mp.bump_texname.empty() && textures.find(mp.bump_texname) == textures.end()) {
                loadImage(mp.bump_texname);
                LOG_INFO("loaded normal map ", mp.bump_texname);
            }

            if (!mp.specular_highlight_texname.empty() && textures.find(mp.specular_highlight_texname) == textures.end()) {
                loadImage(mp.specular_highlight_texname);
                LOG_INFO("loaded roughness map ", mp.specular_highlight_texname);
            }

            if (!mp.ambient_texname.empty() && textures.find(mp.ambient_texname) == textures.end()) {
                loadImage(mp.ambient_texname);
                LOG_INFO("loaded metallic map ", mp.ambient_texname);
            }
        }

        // Default material
        {
            material_t default_material;
            default_material.name = "default";
            default_material.diffuse_texname = DEFAULT_TEXTURE;
            default_material.shininess = 1.0f;
                        
            materials.push_back(default_material);

            GLuint texture_id = GLHelper::createTextureFromImage(string(RESOURCE_DIR) + default_material.diffuse_texname);
            GLTexture2D texture {texture_id};
            textures.insert(make_pair(default_material.diffuse_texname, std::move(texture)));
        }

        drawables.resize(materials.size());
        for (size_t i = 0; i < drawables.size(); i++) {
            drawables[i].material_id = i;
        }

        min = glm::vec3(numeric_limits<float>::max());
        max = glm::vec3(numeric_limits<float>::min());

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
                int material_id = shape.mesh.material_ids[f];
                auto &d = drawables[material_id == -1 ? drawables.size() - 1 : material_id];

                // Store pointers to each vertex so we can process per-face attributes later
                vector<size_t> tri (fv, 0);

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

            min = glm::min(min, v.position);
            max = glm::max(max, v.position);
        }

        glm::vec3 extents = max - min;
        radius = glm::max(glm::max(extents.x, extents.y), extents.z) / 2.0f;

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

        {
            glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboOffsetAlignment);
            LOG_DEBUG("uboOffsetAlignment: ", uboOffsetAlignment);

            glGenBuffers(1, &materialUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, materialUBO);
            glBufferData(GL_UNIFORM_BUFFER, std::max(80, uboOffsetAlignment) * materials.size(), nullptr, GL_STATIC_DRAW);

            for (const auto &d : drawables) {
                const auto &m = materials[d.material_id];

                GLuint hasAmbientMap = false;
                GLuint hasDiffuseMap = textures.count(m.diffuse_texname) != 0;
                GLuint hasSpecularMap = textures.count(m.specular_texname) != 0;
                GLuint hasAlphaMap = false;
                GLuint hasNormalMap = textures.count(m.bump_texname) != 0;
                GLuint hasRoughnessMap = textures.count(m.specular_highlight_texname) != 0;
                GLuint hasMetallicMap = textures.count(m.ambient_texname) != 0;

                GLuint materialOffset = std::max(80, uboOffsetAlignment) * d.material_id;

                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset +  0, 12, m.ambient);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 16, 12, m.diffuse);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 32, 12, m.specular);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 44,  4, &m.shininess);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 48,  4, &hasAmbientMap);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 52,  4, &hasDiffuseMap);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 56,  4, &hasSpecularMap);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 60,  4, &hasAlphaMap);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 64,  4, &hasNormalMap);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 68,  4, &hasRoughnessMap);
                glBufferSubData(GL_UNIFORM_BUFFER, materialOffset + 72,  4, &hasMetallicMap);
            }

            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double> diff = end - start;
        LOG_INFO(
            "\n\tLoaded mesh ", meshname, " in ", diff.count(), " seconds",
            "\n\t# of vertices  = ", (int)(attrib.vertices.size()) / 3,
            "\n\t# of normals   = ", (int)(attrib.normals.size()) / 3,
            "\n\t# of texcoords = ", (int)(attrib.texcoords.size()) / 2,
            "\n\t# of materials = ", (int)materials.size(),
            "\n\t# of shapes    = ", (int)shapes.size(),
            "\n\tmin = ", glm::to_string(min),
            "\n\tmax = ", glm::to_string(max),
            "\n\tradius = ", radius
        );
    }
}

void Mesh::draw(GLuint program) const {
    glBindVertexArray(vao);
    for (const auto &d : drawables) {
        const auto &m = materials[d.material_id];

        GLuint default_texture = textures.at(DEFAULT_TEXTURE).handle;
        bool hasDiffuseMap = textures.count(m.diffuse_texname) != 0;
        bool hasSpecularMap = textures.count(m.specular_texname) != 0;
        bool hasNormalMap = textures.count(m.bump_texname) != 0;
        bool hasRoughnessMap = textures.count(m.specular_highlight_texname) != 0;
        bool hasMetallicMap = textures.count(m.ambient_texname) != 0;

        GLuint diffuse_texture = hasDiffuseMap ? textures.at(m.diffuse_texname).handle : default_texture;
        GLuint specular_texture = hasSpecularMap ? textures.at(m.specular_texname).handle : 0;
        GLuint bump_texture = hasNormalMap ? textures.at(m.bump_texname).handle : 0;
        GLuint roughness_texture = hasRoughnessMap ? textures.at(m.specular_highlight_texname).handle : 0;
        GLuint metallic_texture = hasMetallicMap ? textures.at(m.ambient_texname).handle : 0;

        glBindTextureUnit(0, diffuse_texture);
        glBindTextureUnit(1, specular_texture);
        glBindTextureUnit(5, bump_texture);
        glBindTextureUnit(7, roughness_texture);
        glBindTextureUnit(8, metallic_texture);

        glUniform3fv(glGetUniformLocation(program, "material.ambient"), 1, m.ambient);
        glUniform3fv(glGetUniformLocation(program, "material.diffuse"), 1, m.diffuse);
        glUniform3fv(glGetUniformLocation(program, "material.specular"), 1, m.specular);
        glUniform1f(glGetUniformLocation(program, "material.shininess"), m.shininess);
        glUniform1i(glGetUniformLocation(program, "material.hasAmbientMap"), false);
        glUniform1i(glGetUniformLocation(program, "material.hasDiffuseMap"), hasDiffuseMap);
        glUniform1i(glGetUniformLocation(program, "material.hasSpecularMap"), hasSpecularMap);
        glUniform1i(glGetUniformLocation(program, "material.hasAlphaMap"), false);
        glUniform1i(glGetUniformLocation(program, "material.hasNormalMap"), hasNormalMap);
        glUniform1i(glGetUniformLocation(program, "material.hasRoughnessMap"), hasRoughnessMap);
        glUniform1i(glGetUniformLocation(program, "material.hasMetallicMap"), hasMetallicMap);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d.ebo);
        glDrawElements(GL_TRIANGLES, d.indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(5, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::draw(GLShaderProgram &program) const {
    if (program.getObjectLabel() == "Phong") {
        GLuint uboIndex = glGetUniformBlockIndex(program.getHandle(), "MaterialBlock");
        // LOG_DEBUG("uboIndex: ", uboIndex);
        glUniformBlockBinding(program.getHandle(), uboIndex, 0);
    }

#if 0
    static GLuint ubo = 0;
    if (ubo == 0 && program.getObjectLabel() == "Phong") {
        LOG_DEBUG("Initializing ubo");
        GLint maxUBOs;
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUBOs);
        LOG_DEBUG("Max ubos: ", maxUBOs);

        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, 72, nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        GLuint uboIndex = glGetUniformBlockIndex(program.getHandle(), "MaterialBlock");
        LOG_DEBUG("uboIndex: ", uboIndex);
        glUniformBlockBinding(program.getHandle(), uboIndex, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
        // glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo, 0, 72);

        GLint numBlocks = 0;
        glGetProgramInterfaceiv(program.getHandle(), GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numBlocks);
        const GLenum blockProperties[1] = {GL_NUM_ACTIVE_VARIABLES};
        const GLenum activeUnifProp[1] = {GL_ACTIVE_VARIABLES};
        const GLenum unifProperties[3] = {GL_NAME_LENGTH, GL_TYPE, GL_OFFSET};

        LOG_DEBUG("# Uniform blocks:", numBlocks);
        for(int blockIx = 0; blockIx < numBlocks; ++blockIx) {
            GLint numActiveUnifs = 0;
            glGetProgramResourceiv(program.getHandle(), GL_UNIFORM_BLOCK, blockIx, 1, blockProperties, 1, NULL, &numActiveUnifs);

            if(!numActiveUnifs)
                continue;

            LOG_DEBUG("# Active uniforms: ", numActiveUnifs);
            std::vector<GLint> blockUnifs(numActiveUnifs);
            glGetProgramResourceiv(program.getHandle(), GL_UNIFORM_BLOCK, blockIx, 1, activeUnifProp, numActiveUnifs, NULL, &blockUnifs[0]);

            for(int unifIx = 0; unifIx < numActiveUnifs; ++unifIx) {
                GLint values[3];
                glGetProgramResourceiv(program.getHandle(), GL_UNIFORM, blockUnifs[unifIx], 3, unifProperties, 3, NULL, values);

                // Get the name. Must use a std::vector rather than a std::string for C++03 standards issues.
                // C++11 would let you use a std::string directly.
                std::vector<char> nameData(values[0]);
                glGetProgramResourceName(program.getHandle(), GL_UNIFORM, blockUnifs[unifIx], nameData.size(), NULL, &nameData[0]);
                std::string name(nameData.begin(), nameData.end() - 1);

                LOG_DEBUG(name, " ", values[0], " ", values[1], " ", values[2]);
            }
        }
    }
#endif

    glBindVertexArray(vao);
    for (const auto &d : drawables) {
        const auto &m = materials[d.material_id];

        GLuint default_texture = textures.at(DEFAULT_TEXTURE).handle;
        bool hasDiffuseMap = textures.count(m.diffuse_texname) != 0;
        bool hasSpecularMap = textures.count(m.specular_texname) != 0;
        bool hasNormalMap = textures.count(m.bump_texname) != 0;
        bool hasRoughnessMap = textures.count(m.specular_highlight_texname) != 0;
        bool hasMetallicMap = textures.count(m.ambient_texname) != 0;

        GLuint diffuse_texture = hasDiffuseMap ? textures.at(m.diffuse_texname).handle : default_texture;
        GLuint specular_texture = hasSpecularMap ? textures.at(m.specular_texname).handle : 0;
        GLuint bump_texture = hasNormalMap ? textures.at(m.bump_texname).handle : 0;
        GLuint roughness_texture = hasRoughnessMap ? textures.at(m.specular_highlight_texname).handle : 0;
        GLuint metallic_texture = hasMetallicMap ? textures.at(m.ambient_texname).handle : 0;

        glBindTextureUnit(0, diffuse_texture);
        glBindTextureUnit(1, specular_texture);
        glBindTextureUnit(5, bump_texture);
        glBindTextureUnit(7, roughness_texture);
        glBindTextureUnit(8, metallic_texture);

        if (program.getObjectLabel() == "Phong") {
            glBindBufferRange(GL_UNIFORM_BUFFER, 0, materialUBO, std::max(80, uboOffsetAlignment) * d.material_id, 80);
        }
        else {
            program.setUniform3fv("material.ambient", 1, m.ambient);
            program.setUniform3fv("material.diffuse", 1, m.diffuse);
            program.setUniform3fv("material.specular", 1, m.specular);
            program.setUniform1f("material.shininess", m.shininess);
            program.setUniform1i("material.hasAmbientMap", false);
            program.setUniform1i("material.hasDiffuseMap", hasDiffuseMap);
            program.setUniform1i("material.hasSpecularMap", hasSpecularMap);
            program.setUniform1i("material.hasAlphaMap", false);
            program.setUniform1i("material.hasNormalMap", hasNormalMap);
            program.setUniform1i("material.hasRoughnessMap", hasRoughnessMap);
            program.setUniform1i("material.hasMetallicMap", hasMetallicMap);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d.ebo);
        glDrawElements(GL_TRIANGLES, d.indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(5, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
