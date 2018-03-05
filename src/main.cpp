#include <Graphics/opengl.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "common.h"
#include "Application.h"
#include "Graphics/GLHelper.h"
#include "Input/GLFWHandler.h"

// #define RUN_TEST_TEXTURE2D
// #define RUN_TEST_TEXTURE3D
#include <cmath>
#include <Graphics/GLQuad.h>
#include <Graphics/GLTimer.h>
#include <stb_image.h>
#include <Graphics/GLShaderProgram.h>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

GLFWwindow *init_window(unsigned width, unsigned height, const char *title);

int main() {
    GLFWwindow *window = init_window(WIDTH, HEIGHT, TITLE);

    stbi_set_flip_vertically_on_load(true);

#if defined(RUN_TEST_TEXTURE2D)
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
        "#version 330\n"
        "in vec2 fragTexcoord;\n"
        "out vec4 color;\n"
        "uniform sampler2D texture0;\n"
        "uniform float lod = 0.0;\n"
        "void main() {\n"
        "color = vec4(textureLod(texture0, fragTexcoord, lod).rgb, 1);\n"
        "}\n";

    GLQuad::init();
    GLuint shaders[2], program;
    shaders[0] = GLHelper::createShaderFromString(GL_VERTEX_SHADER, vert);
    shaders[1] = GLHelper::createShaderFromString(GL_FRAGMENT_SHADER, frag);
    program = glCreateProgram();
    glAttachShader(program, shaders[0]);
    glAttachShader(program, shaders[1]);
    glLinkProgram(program);
    if (!GLHelper::checkShaderProgramStatus(program)) {
        return 1;
    }

    int width = 7680, height = 4320;
    GLuint t;
    GLuint maxlevel = 4;
    glCreateTextures(GL_TEXTURE_2D, 1, &t);
    glTextureStorage2D(t, maxlevel, GL_RGBA8, width, height);
    // glTextureSubImage2D(t, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTextureParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(t, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glGenerateTextureMipmap(t);

    GLuint t2;
    glCreateTextures(GL_TEXTURE_2D, 1, &t2);
    glTextureStorage2D(t2, maxlevel, GL_RGBA8, width, height);
    // glTextureSubImage2D(t2, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTextureParameteri(t2, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(t2, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateTextureMipmap(t2);

    int width3, height3, channels;
    unsigned char *image = stbi_load("/home/sam/Pictures/occupancy.png", &width3, &height3, &channels, 1);
    if (image == NULL) {
        LOG_ERROR("TEXTURE::LOAD_FAILED");
    }
    GLuint t3;
    glCreateTextures(GL_TEXTURE_2D, 1, &t3);
    glTextureStorage2D(t3, 1, GL_R8, width3, height3);
    glTextureSubImage2D(t3, 0, 0, 0, width3, height3, GL_RED, GL_UNSIGNED_BYTE, image);
    glTextureParameteri(t3, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(t3, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateTextureMipmap(t3);

    int color_width, color_height;
    image = stbi_load(RESOURCE_DIR "color_gradient.png", &color_width, &color_height, &channels, 1);
    GLuint color_texture;
    glCreateTextures(GL_TEXTURE_1D, 1, &color_texture);
    glTextureStorage1D(color_texture, 1, GL_R8, color_width);
    glTextureSubImage1D(color_texture, 0, 0, color_width, GL_RED, GL_UNSIGNED_BYTE, image);
    glTextureParameteri(color_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(color_texture, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);

    GLShaderProgram mandelbrotProgram ({SHADER_DIR "mandelbrot.comp"});
    GLShaderProgram filterProgram ({SHADER_DIR "filter2d.comp"});

    float aspect = (float) WIDTH / HEIGHT;
    // glm::vec2 center {0, 0};
    // float scale = 0.0f;
    glm::vec2 center {-.232464, -.531013};
    float scale = 0.00288213f;

    GLBufferedTimer mipmapQuery{}, renderQuery{}, totalQuery{}, mandelbrotQuery{};

#elif defined(RUN_TEST_TEXTURE3D)
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
        "#version 450\n"
        "in vec2 fragTexcoord;\n"
        "out vec4 color;\n"
        "layout(binding = 0, rgba8) uniform image2D image0;\n"
        "void main() {\n"
        "color = vec4(imageLoad(image0, ivec2(fragTexcoord * imageSize(image0).xy)).rgb, 1);\n"
        "}\n";

    GLQuad::init();
    GLuint shaders[2], program;
    shaders[0] = GLHelper::createShaderFromString(GL_VERTEX_SHADER, vert);
    shaders[1] = GLHelper::createShaderFromString(GL_FRAGMENT_SHADER, frag);
    glObjectLabel(GL_SHADER, shaders[0], -1, "vertShader");
    program = glCreateProgram();
    glAttachShader(program, shaders[0]);
    glAttachShader(program, shaders[1]);
    glLinkProgram(program);
    if (!GLHelper::checkShaderProgramStatus(program)) {
        return 1;
    }
    glObjectLabel(GL_PROGRAM, program, -1, "quadProgram");

    GLuint t;
    GLuint maxlevel = 2;
    GLsizei size = 4;
    glCreateTextures(GL_TEXTURE_3D, 1, &t);
    glTextureStorage3D(t, maxlevel, GL_RGBA8, size, size, size);
    glTextureParameteri(t, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(t, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glGenerateTextureMipmap(t);

    GLuint t2;
    glCreateTextures(GL_TEXTURE_3D, 1, &t2);
    glTextureStorage3D(t2, maxlevel, GL_RGBA8, size, size, size);
    glTextureParameteri(t2, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(t2, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    for (int depth = 0; depth < size; depth++) {
        struct {unsigned char r,g,b,a;} image[size * size];

        for (int i = 0; i < size * size; i ++) {
            if (i % 4 == 0) {
                image[i].r = 255;
                image[i].g = 255;
                image[i].b = 255;
            }
            else if (i % 2 == 0) {
                image[i].r = 128;
                image[i].g = 128;
                image[i].b = 128;
            }
            else {
                image[i].r = 0;
                image[i].g = 0;
                image[i].b = 0;
            }
            image[i].r *= depth == 0 ? 1 : 0;
            image[i].g *= depth == 1 ? 1 : 0;
            image[i].b *= depth == 2 ? 1 : 0;
            image[i].a = 255;
        }
        glTextureSubImage3D(t, 0, 0, 0, depth, size, size, 1, GL_RGBA, GL_UNSIGNED_BYTE, image);
        glTextureSubImage3D(t2, 0, 0, 0, depth, size, size, 1, GL_RGBA, GL_UNSIGNED_BYTE, image);
    }

    glGenerateTextureMipmap(t2);

    // int width3, height3;
    // image = stbi_load("/home/sam/Pictures/occupancy.png", &width3, &height3, &channels, 1);
    // if (image == NULL) {
    //     LOG_ERROR("TEXTURE::LOAD_FAILED");
    // }
    // GLuint t3;
    // glCreateTextures(GL_TEXTURE_3D, 1, &t3);
    // glTextureStorage2D(t3, 1, GL_R8, width3, height3);
    // glTextureSubImage2D(t3, 0, 0, 0, width3, height3, GL_RED, GL_UNSIGNED_BYTE, image);
    // glTextureParameteri(t3, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTextureParameteri(t3, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glGenerateTextureMipmap(t3);

    GLShaderProgram filterProgram ({SHADER_DIR "filter3d.comp"});
#else
    Application app {window};

    app.init();
    LOG_INFO("Application initialized");
#endif

    float dt, lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

#if defined(RUN_TEST_TEXTURE2D)
        Mouse::update();


        mipmapQuery.getQueryResult();
        renderQuery.getQueryResult();
        totalQuery.getQueryResult();
        mandelbrotQuery.getQueryResult();

        int lod = (int)currentFrame % maxlevel;
        fprintf(stderr, "\rfps: %.2f, frametime: %0.2f, lod: %d, miptime: %0.2fms, rendertime: %0.2fms, mandelbrot: %0.2fms, totaltime: %0.2fms",
            1.0 / dt, dt * 1000.0f, lod,
            mipmapQuery.getTime() / 1.0e6,
            renderQuery.getTime() / 1.0e6,
            mandelbrotQuery.getTime() / 1.0e6,
            totalQuery.getTime() / 1.0e6
        );

        if (Mouse::getMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
            center += glm::vec2(Mouse::getDeltaX(), Mouse::getDeltaY()) * 0.001f * scale;
            std::cout << glm::to_string(center) << std::endl;
        }
        if (Mouse::getMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            scale += Mouse::getDeltaX() * 0.1f * scale;
            std::cout << scale << std::endl;
        }
        
        totalQuery.start();
        mandelbrotQuery.start();
        {
            mandelbrotProgram.bind();
            glBindImageTexture(0, t, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
            glBindTextureUnit(0, color_texture);
            mandelbrotProgram.setUniform2f("center", center.x, center.y);
            mandelbrotProgram.setUniform1f("scale", scale);
            GLuint num_groups_x = (width + 16 - 1) / 16;
            GLuint num_groups_y = (height + 16 - 1) / 16;
            glDispatchCompute(num_groups_x, num_groups_y, 1);
            glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
            glBindTextureUnit(0, 0);
            mandelbrotProgram.bind();
        }
        mandelbrotQuery.stop();

        mipmapQuery.start();
        {
            filterProgram.bind();
            glBindTextureUnit(1, t3);
            int w = width, h = height;
            for (int level = 0; level < maxlevel; level++) {
                glBindImageTexture(0, t, level, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
                glBindImageTexture(1, t, level + 1, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
                GLuint num_groups_x = ((w >> 1) + 16 - 1) / 16;
                GLuint num_groups_y = ((h >> 1) + 16 - 1) / 16;
                glDispatchCompute(num_groups_x, num_groups_y, 1);
                // glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
                // glBindImageTexture(1, 0, 1, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

                w >>= 1;
                h >>= 1;
            }
            filterProgram.unbind();
        }
        // glGenerateTextureMipmap(t);
        mipmapQuery.stop();

        renderQuery.start();
        glUseProgram(program);
        glBindTextureUnit(0, t);//lod == 0 ? t : t2);
        glUniform1f(glGetUniformLocation(program, "lod"), lod);
        GLQuad::draw();
        glBindTextureUnit(0, 0);
        glUseProgram(0);
        renderQuery.stop();
        totalQuery.stop();

#elif defined(RUN_TEST_TEXTURE3D)
        int lod = (int)currentFrame % maxlevel;
        int slice = (int)(currentFrame / 2) % size;
        fprintf(stderr, "\rfps: %.2f, frametime: %0.2f, lod: %d, slice: %d", 1.0 / dt, dt * 1000.0f, lod, slice);

        filterProgram.bind();
        // glBindImageTexture(0, t, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
        glBindTextureUnit(0, t);
        // glBindTextureUnit(1, t3);
        glBindImageTexture(1, t, 1, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);

        GLuint num_groups_x = ((size >> 1) + 8 - 1) / 8;
        GLuint num_groups_y = ((size >> 1) + 8 - 1) / 8;
        GLuint num_groups_z = ((size >> 1) + 8 - 1) / 8;
        glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);

        // glBindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA8);
        glBindTextureUnit(0, 0);
        glBindTextureUnit(1, 0);
        glBindImageTexture(1, 0, 1, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
        filterProgram.unbind();
        glGenerateTextureMipmap(t2);

        glUseProgram(program);
        glBindImageTexture(0, t, lod, GL_FALSE, slice, GL_READ_WRITE, GL_RGBA8);//lod == 0 ? t : t2);
        GLQuad::draw();
        glBindTextureUnit(0, 0);
        glUseProgram(0);
#else
        app.update(dt);
        app.render(dt);
#endif

        glfwSwapBuffers(window);
    }

    LOG_INFO("Exitting...");

    glfwTerminate();

    return 0;
}

GLFWwindow *init_window(unsigned width, unsigned height, const char *title) {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        LOG_ERROR("Failed to create GLFW window");

        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, GLFWHandler::key_callback);
    glfwSetCursorPosCallback(window, GLFWHandler::mouse_callback);
    glfwSetMouseButtonCallback(window, GLFWHandler::mousebtn_callback);
    glfwSetScrollCallback(window, GLFWHandler::scroll_callback);
    glfwSetCharCallback(window, GLFWHandler::char_callback);

    glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        LOG_ERROR("Failed to initialize OpenGL context");
        exit(EXIT_FAILURE);
    }

    GLHelper::printGLInfo();

#ifndef NDEBUG
    GLHelper::registerDebugOutputCallback();
#endif

    return window;
}
