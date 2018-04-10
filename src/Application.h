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
#include "Graphics/GLTimer.h"

#include "common.h"

struct VCTSettings {
    int steps = 16;
    float coneAngle = 0.784398163f;
    float bias = 1.0f;
    float coneInitialHeight = 1.0f;
    float lodOffset = 0.1f;
};

struct Settings {
    int drawNormals = false;
    int drawDominantAxis = false;
    int drawWireframe = false;
    int drawVoxels = false;
    int drawRadiance = true;
    int drawAxes = false;
	int axisOverride = -1;
    int drawShadowmap = false;
    int raymarch = false;

	int conservativeRasterization = true;
    int enablePostprocess = false;
	int enableShadows = true;
    int enableNormalMap = true;
    int enableIndirect = true;
    int enableDiffuse = true;
    int enableSpecular = true;
    int enableReflections = false;
    float ambientScale = 1.0f;
    float reflectScale = 0.5f;

	int miplevel = 0;

    VCTSettings diffuseConeSettings;
    VCTSettings specularConeSettings { 16, 0.1f, 1.0f, 5.0f, 0.1f };
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
    int voxelDim = 128, voxelLevels = 6;
    GLuint voxelColor = 0, voxelNormal = 0, voxelRadiance = 0;
    bool useRGBA16f;

	GLFramebuffer shadowmapFBO;
	GLShaderProgram shadowmapProgram;

	GLShaderProgram injectRadianceProgram;

    GLShaderProgram mipmapProgram;

    Settings settings;
	GLBufferedTimer voxelizeTimer, shadowmapTimer, radianceTimer, mipmapTimer, renderTimer, totalTimer;

    void viewRaymarched();
};

#endif
