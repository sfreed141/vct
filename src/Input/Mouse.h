/*
 * http://www.glfw.org/docs/latest/input_guide.html
 * https://docs.unity3d.com/ScriptReference/Input.html
 */

#ifndef MOUSE_HPP
#define MOUSE_HPP

#include <GLFW/glfw3.h>
#include <cassert>

class Mouse {
public:
    static void update();
    
    static double getX();
    static double getY();

    static double getDeltaX();
    static double getDeltaY();

    static int getMouseButton(int button);
    static bool getMouseButtonDown(int button);
    static bool getMouseButtonUp(int button);

    static void setMousePos(double nextX, double nextY);
    static void setMouseButton(int button, int action);

private:
    static double lastX, lastY, x, y;
    static int buttons[GLFW_MOUSE_BUTTON_LAST];
};

#endif