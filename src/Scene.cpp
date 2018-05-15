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
}

void Scene::draw(GLShaderProgram &program) {
    for (auto &actor : actors) {
        program.setUniformMatrix4fv("model", actor->getTransform());
        actor->draw(program);
    }
}
