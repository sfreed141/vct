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

	// Assumes buffer bound to GL_SHADER_STORAGE_BUFFER target
	void writeSSBO(unsigned int offset) const {
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

	static const unsigned int glslSize = 80;
};

class Scene {
public:
	Scene();
	~Scene() { glDeleteBuffers(1, &lightSSBO); }

	void update(float dt);
	void draw(GLShaderProgram &program);

	void addActor(std::shared_ptr<Actor> actor) { actors.push_back(actor); }
	void addLight(const Light &light) {
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

	void bindLightSSBO(GLuint index) const {
		assert(lights.size() > 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, lightSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

// private:
	std::vector<std::shared_ptr<Actor>> actors;
	std::vector<Light> lights;

	GLuint lightSSBO = 0;
};

#endif