#include "Application.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <vector>
#include <memory>

#include "Graphics/GLHelper.h"
#include "Graphics/GLShaderProgram.h"

#include "Input/Keyboard.h"
#include "Input/Mouse.h"

#include "Overlay.h"
#include "Camera.h"
#include "Scene.h"

#include "common.h"

GLuint make3DTexture(int size);

void Application::init() {
	// Setup for OpenGL
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
	glClearColor(0.5294f, 0.8078f, 0.9216f, 1.0f);
	
	// Setup framebuffers
	shadowmapFBO.bind();
	glm::vec4 borderColor{ 1.0f };
	shadowmapFBO.attachTexture(
		GL_DEPTH_ATTACHMENT, GL_DEPTH_COMPONENT,
		width, height, 
		GL_DEPTH_COMPONENT, GL_FLOAT,
		GL_LINEAR, GL_LINEAR,
		GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER,
		&borderColor
	);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	if (shadowmapFBO.getStatus() != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "shadowmapFBO not created successfully" << std::endl;
	}
	shadowmapFBO.unbind();

	// Create shaders
	program.linkProgram(SHADER_DIR "simple.vert", SHADER_DIR "phong.frag");
	voxelProgram.linkProgram(SHADER_DIR "voxelize.vert", SHADER_DIR "voxelize.frag", SHADER_DIR "voxelize.geom");
	shadowmapProgram.linkProgram(SHADER_DIR "simple.vert", SHADER_DIR "empty.frag");

	// Initialize voxel textures
	voxelColor = make3DTexture(voxelDim);
	voxelNormal = make3DTexture(voxelDim);

	// Create scene
	scene = std::make_unique<Scene>();
	scene->addMesh(RESOURCE_DIR "sponza/sponza_small.obj");

	// Camera setup
	camera.position = glm::vec3(5, 1, 0);
	camera.yaw = 180.0f;
}

void Application::update(float dt) {
	Mouse::update();

	if (GLFW_PRESS == Keyboard::getKey(GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(window, 1);
	}

	if (Keyboard::getKeyTap(GLFW_KEY_LEFT_CONTROL)) {
		int mode = glfwGetInputMode(window, GLFW_CURSOR);
		glfwSetInputMode(window, GLFW_CURSOR, (mode == GLFW_CURSOR_DISABLED) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}

	if (Keyboard::getKeyTap(GLFW_KEY_GRAVE_ACCENT)) {
		ui.enabled = !ui.enabled;
	}

	if (Keyboard::getKeyTap(GLFW_KEY_V)) {
		settings.drawVoxels = !settings.drawVoxels;
	}

	if (GLFW_CURSOR_DISABLED == glfwGetInputMode(window, GLFW_CURSOR)) {
		camera.update(dt);
	}
}

void Application::render(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	timers.beginQuery(TimerQueries::VOXELIZE_TIME);
	// Voxelize scene
	{
		GL_DEBUG_PUSH("Voxelize Scene")
		glViewport(0, 0, voxelDim, voxelDim);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		if (GLEW_NV_conservative_raster && settings.conservativeRasterization) {
			glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
		}

		glClearTexImage(voxelColor, 0, GL_RGBA, GL_FLOAT, nullptr);

		glm::mat4 projection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.0f, 40.0f);
		glm::mat4 mvp_x = projection * glm::lookAt(glm::vec3(20, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 mvp_y = projection * glm::lookAt(glm::vec3(0, 20, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, -1));
		glm::mat4 mvp_z = projection * glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

		voxelProgram.bind();
		glUniformMatrix4fv(voxelProgram.uniformLocation("mvp_x"), 1, GL_FALSE, glm::value_ptr(mvp_x));
		glUniformMatrix4fv(voxelProgram.uniformLocation("mvp_y"), 1, GL_FALSE, glm::value_ptr(mvp_y));
		glUniformMatrix4fv(voxelProgram.uniformLocation("mvp_z"), 1, GL_FALSE, glm::value_ptr(mvp_z));

		glBindImageTexture(0, voxelColor, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
		glBindImageTexture(1, voxelNormal, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

		glUniform1i(voxelProgram.uniformLocation("axis_override"), settings.axisOverride);

		scene->draw(voxelProgram.getHandle());
		voxelProgram.unbind();

		// Restore OpenGL state
		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		if (GLEW_NV_conservative_raster && settings.conservativeRasterization) {
			glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
		}
		GL_DEBUG_POP()
	}
	timers.endQuery();

	// Generate shadowmap
	glm::vec3 lightPos{ 0.0f, 30.0f, -5.0f };
	glm::vec3 lightInt{ 1.0f, 1.0f, 1.0f };
	const float lz_near = 0.1f, lz_far = 50.0f, l_boundary = 25.0f;
	glm::mat4 lp = glm::ortho(-l_boundary, l_boundary, -l_boundary, l_boundary, lz_near, lz_far);
	glm::mat4 lv = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 ls = lp * lv;
	{
		GL_DEBUG_PUSH("Shadowmap")
		shadowmapFBO.bind();
		glViewport(0, 0, width, height);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		glm::mat4 model;

		shadowmapProgram.bind();
		glUniformMatrix4fv(shadowmapProgram.uniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(lp));
		glUniformMatrix4fv(shadowmapProgram.uniformLocation("view"), 1, GL_FALSE, glm::value_ptr(lv));
		glUniformMatrix4fv(shadowmapProgram.uniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));

		scene->draw(shadowmapProgram.getHandle());

		shadowmapProgram.unbind();
		shadowmapFBO.unbind();
		glCullFace(GL_BACK);
		GL_DEBUG_POP()
	}

	timers.beginQuery(TimerQueries::RENDER_TIME);
	// Render scene
	{
		GL_DEBUG_PUSH("Render Scene")
		glm::mat4 projection = glm::perspective(camera.fov, (float)width / height, near, far);
		glm::mat4 view = camera.lookAt();
		glm::mat4 model;

		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glPolygonMode(GL_FRONT_AND_BACK, settings.drawWireframe ? GL_LINE : GL_FILL);

		program.bind();
		glUniformMatrix4fv(program.uniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(program.uniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(program.uniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(program.uniformLocation("eye"), 1, glm::value_ptr(camera.position));
		glUniform3fv(program.uniformLocation("lightPos"), 1, glm::value_ptr(lightPos));
		glUniform3fv(program.uniformLocation("lightInt"), 1, glm::value_ptr(lightInt));
		glUniformMatrix4fv(program.uniformLocation("ls"), 1, GL_FALSE, glm::value_ptr(ls));
		glUniform1i(program.uniformLocation("texture0"), 0);

		GLuint shadowmap = shadowmapFBO.getTexture(0);
		glBindTextureUnit(1, shadowmap);
		glUniform1i(program.uniformLocation("shadowmap"), 1);

		glUniform1i(program.uniformLocation("voxelize"), settings.drawVoxels);
		glUniform1i(program.uniformLocation("normals"), settings.drawNormals);
		glUniform1i(program.uniformLocation("dominant_axis"), settings.drawDominantAxis);		

		glUniform1i(program.uniformLocation("voxelDim"), voxelDim);

		glBindImageTexture(1, voxelColor, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

		scene->draw(program.getHandle());
		program.unbind();
		if (settings.drawWireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		GL_DEBUG_POP()
	}
	timers.endQuery();

	timers.getQueriesAndSwap();

	// Render overlay
	{
		GL_DEBUG_PUSH("Render Overlay")
		ui.render(dt);
		GL_DEBUG_POP()
	}
}

// Create an empty (zeroed) 3D texture with dimensions of size (in bytes)
GLuint make3DTexture(int size) {
	GLuint handle;

	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_3D, handle);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Allocate and zero out texture memory
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, size, size, size, 0, GL_RGBA, GL_FLOAT, nullptr);
	glClearTexImage(handle, 0, GL_RGBA, GL_FLOAT, nullptr);

	glBindTexture(GL_TEXTURE_3D, 0);

	return handle;
}

// Render scene and voxelize hacky way
//	program.linkProgram(SHADER_DIR "simple.vert", SHADER_DIR "phong.frag");
//	voxelProgram.linkProgram(SHADER_DIR "simple.vert", SHADER_DIR "phongVoxel.frag");
void Application::renderSimpleVoxelization(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Voxelize scene
	{
		GL_DEBUG_PUSH("Voxelize Scene")
		glm::mat4 projection = glm::perspective(camera.fov, (float)width / height, near, far);
		glm::mat4 view = camera.lookAt();
		glm::mat4 model;

		glm::vec3 lightPos{ 100.0f, 100.0f, 0.0f };
		glm::vec3 lightInt{ 1.0f, 1.0f, 1.0f };

		glViewport(0, 0, width, height);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDepthMask(GL_FALSE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		voxelProgram.bind();
		glUniformMatrix4fv(voxelProgram.uniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(voxelProgram.uniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(voxelProgram.uniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(voxelProgram.uniformLocation("eye"), 1, glm::value_ptr(camera.position));
		glUniform3fv(voxelProgram.uniformLocation("lightPos"), 1, glm::value_ptr(lightPos));
		glUniform3fv(voxelProgram.uniformLocation("lightInt"), 1, glm::value_ptr(lightInt));
		glUniform1i(voxelProgram.uniformLocation("texture0"), 0);

		glBindImageTexture(1, voxelColor, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

		scene->draw(voxelProgram.getHandle());
		voxelProgram.unbind();

		// Restore OpenGL state
		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		GL_DEBUG_POP()
	}

	// Render scene
	{
		GL_DEBUG_PUSH("Render Scene")
		glm::mat4 projection = glm::perspective(camera.fov, (float)width / height, near, far);
		glm::mat4 view = camera.lookAt();
		glm::mat4 model;

		glm::vec3 lightPos{ 100.0f, 100.0f, 0.0f };
		glm::vec3 lightInt{ 1.0f, 1.0f, 1.0f };

		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		program.bind();
		glUniformMatrix4fv(program.uniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(program.uniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(program.uniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(program.uniformLocation("eye"), 1, glm::value_ptr(camera.position));
		glUniform3fv(program.uniformLocation("lightPos"), 1, glm::value_ptr(lightPos));
		glUniform3fv(program.uniformLocation("lightInt"), 1, glm::value_ptr(lightInt));
		glUniform1i(program.uniformLocation("texture0"), 0);

		glUniform1i(program.uniformLocation("voxelize"), settings.drawVoxels);

		glBindImageTexture(1, voxelColor, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

		scene->draw(program.getHandle());
		program.unbind();
		GL_DEBUG_POP()
	}

	// Render overlay
	{
		GL_DEBUG_PUSH("Render Overlay")
		ui.render(dt);
		GL_DEBUG_POP()
	}
}