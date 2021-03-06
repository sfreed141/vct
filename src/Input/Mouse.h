/*
 * http://www.glfw.org/docs/latest/input_guide.html
 * https://docs.unity3d.com/ScriptReference/Input.html
 */

#ifndef MOUSE_H
#define MOUSE_H

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
    static bool getMouseButtonClick(int button);
    static double getMouseLastClickX(int button);
    static double getMouseLastClickY(int button);

    static void setMousePos(double nextX, double nextY);
    static void setMouseButton(int button, int action);

private:
    Mouse() {}
    static void setMouseClickPos(int button, double x, double y);

    static double lastX, lastY, x, y;
    static double lastClickX[GLFW_MOUSE_BUTTON_LAST];
    static double lastClickY[GLFW_MOUSE_BUTTON_LAST];
    static bool button_click[GLFW_MOUSE_BUTTON_LAST];
    static int buttons[GLFW_MOUSE_BUTTON_LAST];
};

#endif
