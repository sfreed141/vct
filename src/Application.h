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

	enum ConservativeRasterizeMode { OFF, MSAA, NV };
    ConservativeRasterizeMode conservativeRasterization = MSAA;
    int enablePostprocess = false;
	int enableShadows = true;
    int enableNormalMap = true;
    int enableIndirect = true;
    int enableDiffuse = true;
    int enableSpecular = true;
    int enableReflections = false;
    float ambientScale = 1.0f;
    float reflectScale = 0.5f;

    int radianceDilate = false;
    int temporalFilterRadiance = false;
    float temporalDecay = 0.8f;

	int miplevel = 0;

    int voxelTrackCamera = false;
    float voxelizeMultiplier = 1.0f;
    int voxelizeDilate = false;
    int voxelWarp = false;
    VCTSettings diffuseConeSettings;
    VCTSettings specularConeSettings { 16, 0.1f, 1.0f, 5.0f, 0.1f };
};

GLuint make3DTexture(GLsizei size, GLsizei levels, GLenum internalFormat, GLint minFilter, GLint magFilter);

class VCT {
public:
    VCT() {
        useRGBA16f = GLAD_GL_NV_shader_atomic_fp16_vector;
	    voxelFormat = useRGBA16f ? GL_RGBA16F : GL_RGBA8;
        make();
    }

    ~VCT() { cleanup(); }

    void remake(int dim, int levels) {
        assert(dim > 0);
        voxelDim = dim;
        voxelLevels = glm::clamp<int>(levels, 0, std::log2(dim) + 1);

        if (voxelLevels != levels) {
            LOG_WARN("Attempted remaking VCT with invalid number of levels, clamped ", levels, " to ", voxelLevels);
        }

        cleanup();
        make();
    }

    glm::vec3 voxelWorldSize() const { return (max - min) / (float)voxelDim; }

    int voxelDim = 256, voxelLevels = 6;
    GLuint voxelColor = 0, voxelNormal = 0, voxelRadiance = 0;
    bool useRGBA16f;
    GLenum voxelFormat;

    glm::vec3 center { 0.0f };
    glm::vec3 min { -20.0f }, max { 20.0f };

private:
    void make() {
        voxelColor = make3DTexture(voxelDim, 1, voxelFormat, GL_LINEAR, GL_NEAREST);
        voxelNormal = make3DTexture(voxelDim, 1, voxelFormat, GL_NEAREST, GL_NEAREST);
        voxelRadiance = make3DTexture(voxelDim, voxelLevels, GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST);
    }

    void cleanup() {
        glDeleteTextures(1, &voxelColor);
        glDeleteTextures(1, &voxelNormal);
        glDeleteTextures(1, &voxelRadiance);
    }
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
    VCT vct;

	GLFramebuffer shadowmapFBO;
	GLShaderProgram shadowmapProgram;

	GLShaderProgram injectRadianceProgram, temporalRadianceFilterProgram;

    GLShaderProgram mipmapProgram;

    Settings settings;
	GLBufferedTimer voxelizeTimer, shadowmapTimer, radianceTimer, mipmapTimer, renderTimer, totalTimer;

    void viewRaymarched();
};

#endif
