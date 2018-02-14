#ifndef SCENE_H
#define SCENE_H

#include <Graphics/opengl.h>

#include <string>
#include <vector>
#include <memory>
#include <initializer_list>

#include <Graphics/Mesh.h>

class Scene {
public:
	Scene();
	Scene(std::initializer_list<const std::string> meshnames);

	// TODO: option to add local transform, normalize to ndc after loading, error handling (in mesh.cpp)
	void addMesh(const std::string &meshname);
	void draw(GLuint program) const;

private:
	std::vector<std::unique_ptr<Mesh>> meshes;
};

#endif