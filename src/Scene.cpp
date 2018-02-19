#include "Scene.h"

#include <Graphics/opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <memory>
#include <initializer_list>

#include "Graphics/Mesh.h"

Scene::Scene() {}
Scene::Scene(std::initializer_list<const std::string> meshnames) {
	for (const auto &meshname : meshnames) {
		addMesh(meshname);
	}
}

void Scene::addMesh(const std::string &meshname, const glm::mat4 &model) {
	nodes.push_back({std::make_unique<Mesh>(meshname), model});
}

void Scene::draw(GLuint program) const {
	for (const auto &node : nodes) {
		glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(node.model));
		node.mesh->draw(program);
	}
}