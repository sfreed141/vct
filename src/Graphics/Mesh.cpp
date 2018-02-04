#include "Mesh.h"

#include <GL/glew.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cmath>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stb_image.h>

using namespace std;
using namespace tinyobj;

Mesh::Mesh(const std::string &meshname) {
    loadMesh(meshname);
}

// modified from https://github.com/syoyo/tinyobjloader/blob/master/examples/viewer/viewer.cc
void Mesh::loadMesh(const std::string &meshname) {
//    attrib_t attrib;
//    vector<shape_t> shapes;
//    vector<material_t> materials;

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

        // Default material
        materials.push_back(material_t());

		float maxAnisotropy = 1.0f;
		if (GLEW_EXT_texture_filter_anisotropic) {
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
		}

        // Load materials and store name->id map
        for (material_t &mp : materials) {
            if (!mp.diffuse_texname.empty() && textures.find(mp.diffuse_texname) == textures.end()) {
                int width, height, components;

                string texture_name = mp.diffuse_texname;
#ifndef _WIN32
                replace(begin(texture_name), end(texture_name), '\\', '/');
#endif
                texture_name = basedir + texture_name;
                unsigned char *image = stbi_load(texture_name.c_str(), &width, &height, &components, STBI_default);
                if (image == nullptr) {
                    cerr << "Unable to load texture: " << texture_name << endl;
                    return;
                }

                GLuint texture_id;
                glCreateTextures(GL_TEXTURE_2D, 1, &texture_id);
                glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTextureParameterf(texture_id, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
				GLint levels = (GLint)std::log2(std::max(width, height)) + 1;
                if (components == 3) {
					glTextureStorage2D(texture_id, levels, GL_RGB8, width, height);
					glTextureSubImage2D(texture_id, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);
                }
                else if (components == 4) {
					glTextureStorage2D(texture_id, levels, GL_RGBA8, width, height);
					glTextureSubImage2D(texture_id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image); 
                }
				glGenerateTextureMipmap(texture_id);
                stbi_image_free(image);
                textures.insert(make_pair(mp.diffuse_texname, texture_id));
            }
        }

        drawables.resize(materials.size());
        for (const auto &shape : shapes) {
            size_t material_id = shape.mesh.material_ids[0];
            if (textures.count(materials[material_id].diffuse_texname) == 0) {
                material_id = 0;
            }

            auto &d = drawables[material_id];
            d.material_id = material_id;
            d.vertexCount += shape.mesh.indices.size();
        }

        for (auto &d : drawables) {
            if (d.vertexCount > 0) {
                glGenVertexArrays(1, &d.vao);
                glGenBuffers(1, &d.buf);

                glBindVertexArray(d.vao);
                glBindBuffer(GL_ARRAY_BUFFER, d.buf);

                glBufferData(GL_ARRAY_BUFFER, d.vertexCount * sizeof(float) * 8, nullptr, GL_STATIC_DRAW);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid *)(3 * sizeof(float)));
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid *)(6 * sizeof(float)));
                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glEnableVertexAttribArray(2);

                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }

        vector<size_t> lastwrite;
        lastwrite.resize(drawables.size());
        lastwrite.assign(drawables.size(), 0);
        for (const auto &shape : shapes) {
            size_t material_id = shape.mesh.material_ids[0];
            if (textures.count(materials[material_id].diffuse_texname) == 0) {
                material_id = 0;
            }

            auto &d = drawables[material_id];

            if (d.vertexCount > 0) {
                vector<float> vertices;
                for (const auto &index : shape.mesh.indices) {
                    float v0, v1, v2, n0, n1, n2, t0, t1;
                    v0 = attrib.vertices[3 * index.vertex_index];
                    v1 = attrib.vertices[3 * index.vertex_index + 1];
                    v2 = attrib.vertices[3 * index.vertex_index + 2];
                    if (index.normal_index >= 0) {
                        n0 = attrib.normals[3 * index.normal_index];
                        n1 = attrib.normals[3 * index.normal_index + 1];
                        n2 = attrib.normals[3 * index.normal_index + 2];
                    }
                    if (index.texcoord_index >= 0) {
                        t0 = attrib.texcoords[2 * index.texcoord_index];
                        t1 = attrib.texcoords[2 * index.texcoord_index + 1];
                    }

                    vertices.insert(end(vertices), {v0, v1, v2, n0, n1, n2, t0, t1});
                }

                glBindVertexArray(d.vao);
                glBindBuffer(GL_ARRAY_BUFFER, d.buf);
                glBufferSubData(GL_ARRAY_BUFFER, lastwrite[material_id], vertices.size() * sizeof(float), vertices.data());
                lastwrite[material_id] += vertices.size() * sizeof(float);
                assert(lastwrite[material_id] <= d.vertexCount * sizeof(float) * 8);

                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
        }

        auto last = remove_if(begin(drawables), end(drawables), [](const Drawable &d) { return d.vertexCount == 0; });
        drawables.erase(last, end(drawables));
    }
}

void Mesh::draw(GLuint program) const {
    glActiveTexture(GL_TEXTURE0);

    for (const auto &d : drawables) {
        string texture_name = materials[d.material_id].diffuse_texname;
        GLuint texture_id = textures.at(texture_name);

        glBindVertexArray(d.vao);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        glUniform1f(glGetUniformLocation(program, "shine"), materials[d.material_id].shininess);
        glDrawArrays(GL_TRIANGLES, 0, d.vertexCount);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}
