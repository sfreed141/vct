/*
 * http://www.glfw.org/docs/latest/input_guide.html
 */

#ifndef GLFWHANDLER_H
#define GLFWHANDLER_H

#include <GLFW/glfw3.h>

#include "Mouse.h"
#include "Keyboard.h"

class GLFWHandler {
public:
    static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
    static void mouse_callback(GLFWwindow *window, double xpos, double ypos);
    static void mousebtn_callback(GLFWwindow *window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
    static void char_callback(GLFWwindow *window, unsigned int codepoint);
};

#endif
