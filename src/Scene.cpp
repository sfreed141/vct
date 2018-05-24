#include "Scene.h"

#include <Graphics/opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <memory>

#include "Graphics/Mesh.h"

Scene::Scene() {}

void Scene::update(float dt) {
    for (auto &actor : actors) {
        actor->update(dt);
    }

    for (std::size_t i = 0; i < lights.size(); i++) {
        auto &light = lights[i];

        if (light.dirty) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
            light.writeSSBO(i * Light::glslSize);
        }
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Scene::draw(GLShaderProgram &program, GLenum mode) {
    for (auto &actor : actors) {
        program.setUniformMatrix4fv("model", actor->getTransform());
        actor->draw(program, mode);
    }
}

void Scene::addLight(const Light &light) {
    if (lightSSBO == 0) {
        glCreateBuffers(1, &lightSSBO);
    }

    lights.push_back(light);

    // Extend buffer (orphaning old one)
    glNamedBufferData(lightSSBO, lights.size() * Light::glslSize, nullptr, GL_STATIC_DRAW);

    // Copy data to buffer
    unsigned int offset = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
    for (const auto &light : lights) {
        light.writeSSBO(offset);
        offset += Light::glslSize;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Scene::bindLightSSBO(GLuint index) const {
    assert(lights.size() > 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, lightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Light::writeSSBO(unsigned int offset) const {
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 0, 12, &position);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 16, 12, &direction);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 32, 12, &color);

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 44, 4, &range);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 48, 4, &intensity);

    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 52, 4, &enabled);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 56, 4, &selected);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 60, 4, &shadowCaster);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset + 64, 4, &type);
}
