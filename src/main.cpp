#include <Graphics/opengl.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "common.h"
#include "Application.h"
#include "Graphics/GLHelper.h"
#include "Input/GLFWHandler.h"

#include <stb_image.h>

#include <glm/gtx/string_cast.hpp>
#include <Graphics/GLQuad.h>

GLFWwindow *init_window(unsigned width, unsigned height, const char *title);

int main() {
    GLFWwindow *window = init_window(WIDTH, HEIGHT, TITLE);
#if 0
    using namespace std;
    using namespace glm;

    // TEST WARP TEXTURE (do entirely on CPU for now???)
    // Create 2D texture and manually populate values
    const int warpDim = 4;
    float warpTexture[warpDim][warpDim] = {
        0.f, 1.f, 1.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 1.f, 0.f,
        0.f, 1.f, 0.f, 0.f
    };

    float occupancyTexture[warpDim * 2][warpDim * 2] {
        0, 0,   1, 0,   1, 1,   1, 1,
        0, 0,   1, 1,   0, 1,   1, 1,

        0, 0,   1, 1,   0, 0,   1, 0,
        0, 0,   0, 0,   0, 0,   0, 0,

        0, 0,   0, 0,   1, 1,   0, 0,
        0, 0,   1, 1,   1, 1,   0, 0,

        0, 0,   0, 0,   0, 0,   0, 0,
        0, 0,   1, 1,   0, 0,   0, 0
    };

    // Compute partial sum table
    int warpPartialsX[warpDim][warpDim], warpPartialsY[warpDim][warpDim];

    // partials for x: sum along rows
    for (int row = 0; row < warpDim; row++) {
        int sumX = 0;
        for (int x = 0; x < warpDim; x++) {
            sumX += (warpTexture[row][x] > 0.5f) ? 1 : 0;
            warpPartialsX[row][x] = sumX;
        }
    }

    // partials for y: sum along columns
    for (int col = 0; col < warpDim; col++) {
        int sumY = 0;
        for (int y = 0; y < warpDim; y++) {
            sumY += (warpTexture[y][col] > 0.5f) ? 1 : 0;
            warpPartialsY[y][col] = sumY;
        }
    }

    // Calculate weights (index by # occupied cells, low and then high)
    float warpWeights[2][warpDim+1];
    for (int occupied = 0; occupied <= warpDim; occupied++) {
        if (occupied == 0 || occupied == warpDim) {
            // Linear scale if a row is all empty or all occupied
            warpWeights[0][occupied] = warpWeights[1][occupied] = 1.f;
        }
        else {
            float l = 0.5f;                     // using constant value for low resolution right now
            int empty = warpDim - occupied;     // number of empty cells along row
            int total = warpDim;                // total number of cells along row
            // Solve for h: l * empty + h * occupied = total --> h = (total - l * empty) / occupied
            float h = (total - l * empty) / (float)occupied;

            warpWeights[0][occupied] = l;
            warpWeights[1][occupied] = h;
        }
    }

    // Test calculating some offsets
    vec2 linearTexcoord {0.1f, 1.5f};   // integral portion is warp cell index, fractional is position within warp cell
                                        // will need to convert to texture space for final implementation
    auto calculateWarpPosition = [&](vec2 linearTexcoord) {
        linearTexcoord *= warpDim;      // convert [0, 1] -> [0, warpDim] for indexing into warp texture
        vec2 warpCellIndex;
        vec2 warpCellPosition = modf(linearTexcoord, warpCellIndex);
        int x = (int)warpCellIndex.x, y = (int)warpCellIndex.y;
        cout << "\twarpCellIndex\t\t" << to_string(warpCellIndex) << endl;
        cout << "\twarpCellPosition\t" << to_string(warpCellPosition) << endl;

        bool warpCellOccupied = warpTexture[y][x] > 0.5f;
        vec2 totalOccupied { warpPartialsX[y][warpDim - 1], warpPartialsY[warpDim - 1][x] };
        vec2 partialSum { warpPartialsX[y][x], warpPartialsY[y][x] };
        cout << "\twarpCellOccupied\t" << warpCellOccupied << endl;
        cout << "\ttotalOccupied\t\t" << to_string(totalOccupied) << endl;
        cout << "\tpartialSum\t\t" << to_string(partialSum) << endl;

        vec2 warpCellWeightsLow { warpWeights[0][(int)totalOccupied.x], warpWeights[0][(int)totalOccupied.y] };
        vec2 warpCellWeightsHigh { warpWeights[1][(int)totalOccupied.x], warpWeights[1][(int)totalOccupied.y] };
        vec2 warpCellResolution = warpCellOccupied ? warpCellWeightsHigh : warpCellWeightsLow;
        cout << "\twarpCellWeightsLow\t" << to_string(warpCellWeightsLow) << endl;
        cout << "\twarpCellWeightsHigh\t" << to_string(warpCellWeightsHigh) << endl;
        cout << "\twarpCellResolution\t" << to_string(warpCellResolution) << endl;

        // TODO offsets don't seem right. partialSum should definitely be used somewhere...
        // TODO try calculating width and height of each type of warp cell from weights --> make calculating offset easier
        // if totalOccupied = 0 or warpDim then it doesn't matter which we choose (edge case for all linear cells)
        if (totalOccupied.x == 0) totalOccupied.x = warpDim;
        if (totalOccupied.y == 0) totalOccupied.y = warpDim;
        vec2 previousPartial = warpCellOccupied ? partialSum - vec2(1) : partialSum;
        // vec2 warpCellOffset = warpCellIndex * mix(warpCellWeightsLow, warpCellWeightsHigh, previousPartial / totalOccupied);
        vec2 warpCellOffset = warpCellWeightsLow * (warpCellIndex - previousPartial) + warpCellWeightsHigh * previousPartial;
        vec2 warpCellInnerOffset = warpCellPosition * warpCellResolution;
        cout << "\twarpCellOffset\t\t" << to_string(warpCellOffset) << endl;
        cout << "\twarpCellInnerOffset\t" << to_string(warpCellInnerOffset) << endl;

        vec2 warpedTexcoord = warpCellOffset + warpCellInnerOffset;
        return warpedTexcoord / (float)warpDim;
    };

    GLQuad::init();
    GLShaderProgram shader {"Test warp texture", {SHADER_DIR "quad.vert", SHADER_DIR "testWarpTexture.frag"}};
    GLint toggle = 1, toggleFilter = 0;
    GLFramebuffer warpmapFBO;
    warpmapFBO.bind();
    warpmapFBO.attachTexture(
        GL_COLOR_ATTACHMENT0, GL_RGBA8, warpDim, warpDim, GL_RGBA, GL_UNSIGNED_BYTE,
        GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE
    );
    GLuint attachments[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, attachments);
    if (warpmapFBO.getStatus() != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("warpmapFBO not created successfully");
    }
    warpmapFBO.unbind();
    GLShaderProgram warpmapShader {"Warpmap", {SHADER_DIR "quad.vert", SHADER_DIR "generateWarpmap.frag"}};

    glViewport(0, 0, WIDTH, HEIGHT);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Mouse::update();

        if (Mouse::getMouseButtonClick(GLFW_MOUSE_BUTTON_LEFT)) {
            vec2 position { Mouse::getX() / WIDTH, 1 - Mouse::getY() / HEIGHT };
            cout << to_string(position) << ":" << endl;
            vec2 warpedPosition = calculateWarpPosition(position);
            cout << "\twarpedPosition\t\t" << to_string(warpedPosition) << endl << endl;
        }
        if (Keyboard::getKeyTap(GLFW_KEY_T)) {
            toggle = !toggle;
            cout << toggle << endl;
        }
        if (Keyboard::getKeyTap(GLFW_KEY_F)) {
            toggleFilter = !toggleFilter;
            cout << toggleFilter << endl;
        }

        glViewport(0, 0, warpDim, warpDim);
        warpmapFBO.bind();
        warpmapShader.bind();
        glUniform1fv(warpmapShader.uniformLocation("warpTexture[0]"), warpDim * warpDim, (GLfloat *)warpTexture);
        glUniform1fv(warpmapShader.uniformLocation("occupancyTexture[0]"), (warpDim * 2) * (warpDim * 2), (GLfloat *)occupancyTexture);
        glUniform1iv(warpmapShader.uniformLocation("warpPartialsX[0]"), warpDim * warpDim, (GLint *)warpPartialsX);
        glUniform1iv(warpmapShader.uniformLocation("warpPartialsY[0]"), warpDim * warpDim, (GLint *)warpPartialsY);
        glUniform1fv(warpmapShader.uniformLocation("warpWeights[0]"), 2 * (warpDim + 1), (GLfloat *)warpWeights);
        GLQuad::draw();
        warpmapShader.unbind();
        warpmapFBO.unbind();

        glViewport(0, 0, WIDTH, HEIGHT);
        shader.bind();
        glBindTextureUnit(0, warpmapFBO.getTexture(0));
        glUniform1fv(shader.uniformLocation("warpTexture[0]"), warpDim * warpDim, (GLfloat *)warpTexture);
        glUniform1fv(shader.uniformLocation("occupancyTexture[0]"), (warpDim * 2) * (warpDim * 2), (GLfloat *)occupancyTexture);
        glUniform1iv(shader.uniformLocation("warpPartialsX[0]"), warpDim * warpDim, (GLint*)warpPartialsX);
        glUniform1iv(shader.uniformLocation("warpPartialsY[0]"), warpDim * warpDim, (GLint*)warpPartialsY);
        glUniform1fv(shader.uniformLocation("warpWeights[0]"), 2 * (warpDim + 1), (GLfloat *)warpWeights);
        glUniform2f(shader.uniformLocation("click"),
            Mouse::getMouseLastClickX(GLFW_MOUSE_BUTTON_LEFT) / WIDTH,
            1 - Mouse::getMouseLastClickY(GLFW_MOUSE_BUTTON_LEFT) / HEIGHT
        );
        shader.setUniform1i("toggle", toggle);
        shader.setUniform1i("toggleFilter", toggleFilter);
        GLQuad::draw();
        shader.unbind();
        glBindTextureUnit(0, 0);

        glfwSwapBuffers(window);
    }

    return 0;
#endif

    // stbi_set_flip_vertically_on_load(true);

    Application app {window};

    app.init();
    LOG_INFO("Application initialized");

    float dt, lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        app.update(dt);
        app.render(dt);

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
    glfwWindowHint(GLFW_SAMPLES, 4);

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
