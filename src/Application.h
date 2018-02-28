#ifndef APPLICATION_H
#define APPLICATION_H

#include <Graphics/opengl.h>
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
#include "Graphics/GLFramebuffer.h"

#include "Overlay.h"
#include "Camera.h"
#include "Scene.h"
#include "TimerQueries.h"

#include "common.h"

struct Settings {
    int drawNormals = false;
    int drawDominantAxis = false;
    int drawWireframe = false;
    int drawVoxels = false;
    int drawRadiance = false;
    int drawAxes = false;
	int axisOverride = -1;
    int drawShadowmap = false;

	int conservativeRasterization = true;
	int enableShadows = true;
    int enableNormalMap = true;
    int enableIndirect = true;
    int enableDiffuse = false;
    int enableSpecular = false;
    float ambientScale = 0.2f;

	int miplevel = 0;

    int vctSteps = 1;
    float vctConeAngle = 0.784398163f;
    float vctBias = 1.0f;
    float vctConeInitialHeight = 1.0f;
    float vctLodOffset = 0.0f;
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

    std::unique_ptr<Scene> scene = nullptr;
    GLShaderProgram program;
    
    GLShaderProgram voxelProgram;
    int voxelDim = 64;
    GLuint voxelColor = 0, voxelNormal = 0, voxelRadiance = 0;

	GLFramebuffer shadowmapFBO;
	GLShaderProgram shadowmapProgram;

	GLShaderProgram injectRadianceProgram;

    Settings settings;
	TimerQueries timers;

	void renderSimpleVoxelization(float dt);
};

#endif
