#include "Scene.h"

#include <glad/glad.h>

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

void Scene::addMesh(const std::string &meshname) {
	meshes.push_back(std::make_unique<Mesh>(meshname));
}

void Scene::draw(GLuint program) const {
	for (const auto &mesh : meshes) {
		mesh->draw(program);
	}
}