#ifndef SCENE_H
#define SCENE_H

#include <Graphics/opengl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>
#include <initializer_list>

#include <Graphics/Mesh.h>

struct Light {
	glm::vec3 position, intensity;
};

class Scene {
public:
	Scene();
	Scene(std::initializer_list<const std::string> meshnames);

	// TODO: option to add local transform, normalize to ndc after loading, error handling (in mesh.cpp)
	void addMesh(const std::string &meshname, const glm::mat4 &model = glm::mat4());
	void draw(GLuint program) const;

	Light &getMainlight() { return mainlight; }
	void setMainlight(const glm::vec3 &position, const glm::vec3 &intensity);

private:
	struct SceneNode {
		std::unique_ptr<Mesh> mesh;
		glm::mat4 model;
	};

	std::vector<SceneNode> nodes;
	Light mainlight;
};

#endif