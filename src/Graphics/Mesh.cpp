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

        mats.clear();
        mats.reserve(materials.size() + 1);
        // Load materials and store name->id map
        for (material_t &mp : materials) {
            Material m {mp};

            auto loadImage = [&] (const std::string &path) {
                if (textures.find(path) != textures.end()) {
                    return textures[path].handle;
                }

                string texture_name = path;
                convertPathFromWindows(texture_name);

                texture_name = basedir + texture_name;
                GLuint texture_id = GLHelper::createTextureFromImage(texture_name);
                GLTexture2D texture {texture_id};

                textures.insert(make_pair(path, std::move(texture)));

                LOG_INFO("loaded texture ", path);
                return texture_id;
            };

            if (!mp.diffuse_texname.empty()) { m.diffuse_map = loadImage(mp.diffuse_texname); }
            if (!mp.specular_texname.empty()) { m.specular_map = loadImage(mp.specular_texname); }
            if (!mp.normal_texname.empty()) { m.normal_map = loadImage(mp.normal_texname); }
            if (!mp.roughness_texname.empty()) { m.roughness_map = loadImage(mp.roughness_texname); }
            if (!mp.metallic_texname.empty()) { m.metallic_map = loadImage(mp.metallic_texname); }
            if (!mp.alpha_texname.empty()) { m.alpha_map = loadImage(mp.alpha_texname); }

            mats.push_back(m);
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

            Material m {default_material};
            m.diffuse_map = texture_id;
            mats.push_back(m);
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
                            vertices[tri[v]].texcoord[1] = 1.f - attrib.texcoords[2 * index.texcoord_index + 1];
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
            glGenBuffers(1, &materialUBO);
            glBindBuffer(GL_UNIFORM_BUFFER, materialUBO);
            glBufferData(GL_UNIFORM_BUFFER, Material::getAlignment() * mats.size(), nullptr, GL_STATIC_DRAW);

            for (const auto &d : drawables) {
                const auto &m = mats[d.material_id];

                GLuint materialOffset = Material::getAlignment() * d.material_id;

                m.writeUBO(materialOffset);
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

void Mesh::draw(GLShaderProgram &program, GLenum mode) const {
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
        const auto &m = mats[d.material_id];

        GLuint default_texture = textures.at(DEFAULT_TEXTURE).handle;
        bool hasDiffuseMap = m.diffuse_map != 0;
        bool hasSpecularMap = m.specular_map != 0;
        bool hasNormalMap = m.normal_map != 0;
        bool hasRoughnessMap = m.roughness_map != 0;
        bool hasMetallicMap = m.ambient_map != 0;
        bool hasAlphaMap = m.alpha_map != 0;

        glBindTextureUnit(0, m.diffuse_map);
        glBindTextureUnit(1, m.specular_map);
        glBindTextureUnit(5, m.normal_map);
        glBindTextureUnit(7, m.roughness_map);
        glBindTextureUnit(8, m.metallic_map);
        glBindTextureUnit(9, m.alpha_map);

        if (program.getObjectLabel() == "Phong") {
            glBindBufferRange(GL_UNIFORM_BUFFER, 0, materialUBO, Material::getAlignment() * d.material_id, Material::glslSize);
        }
        else {
            program.setUniform3fv("material.ambient", m.ambient);
            program.setUniform3fv("material.diffuse", m.diffuse);
            program.setUniform3fv("material.specular", m.specular);
            program.setUniform1f("material.shininess", m.shininess);
            program.setUniform1i("material.hasAmbientMap", false);
            program.setUniform1i("material.hasDiffuseMap", hasDiffuseMap);
            program.setUniform1i("material.hasSpecularMap", hasSpecularMap);
            program.setUniform1i("material.hasAlphaMap", hasAlphaMap);
            program.setUniform1i("material.hasNormalMap", hasNormalMap);
            program.setUniform1i("material.hasRoughnessMap", hasRoughnessMap);
            program.setUniform1i("material.hasMetallicMap", hasMetallicMap);
            program.setUniform1i("material.hasAlphaMap", hasAlphaMap);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, d.ebo);
        glDrawElements(mode, d.indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindTextureUnit(0, 0);
    glBindTextureUnit(1, 0);
    glBindTextureUnit(5, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
