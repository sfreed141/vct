#ifndef APPLICATION_H
#define APPLICATION_H

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
#include "Graphics/Mesh.h"
#include "Graphics/GLShaderProgram.h"

#include "Overlay.h"
#include "Camera.h"

#include "common.h"

struct Settings {
    int drawNormals = false;
    int drawDominantAxis = false;
    int drawWireframe = false;
    int drawVoxels = false;
    int drawAxes = false;
	int axisOverride = -1;

	int conservativeRasterization = true;
};

class Application {
public:
    friend class Overlay;

    Application(GLFWwindow *window) : window{window}, ui{window, *this} {}

    void init();
    void update(float dt);
    void render(float dt);

private:
    GLFWwindow *window = nullptr;
    Camera camera;
    Overlay ui;
    int width, height;
    float near = 0.1f, far = 100.0f;

    std::unique_ptr<Mesh> mesh = nullptr;
    GLShaderProgram program;
    
    GLShaderProgram voxelProgram;
    int voxelDim = 64;
    GLuint voxelColor = 0, voxelNormal = 0;

    Settings settings;

	void renderSimpleVoxelization(float dt);
};

#endif
