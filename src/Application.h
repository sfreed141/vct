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
    int steps;
    float coneAngle;
    float bias;
    float coneInitialHeight;
    float lodOffset;
};

struct Settings {
    int toggle = false;
    int drawNormals = false;
    int drawDominantAxis = false;
    int drawWireframe = false;
    int drawVoxels = false;
    int drawRadiance = true;
    int drawAxes = false;
    int axisOverride = -1;
    int drawShadowmap = false;
    int raymarch = false;
    int drawWarpSlope = false;
    int drawOcclusion = true;
    int debugVoxels = false;
    int debugVoxelOpacity = false;
    int debugOcclusion = false;
    int debugIndirect = false;
    int debugReflections = false;
    int debugWarpTexture = false;

    int debugMaterialDiffuse = false, debugMaterialRoughness = false, debugMaterialMetallic = false;

    int msaa = false;
    int alphatocoverage = false;
    int cooktorrance = true;
    enum ConservativeRasterizeMode { OFF, MSAA, NV };
    ConservativeRasterizeMode conservativeRasterization = MSAA;
    int enablePostprocess = true;
    int enableShadows = true;
    int enableNormalMap = true;
    int enableIndirect = true;
    int enableDiffuse = true;
    int enableSpecular = true;
    int enableReflections = true;
    float ambientScale = 1.0f;
    float reflectScale = 1.0f;

    int radianceLighting = false;
    int radianceDilate = false;
    int temporalFilterRadiance = false;
    float temporalDecay = 0.8f;
    float voxelSetOpacity = 0.5f;

    float miplevel = 0;

    float warpTextureHighResolution = 2.0f;
    float warpTextureLowResolution = 0.5;

    int voxelizeLighting = true;
    int voxelizeAtomicMax = false;  // only used if voxel textures are rgba8
    int voxelTrackCamera = false;
    float voxelizeMultiplier = 1.0f;
    int voxelizeDilate = false;
    int warpVoxels = false;
    int warpTexture = false;
    int warpTextureLinear = false;
    int warpTextureAxes[3] = {true, true, true};
    int useWarpmapWeightsTexture = true;
    VCTSettings diffuseConeSettings { 16, glm::radians(60.f), 1.0f, 1.0f, 0.5f };
    VCTSettings specularConeSettings { 32, glm::radians(30.f), 1.7f, 0.5f, 0.1f };
    int specularConeAngleFromRoughness = true;
};

GLuint make3DTexture(GLsizei size, GLsizei levels, GLenum internalFormat, GLint minFilter, GLint magFilter);

class VCT {
public:
    VCT() {
        useRGBA16f = false;//GLAD_GL_NV_shader_atomic_fp16_vector; // MUST ALSO SET USE_RGBA16F to 1 in voxelize.frag, transferVoxels.comp, injectRadiance.comp
        voxelFormat = useRGBA16f ? GL_RGBA16F : GL_RGBA8;
        LOG_DEBUG("useRGBA16f: ", useRGBA16f ? "true" : "false");
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
    GLuint voxelColor = 0, voxelNormal = 0, voxelRadiance = 0, voxelOccupancy = 0;
    bool useRGBA16f;
    GLenum voxelFormat;
    int voxelOccupancyDim = 32;

    glm::vec3 center { 0.0f };
    glm::vec3 min { -20.0f }, max { 20.0f };

private:
    void make() {
        voxelColor = make3DTexture(voxelDim, voxelLevels, voxelFormat, GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST);
        voxelNormal = make3DTexture(voxelDim, 1, voxelFormat, GL_NEAREST, GL_NEAREST);
        voxelRadiance = make3DTexture(voxelDim, voxelLevels, GL_RGBA8, GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST);
        voxelOccupancy = make3DTexture(voxelOccupancyDim, 1, GL_R32UI, GL_NEAREST, GL_NEAREST);
    }

    void cleanup() {
        glDeleteTextures(1, &voxelColor);
        glDeleteTextures(1, &voxelNormal);
        glDeleteTextures(1, &voxelRadiance);
        glDeleteTextures(1, &voxelOccupancy);
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

    static const size_t warpDim = 32;
    GLuint warpmap;

    GLShaderProgram voxelProgram;
    VCT vct;

    GLFramebuffer shadowmapFBO;
    GLShaderProgram shadowmapProgram;

    GLShaderProgram injectRadianceProgram, temporalRadianceFilterProgram;

    GLShaderProgram mipmapProgram, ditherProgram;

    Settings settings;
    GLBufferedTimer voxelizeTimer, shadowmapTimer, radianceTimer, mipmapTimer, renderTimer, totalTimer;

    // if voxelizeDilate is enabled then maxFragmentsPerVoxel is invalid
    struct VoxelizeInfo {
        GLuint totalVoxelFragments = 0, uniqueVoxels = 0, maxFragmentsPerVoxel = 0;
    } voxelizeInfo;
    GLuint voxelizeInfoSSBO = 0;

    void viewRaymarched();
    void debugVoxels(GLuint texture_id, const glm::mat4 &mvp);
};

#endif
