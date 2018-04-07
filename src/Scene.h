#ifndef SCENE_H
#define SCENE_H

#include <Graphics/opengl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>

#include <Actor.h>

struct Light {
	glm::vec3 position, direction, intensity;
};

class Scene {
public:
	Scene();

	void addMesh(std::shared_ptr<Actor> actor);
	void draw(GLShaderProgram &program);

	Light &getMainlight() { return mainlight; }
	void setMainlight(const glm::vec3 &position, const glm::vec3 &direction, const glm::vec3 &intensity);

	void update(float dt);

private:
	std::vector<std::shared_ptr<Actor>> actors;
	
	Light mainlight;
};

#endif