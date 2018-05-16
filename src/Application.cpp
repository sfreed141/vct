#include "Application.h"

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
#include "Graphics/GLQuad.h"
#include "Graphics/GLTimer.h"

#include "Input/Keyboard.h"
#include "Input/Mouse.h"

#include "Overlay.h"
#include "Camera.h"
#include "Scene.h"

#include "common.h"

#define SHADOWMAP_WIDTH 4096
#define SHADOWMAP_HEIGHT 4096

void view2DTexture(GLuint texture);

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
        SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT,
        GL_DEPTH_COMPONENT, GL_FLOAT,
        GL_LINEAR, GL_LINEAR,
        GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER,
        &borderColor
    );
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (shadowmapFBO.getStatus() != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("shadowmapFBO not created successfully");
    }
    shadowmapFBO.unbind();

    // Create shaders
    program.attachAndLink({SHADER_DIR "phong.vert", SHADER_DIR "phong.frag"});
    program.setObjectLabel("Phong");
    voxelProgram.attachAndLink({SHADER_DIR "voxelize.vert", SHADER_DIR "voxelize.frag", SHADER_DIR "voxelize.geom"});
    voxelProgram.setObjectLabel("Voxelize");
    shadowmapProgram.attachAndLink({SHADER_DIR "simple.vert", SHADER_DIR "empty.frag"});
    shadowmapProgram.setObjectLabel("Shadowmap");
    injectRadianceProgram.attachAndLink({SHADER_DIR "injectRadiance.comp"});
    injectRadianceProgram.setObjectLabel("Inject Radiance");
    temporalRadianceFilterProgram.attachAndLink({SHADER_DIR "temporalRadianceFilter.comp"});
    temporalRadianceFilterProgram.setObjectLabel("Temporal Radiance Filter");
    mipmapProgram.attachAndLink({SHADER_DIR "filterRadiance.comp"});
    mipmapProgram.setObjectLabel("Filter Radiance");
    ditherProgram.attachAndLink({SHADER_DIR "simple.vert", SHADER_DIR "dither.frag"});
    ditherProgram.setObjectLabel("Dither");

    // Create scene
    scene = std::make_unique<Scene>();

    StaticMeshActor sponza {RESOURCE_DIR "sponza/sponza_dds.obj"};
    // StaticMeshActor sponza {RESOURCE_DIR "sponza/sponza_pbr.obj"}, nanosuit {RESOURCE_DIR "nanosuit/nanosuit.obj"};
    sponza.transform.setScale(glm::vec3(0.01f));
    scene->addActor(std::make_shared<StaticMeshActor>(sponza));

    // nanosuit.transform.setScale(glm::vec3(0.25f));
    // nanosuit.controller = new LambdaActorController([](Actor &actor, float dt, float time) {
    // 	const float speedMultiplier = 1.0f;
    // 	// actor.transform.setPosition(actor.transform.getPosition() + glm::vec3(0.0f, glm::sin(time), 0.0f));
    // 	actor.transform.setScale(glm::vec3(0.25f + 0.05f * glm::sin(speedMultiplier * time)));
    // 	actor.transform.setPosition(glm::vec3(glm::cos(speedMultiplier * time), 0.0f, glm::sin(speedMultiplier * time)));
    // 	actor.transform.rotate(speedMultiplier * dt, glm::vec3(0.0f, 1.0f, 0.0f));
    // });
    // scene->addActor(std::make_shared<StaticMeshActor>(nanosuit));

    // StaticMeshActor nanosuit2 {RESOURCE_DIR "nanosuit/nanosuit.obj"};
    // nanosuit2.transform.setScale(glm::vec3(0.2f));
    // nanosuit2.controller = new LambdaActorController([](Actor &actor, float dt, float time) {
    // 	actor.transform.setPosition(glm::vec3(2 * glm::sin(0.4 * time) - 2, 4.2f, 3.0f));
    // });
    // scene->addActor(std::make_shared<StaticMeshActor>(nanosuit2));

    // StaticMeshActor cube {RESOURCE_DIR "cube.obj"};
    // cube.transform.setScale(glm::vec3(0.5f));
    // cube.controller = new LambdaActorController([](Actor &actor, float dt, float time) {
    // 	actor.transform.setPosition(glm::vec3(2 * glm::sin(0.4 * time) + 2, 5.0f, 3.0f));
    // });
    // scene->addActor(std::make_shared<StaticMeshActor>(cube));

    Light mainlight;
    mainlight.type = Light::Type::Directional;
    mainlight.shadowCaster = true;
    mainlight.position = glm::vec3(12.0f, 40.0f, -7.0f);
    mainlight.direction = glm::vec3(-0.38f, -0.88f, 0.2f);
    scene->addLight(mainlight);

    Light test;
    test.type = Light::Type::Point;
    test.position = glm::vec3(0.0f, 10.0f, 0.0f);
    test.color = glm::vec3(1.0f, 0.0f, 1.0f);
    scene->addLight(test);

    // Camera setup
    camera.position = glm::vec3(5, 1, 0);
    camera.yaw = 180.0f;
    camera.update(0.0f);

    GLQuad::init();
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

    if (Keyboard::getKeyTap(GLFW_KEY_T)) {
        settings.toggle = !settings.toggle;
    }

    if (GLFW_CURSOR_DISABLED == glfwGetInputMode(window, GLFW_CURSOR)) {
        camera.update(dt);
    }

    if (settings.voxelTrackCamera) {
        // To prevent temporal artifacts, the voxel textures are 'snapped' to a discrete grid

        // TODO
        glm::vec3 gridcell = glm::pow(2.f, (float)vct.voxelLevels) * (vct.max - vct.min) / (float)vct.voxelDim;
        vct.center = glm::floor(camera.position / gridcell) * gridcell;
    }

    scene->update(dt);
}

void Application::render(float dt) {
    totalTimer.start();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Light mainlight = scene->lights[0];

    const float lz_near = 0.0f, lz_far = 100.0f, l_boundary = 25.0f;
    glm::mat4 lp = glm::ortho(-l_boundary, l_boundary, -l_boundary, l_boundary, lz_near, lz_far);
    glm::mat4 lv = glm::lookAt(mainlight.position, mainlight.position + mainlight.direction, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 ls = lp * lv;
    // Generate shadowmap
    shadowmapTimer.start();
    {
        GL_DEBUG_PUSH("Shadowmap")
        shadowmapFBO.bind();
        glViewport(0, 0, SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT);
        glClear(GL_DEPTH_BUFFER_BIT);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        shadowmapProgram.bind();
        shadowmapProgram.setUniformMatrix4fv("projection", lp);
        shadowmapProgram.setUniformMatrix4fv("view", lv);

        scene->draw(shadowmapProgram);

        shadowmapProgram.unbind();
        shadowmapFBO.unbind();

        GL_DEBUG_POP()
    }
    shadowmapTimer.stop();

    voxelizeTimer.start();
    // Voxelize scene
    {
        GL_DEBUG_PUSH("Voxelize Scene")
        glViewport(0, 0, settings.voxelizeMultiplier * vct.voxelDim, settings.voxelizeMultiplier * vct.voxelDim);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        switch (settings.conservativeRasterization) {
            case Settings::ConservativeRasterizeMode::NV:
                glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
                break;
            case Settings::ConservativeRasterizeMode::MSAA:
                glEnable(GL_MULTISAMPLE);
                break;
            case Settings::ConservativeRasterizeMode::OFF:
            default:
                // pass
                break;
        }

        glClearTexImage(vct.voxelColor, 0, GL_RGBA, GL_FLOAT, nullptr);
        glClearTexImage(vct.voxelNormal, 0, GL_RGBA, GL_FLOAT, nullptr);

        glm::mat4 projection = glm::ortho(vct.min.x, vct.max.x, vct.min.y, vct.max.y, 0.0f, vct.max.z - vct.min.z);
        glm::mat4 mvp_x = projection * glm::lookAt(glm::vec3(vct.max.x, 0, 0) + vct.center, vct.center, glm::vec3(0, 1, 0));
        glm::mat4 mvp_y = projection * glm::lookAt(glm::vec3(0, vct.max.y, 0) + vct.center, vct.center, glm::vec3(0, 0, -1));
        glm::mat4 mvp_z = projection * glm::lookAt(glm::vec3(0, 0, vct.max.z) + vct.center, vct.center, glm::vec3(0, 1, 0));

        voxelProgram.bind();
        voxelProgram.setUniformMatrix4fv("mvp_x", mvp_x);
        voxelProgram.setUniformMatrix4fv("mvp_y", mvp_y);
        voxelProgram.setUniformMatrix4fv("mvp_z", mvp_z);

        voxelProgram.setUniform1i("axis_override", settings.axisOverride);

        voxelProgram.setUniform3fv("eye", camera.position);
        voxelProgram.setUniform3fv("lightPos", mainlight.position);
        voxelProgram.setUniform3fv("lightInt", mainlight.color);
        voxelProgram.setUniform1i("voxelizeDilate", settings.voxelizeDilate);
        voxelProgram.setUniform1i("warpVoxels", settings.warpVoxels);
        voxelProgram.setUniform1i("voxelizeAtomicMax", settings.voxelizeAtomicMax);
        voxelProgram.setUniform1i("toggle", settings.toggle);
        voxelProgram.setUniform1i("voxelizeLighting", settings.voxelizeLighting);
        voxelProgram.setUniform3fv("voxelMin", vct.min);
        voxelProgram.setUniform3fv("voxelMax", vct.max);
        voxelProgram.setUniform3fv("voxelCenter", vct.center);

        glBindImageTexture(0, vct.voxelColor, 0, GL_TRUE, 0, GL_READ_WRITE, vct.useRGBA16f ? GL_RGBA16F : GL_R32UI);
        glBindImageTexture(1, vct.voxelNormal, 0, GL_TRUE, 0, GL_READ_WRITE, vct.useRGBA16f ? GL_RGBA16F : GL_R32UI);

        scene->bindLightSSBO(3);
        voxelProgram.setUniformMatrix4fv("ls", ls);

        GLuint shadowmap = shadowmapFBO.getTexture(0);
        glBindTextureUnit(6, shadowmap);

        scene->draw(voxelProgram);

        glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindImageTexture(1, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
        voxelProgram.unbind();

        // Restore OpenGL state
        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        switch (settings.conservativeRasterization) {
            case Settings::ConservativeRasterizeMode::NV:
                glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
                break;
            case Settings::ConservativeRasterizeMode::MSAA:
                glDisable(GL_MULTISAMPLE);
                break;
            case Settings::ConservativeRasterizeMode::OFF:
            default:
                // pass
                break;
        }
        GL_DEBUG_POP()
    }
    voxelizeTimer.stop();

    // Normalize vct.voxelColor and vct.voxelNormal textures (divides by alpha component)
    if (vct.useRGBA16f) {
        static GLShaderProgram p ("Normalize Voxels", {SHADER_DIR "normalizeVoxels.comp"});

        p.bind();
        glBindImageTexture(0, vct.voxelColor, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindImageTexture(1, vct.voxelNormal, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

        glDispatchCompute((vct.voxelDim + 4 - 1) / 4, (vct.voxelDim + 4 - 1) / 4, (vct.voxelDim + 4 - 1) / 4);

        glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindImageTexture(1, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
        p.unbind();
    }
    else {
        glClearTexImage(vct.voxelRadiance, 0, GL_RGBA, GL_FLOAT, nullptr);

        // Copies opacity to radiance texture. Optionally sets opacity to 1 (which should be 'correct' but it looks too dark)
        // TODO breaks temporal filtering (since it sets to constant color + alpha)
        {
            static GLShaderProgram shader {"Set Voxel Opacity", {SHADER_DIR "setVoxelOpacity.comp"}};
            shader.bind();

            shader.setUniform1f("voxelSetOpacity", settings.voxelSetOpacity);

            glBindImageTexture(0, vct.voxelColor, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
            glBindImageTexture(1, vct.voxelRadiance, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
            glDispatchCompute((vct.voxelDim + 4 - 1) / 4, (vct.voxelDim + 4 - 1) / 4, (vct.voxelDim + 4 - 1) / 4);
            glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
            glBindImageTexture(1, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);

            shader.unbind();
        }
    }

    // Inject radiance into voxel grid
    radianceTimer.start();
    {
        GL_DEBUG_PUSH("Radiance Injection")

        if (settings.temporalFilterRadiance) {
            GL_DEBUG_PUSH("Temporal Radiance Filter")
            temporalRadianceFilterProgram.bind();

            glBindImageTexture(0, vct.voxelRadiance, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
            temporalRadianceFilterProgram.setUniform1f("temporalDecay", settings.temporalDecay);

            glDispatchCompute((vct.voxelDim + 8 - 1) / 8, (vct.voxelDim + 8 - 1) / 8, (vct.voxelDim + 8 - 1) / 8);

            temporalRadianceFilterProgram.unbind();
            GL_DEBUG_POP()
        }
        else if (vct.useRGBA16f) {
            glClearTexImage(vct.voxelRadiance, 0, GL_RGBA, GL_FLOAT, nullptr);
        }

        injectRadianceProgram.bind();

        glBindImageTexture(0, vct.voxelColor, 0, GL_TRUE, 0, GL_READ_ONLY, vct.voxelFormat);
        glBindImageTexture(1, vct.voxelNormal, 0, GL_TRUE, 0, GL_READ_ONLY, vct.voxelFormat);
        glBindImageTexture(2, vct.voxelRadiance, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);

        GLuint shadowmap = shadowmapFBO.getTexture(0);
        glBindTextureUnit(1, shadowmap);
        injectRadianceProgram.setUniform1i("shadowmap", 1);

        glm::mat4 lsInverse = glm::inverse(ls);
        injectRadianceProgram.setUniformMatrix4fv("lsInverse", lsInverse);
        injectRadianceProgram.setUniform3fv("lightPos", mainlight.position);
        injectRadianceProgram.setUniform3fv("lightInt", mainlight.color);

        injectRadianceProgram.setUniform3fv("eye", camera.position);
        injectRadianceProgram.setUniform1i("warpVoxels", settings.warpVoxels);
        injectRadianceProgram.setUniform1i("voxelDim", vct.voxelDim);
        injectRadianceProgram.setUniform3fv("voxelMin", vct.min);
        injectRadianceProgram.setUniform3fv("voxelMax", vct.max);
        injectRadianceProgram.setUniform3fv("voxelCenter", vct.center);

        injectRadianceProgram.setUniform1i("radianceLighting", settings.radianceLighting);
        injectRadianceProgram.setUniform1i("radianceDilate", settings.radianceDilate);
        injectRadianceProgram.setUniform1i("temporalFilterRadiance", settings.temporalFilterRadiance);

        // 2D workgroup should be the size of shadowmap, local_size = 16
        glDispatchCompute((SHADOWMAP_WIDTH + 16 - 1) / 16, (SHADOWMAP_HEIGHT + 16 - 1) / 16, 1);

        glBindTextureUnit(1, 0);
        glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
        glBindImageTexture(1, 0, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
        glBindImageTexture(2, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
        injectRadianceProgram.unbind();

        GL_DEBUG_POP()
    }
    radianceTimer.stop();

    mipmapTimer.start();
    {
        // glGenerateTextureMipmap(vct.voxelColor);
        // glGenerateTextureMipmap(vct.voxelNormal);
        // glGenerateTextureMipmap(vct.voxelRadiance);

        mipmapProgram.bind();

        int dim = vct.voxelDim;
        const int local_size = 8;
        for (int level = 0; level < vct.voxelLevels; level++) {
            glBindImageTexture(0, vct.voxelRadiance, level, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
            glBindImageTexture(1, vct.voxelRadiance, level + 1, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

            GLuint num_groups = ((dim >> 1) + local_size - 1) / local_size;
            glDispatchCompute(num_groups, num_groups, num_groups);

            glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
            glBindImageTexture(1, 0, 1, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

            dim >>= 1;
        }
        dim = vct.voxelDim;
        for (int level = 0; level < vct.voxelLevels; level++) {
            glBindImageTexture(0, vct.voxelColor, level, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
            glBindImageTexture(1, vct.voxelColor, level + 1, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

            GLuint num_groups = ((dim >> 1) + local_size - 1) / local_size;
            glDispatchCompute(num_groups, num_groups, num_groups);

            glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
            glBindImageTexture(1, 0, 1, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

            dim >>= 1;
        }

        mipmapProgram.unbind();
    }
    mipmapTimer.stop();

    if (settings.debugVoxels) {
        glm::mat4 projection = glm::perspective(camera.fov, (float)width / height, near, far);
        glm::mat4 view = camera.lookAt();
        glm::mat4 mvp = projection * view;
        debugVoxels(settings.drawRadiance ? vct.voxelRadiance : vct.voxelColor, mvp);
    }
    else if (settings.raymarch) {
        viewRaymarched();
    }
    else if (settings.drawShadowmap) {
        view2DTexture(shadowmapFBO.getTexture(0));
    }
    else {
        // Depth prepass
        {
            GL_DEBUG_PUSH("Depth Prepass")
            glm::mat4 projection = glm::perspective(camera.fov, (float)width / height, near, far);
            glm::mat4 view = camera.lookAt();

            glViewport(0, 0, width, height);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            if (settings.msaa) {
                glEnable(GL_MULTISAMPLE);
            }
            if (settings.alphatocoverage) {
                glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            }
            else {
                glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            }

            ditherProgram.bind();

            ditherProgram.setUniformMatrix4fv("projection", projection);
            ditherProgram.setUniformMatrix4fv("view", view);

            scene->draw(ditherProgram);

            ditherProgram.unbind();

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            GL_DEBUG_POP()
        }

        renderTimer.start();
        // Render scene
        {
            GL_DEBUG_PUSH("Render Scene")
            glm::mat4 projection = glm::perspective(camera.fov, (float)width / height, near, far);
            glm::mat4 view = camera.lookAt();

            glViewport(0, 0, width, height);
            // glEnable(GL_FRAMEBUFFER_SRGB);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_EQUAL);
            glDepthMask(GL_FALSE);
            glEnable(GL_CULL_FACE);
            glPolygonMode(GL_FRONT_AND_BACK, settings.drawWireframe ? GL_LINE : GL_FILL);

            program.bind();
            program.setUniformMatrix4fv("projection", projection);
            program.setUniformMatrix4fv("view", view);
            program.setUniform3fv("eye", camera.position);
            program.setUniformMatrix4fv("ls", ls);

            GLuint shadowmap = shadowmapFBO.getTexture(0);
            glBindTextureUnit(6, shadowmap);

            program.setUniform1i("voxelize", settings.drawVoxels);
            program.setUniform1i("normals", settings.drawNormals);
            program.setUniform1i("dominant_axis", settings.drawDominantAxis);
            program.setUniform1i("radiance", settings.drawRadiance);
            program.setUniform1i("drawWarpSlope", settings.drawWarpSlope);
            program.setUniform1i("drawOcclusion", settings.drawOcclusion);
            program.setUniform1i("debugOcclusion", settings.debugOcclusion);
            program.setUniform1i("debugIndirect", settings.debugIndirect);

            program.setUniform1i("cooktorrance", settings.cooktorrance);
            program.setUniform1i("enablePostprocess", settings.enablePostprocess);
            program.setUniform1i("enableShadows", settings.enableShadows);
            program.setUniform1i("enableNormalMap", settings.enableNormalMap);
            program.setUniform1i("enableIndirect", settings.enableIndirect);
            program.setUniform1i("enableDiffuse", settings.enableDiffuse);
            program.setUniform1i("enableSpecular", settings.enableSpecular);
            program.setUniform1i("enableReflections", settings.enableReflections);
            program.setUniform1f("ambientScale", settings.ambientScale);
            program.setUniform1f("reflectScale", settings.reflectScale);

            program.setUniform1i("miplevel", settings.miplevel);

            program.setUniform1i("warpVoxels", settings.warpVoxels);
            program.setUniform1i("voxelDim", vct.voxelDim);
            program.setUniform3fv("voxelMin", vct.min);
            program.setUniform3fv("voxelMax", vct.max);
            program.setUniform3fv("voxelCenter", vct.center);
            glBindTextureUnit(2, vct.voxelColor);
            glBindTextureUnit(3, vct.voxelNormal);
            glBindTextureUnit(4, vct.voxelRadiance);

            program.setUniform1i("vctSteps", settings.diffuseConeSettings.steps);
            program.setUniform1f("vctBias", settings.diffuseConeSettings.bias);
            program.setUniform1f("vctConeAngle", settings.diffuseConeSettings.coneAngle);
            program.setUniform1f("vctConeInitialHeight", settings.diffuseConeSettings.coneInitialHeight);
            program.setUniform1f("vctLodOffset", settings.diffuseConeSettings.lodOffset);

            program.setUniform1i("vctSpecularSteps", settings.specularConeSettings.steps);
            program.setUniform1f("vctSpecularBias", settings.specularConeSettings.bias);
            program.setUniform1f("vctSpecularConeAngle", settings.specularConeSettings.coneAngle);
            program.setUniform1f("vctSpecularConeInitialHeight", settings.specularConeSettings.coneInitialHeight);
            program.setUniform1f("vctSpecularLodOffset", settings.specularConeSettings.lodOffset);

            scene->bindLightSSBO(3);

            scene->draw(program);

            glBindTextureUnit(1, 0);
            glBindTextureUnit(2, 0);
            glBindTextureUnit(3, 0);
            glBindTextureUnit(4, 0);
            program.unbind();

            glDisable(GL_MULTISAMPLE);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            // glDisable(GL_FRAMEBUFFER_SRGB);
            if (settings.drawWireframe) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            GL_DEBUG_POP()
        }
        renderTimer.stop();
    }

    totalTimer.stop();

    voxelizeTimer.getQueryResult();
    shadowmapTimer.getQueryResult();
    radianceTimer.getQueryResult();
    mipmapTimer.getQueryResult();
    renderTimer.getQueryResult();
    totalTimer.getQueryResult();

    // Render overlay
    {
        GL_DEBUG_PUSH("Render Overlay")
        ui.render(dt);
        GL_DEBUG_POP()
    }
}

// Create a 3D texture
GLuint make3DTexture(GLsizei size, GLsizei levels, GLenum internalFormat, GLint minFilter, GLint magFilter) {
    GLuint handle;

    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_3D, handle);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

    glTexStorage3D(GL_TEXTURE_3D, levels, internalFormat, size, size, size);
    glClearTexImage(handle, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    if (levels > 1) {
        glGenerateMipmap(GL_TEXTURE_3D);
    }

    glBindTexture(GL_TEXTURE_3D, 0);

    return handle;
}

// Dirty function that renders a texture to a full screen quad.
void view2DTexture(GLuint texture) {
    static const GLchar *vert =
        "#version 330\n"
        "layout(location = 0) in vec3 pos;\n"
        "layout(location = 1) in vec2 tc;\n"
        "out vec2 fragTexcoord;\n"
        "void main() {\n"
        "gl_Position = vec4(pos, 1);\n"
        "fragTexcoord = tc;\n"
        "}\n";

    static const GLchar *frag =
        "#version 420\n"
        "in vec2 fragTexcoord;\n"
        "out vec4 color;\n"
        "layout(binding = 0) uniform sampler2D texture0;\n"
        "void main() {\n"
        "color = vec4(texture(texture0, fragTexcoord).rgb, 1);\n"
        "}\n";

    static GLuint program = 0;
    if (program == 0) {
        GLuint shaders[2];
        shaders[0] = GLHelper::createShaderFromString(GL_VERTEX_SHADER, vert);
        shaders[1] = GLHelper::createShaderFromString(GL_FRAGMENT_SHADER, frag);
        program = glCreateProgram();
        glAttachShader(program, shaders[0]);
        glAttachShader(program, shaders[1]);
        glLinkProgram(program);
        if (!GLHelper::checkShaderProgramStatus(program)) {
            LOG_ERROR("Quad debug shader compilation failed");
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);

    glBindTextureUnit(0, texture);
    GLQuad::draw();

    glUseProgram(0);
}

void Application::viewRaymarched() {
    static const GLchar *vert =
        "#version 330\n"
        "layout(location = 0) in vec3 pos;\n"
        "layout(location = 1) in vec2 tc;\n"
        "out vec2 fragTexcoord;\n"
        "void main() {\n"
        "gl_Position = vec4(pos, 1);\n"
        "fragTexcoord = tc;\n"
        "}\n";

    static GLuint program = 0;
    if (program == 0) {
        GLuint shaders[2];
        shaders[0] = GLHelper::createShaderFromString(GL_VERTEX_SHADER, vert);
        shaders[1] = GLHelper::createShaderFromFile(GL_FRAGMENT_SHADER, SHADER_DIR "raymarch.frag");
        program = glCreateProgram();
        glAttachShader(program, shaders[0]);
        glAttachShader(program, shaders[1]);
        glLinkProgram(program);
        if (!GLHelper::checkShaderProgramStatus(program)) {
            LOG_ERROR("Raymarch debug shader compilation failed");
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);

    glBindTextureUnit(0, vct.voxelColor);
    glBindTextureUnit(1, vct.voxelNormal);
    glBindTextureUnit(2, vct.voxelRadiance);

    glm::vec3 cameraRight = glm::normalize(glm::cross(camera.front, camera.up)) * ((float)width / height);

    glUniform3fv(glGetUniformLocation(program, "eye"), 1, glm::value_ptr(camera.position));
    glUniform3fv(glGetUniformLocation(program, "viewForward"), 1, glm::value_ptr(camera.front));
    glUniform3fv(glGetUniformLocation(program, "viewRight"), 1, glm::value_ptr(cameraRight));
    glUniform3fv(glGetUniformLocation(program, "viewUp"), 1, glm::value_ptr(glm::normalize(glm::cross(cameraRight, camera.front))));
    glUniform1i(glGetUniformLocation(program, "width"), width);
    glUniform1i(glGetUniformLocation(program, "height"), height);
    glUniform1f(glGetUniformLocation(program, "near"), near);
    glUniform1f(glGetUniformLocation(program, "far"), far);
    glUniform1i(glGetUniformLocation(program, "voxelDim"), vct.voxelDim);
    glUniform3fv(glGetUniformLocation(program, "voxelMin"), 1, glm::value_ptr(vct.min));
    glUniform3fv(glGetUniformLocation(program, "voxelMax"), 1, glm::value_ptr(vct.max));
    glUniform3fv(glGetUniformLocation(program, "voxelCenter"), 1, glm::value_ptr(vct.center));
    glUniform1i(glGetUniformLocation(program, "lod"), settings.miplevel);
    glUniform1i(glGetUniformLocation(program, "radiance"), settings.drawRadiance);


    GLQuad::draw();

    glUseProgram(0);
}

void Application::debugVoxels(GLuint texture_id, const glm::mat4 &mvp) {
    static const float point[] = { 0.0f, 0.0f, 0.0f };
    static GLuint vao = 0;
    static GLShaderProgram *shader = nullptr;
    if (!shader) {
        GLuint vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(point), point, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid *)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        shader = new GLShaderProgram();
        shader->attachAndLink({SHADER_DIR "debugVoxels.vert", SHADER_DIR "debugVoxels.geom", SHADER_DIR "debugVoxels.frag"});
        shader->setObjectLabel("Debug Voxels");
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    if (settings.debugVoxelOpacity) {
        glEnable(GL_BLEND);
    }

    shader->bind();

    shader->setUniformMatrix4fv("mvp", mvp);
    shader->setUniform1i("level", settings.miplevel);
    shader->setUniform1f("voxelDim", vct.voxelDim);
    shader->setUniform3fv("voxelMin", vct.min);
    shader->setUniform3fv("voxelMax", vct.max);
    shader->setUniform3fv("voxelCenter", vct.center);
    shader->setUniform1i("debugOpacity", settings.debugVoxelOpacity);

    glBindTextureUnit(0, texture_id);

    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_POINTS, 0, 1, vct.voxelDim * vct.voxelDim * vct.voxelDim);
    glBindVertexArray(0);

    glBindTextureUnit(0, 0);

    shader->unbind();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    if (settings.debugVoxelOpacity) {
        glDisable(GL_BLEND);
    }
}