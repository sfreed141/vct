#ifndef SCENE_H
#define SCENE_H

#include <Graphics/opengl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>

#include <Actor.h>

struct Light {
    enum Type { Point, Directional };

    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 color = glm::vec3(1.0f);

    float range = 5.0f;
    float intensity = 1.0f;

    bool enabled = true;
    bool selected = false;
    bool shadowCaster = false;
    unsigned int type = Type::Point;

    bool dirty = false;

    // Assumes buffer bound to GL_SHADER_STORAGE_BUFFER target
    void writeSSBO(unsigned int offset) const;

    static const unsigned int glslSize = 80;
};

class Scene {
public:
    Scene();
    ~Scene() { glDeleteBuffers(1, &lightSSBO); }

    void update(float dt);
    void draw(GLShaderProgram &program);

    void addActor(std::shared_ptr<Actor> actor) { actors.push_back(actor); }
    void addLight(const Light &light);

    void bindLightSSBO(GLuint index) const;

// private:
    std::vector<std::shared_ptr<Actor>> actors;
    std::vector<Light> lights;

    GLuint lightSSBO = 0;
};

#endif