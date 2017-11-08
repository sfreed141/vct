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

#include "Input/Keyboard.h"
#include "Input/Mouse.h"

#include "Overlay.h"
#include "Camera.h"

#include "common.h"

class Application {
public:
    friend class Overlay;

    Application(GLFWwindow *window) : window{window}, ui{window, *this} {}

    void init() {
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);

        glClearColor(0.5294f, 0.8078f, 0.9216f, 1.0f);
        glEnable(GL_DEPTH_TEST);

        /* mesh = make_unique<Mesh>(RESOURCE_DIR "cubes.obj"); */
        mesh = std::make_unique<Mesh>(RESOURCE_DIR "sponza/sponza_small.obj");

        program.linkProgram(SHADER_DIR "simple.vert", SHADER_DIR "phong.frag");
        voxelProgram.linkProgram(SHADER_DIR "voxelize.vert", SHADER_DIR "voxelize.frag", SHADER_DIR "voxelize.geom");

        camera.position = glm::vec3(5, 1, 0);
        camera.yaw = 180.0f;

        // Initialize voxel textures
        voxelColor = make3DTexture(voxelDim);
        voxelNormal = make3DTexture(voxelDim);
    }

    void update(float dt) {
        Mouse::update();

        if (GLFW_PRESS == Keyboard::getKey(GLFW_KEY_ESCAPE)) {
            glfwSetWindowShouldClose(window, 1);
        }
        else if (Keyboard::getKeyTap(GLFW_KEY_LEFT_CONTROL)) {
            int mode = glfwGetInputMode(window, GLFW_CURSOR);
            glfwSetInputMode(window, GLFW_CURSOR, (mode == GLFW_CURSOR_DISABLED) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
        }
        else if (Keyboard::getKeyTap(GLFW_KEY_GRAVE_ACCENT)) {
            ui.enabled = !ui.enabled;
        }

        if (GLFW_CURSOR_DISABLED == glfwGetInputMode(window, GLFW_CURSOR)) {
            camera.update(dt);
        }
    }

    void render(float dt) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render scene
        {
            GL_DEBUG_PUSH("Render Scene")
            glm::mat4 projection = glm::perspective(camera.fov, (float)width / height, near, far);
            glm::mat4 view = camera.lookAt();
            glm::mat4 model;

            glm::vec3 lightPos {100.0f, 100.0f, 0.0f};
            glm::vec3 lightInt {1.0f, 1.0f, 1.0f};

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

            mesh->draw(program.getHandle());
            program.unbind();
            GL_DEBUG_POP()
        }

        // Voxelize scene
        {
            GL_DEBUG_PUSH("Voxelization Pass")
            // Set OpenGL state
            glViewport(0, 0, voxelDim, voxelDim);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glDepthMask(GL_FALSE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            voxelProgram.bind();

            // TODO: ortho values, don't need to send each frame
            // Make and bind mvp matrices for each axis
            glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, near, far);
            glm::mat4 mvp_x = projection * glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0, 1, 0));
            glm::mat4 mvp_y = projection * glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(1, 0, 0));
            glm::mat4 mvp_z = projection * glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0, 1, 0));
            glUniformMatrix4fv(voxelProgram.uniformLocation("mvp_x"), 1, GL_FALSE, value_ptr(mvp_x));
            glUniformMatrix4fv(voxelProgram.uniformLocation("mvp_y"), 1, GL_FALSE, value_ptr(mvp_y));
            glUniformMatrix4fv(voxelProgram.uniformLocation("mvp_z"), 1, GL_FALSE, value_ptr(mvp_z));

            // Bind voxel grids
            glBindImageTexture(0, voxelColor, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
            glBindImageTexture(1, voxelNormal, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);

            // Render geometry
            mesh->draw(voxelProgram.getHandle());

            voxelProgram.unbind();

            // Restore OpenGL state
            glViewport(0, 0, width, height);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glDepthMask(GL_TRUE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            GL_DEBUG_POP();
        }

        // Render overlay
        {
            GL_DEBUG_PUSH("Render Overlay")
            ui.render(dt);
            GL_DEBUG_POP()
        }
    }

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
};

#endif
