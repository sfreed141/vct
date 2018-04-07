#include "Scene.h"

#include <Graphics/opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <memory>

#include "Graphics/Mesh.h"

Scene::Scene() {}

void Scene::addMesh(std::shared_ptr<Actor> actor) {
	actors.push_back(actor);
}

void Scene::draw(GLShaderProgram &program) {
	for (auto &actor : actors) {
		program.setUniformMatrix4fv("model", actor->getTransform());
		actor->draw(program);
	}
}

void Scene::setMainlight(const glm::vec3 &position, const glm::vec3 &direction, const glm::vec3 &intensity) {
	mainlight.position = position;
	mainlight.direction = direction;
	mainlight.intensity = intensity;
}

void Scene::update(float dt) {
	for (auto &actor : actors) {
		actor->update(dt);
	}
}