#include "GLFWHandler.h"

void GLFWHandler::key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    Keyboard::setKeyStatus(key, action);
}

void GLFWHandler::mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    static bool firstMouse = true;

    if (firstMouse) {
        Mouse::setMousePos(xpos, ypos);
        Mouse::update();
        firstMouse = false;
    }

    Mouse::setMousePos(xpos, ypos);
}

void GLFWHandler::mousebtn_callback(GLFWwindow *window, int button, int action, int mods) {
    Mouse::setMouseButton(button, action);
}

void GLFWHandler::scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
}

void GLFWHandler::char_callback(GLFWwindow *window, unsigned int codepoint) {
}