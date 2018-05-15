#include "Mouse.h"

double Mouse::lastX = 0;
double Mouse::lastY = 0;
double Mouse::x = 0;
double Mouse::y = 0;
int Mouse::buttons[GLFW_MOUSE_BUTTON_LAST] = {GLFW_RELEASE};

static double nX, nY;

void Mouse::update() {
    Mouse::lastX = Mouse::x;
    Mouse::lastY = Mouse::y;

    Mouse::x = nX;
    Mouse::y = nY;
}

double Mouse::getX() {
    return Mouse::x;
}

double Mouse::getY() {
    return Mouse::y;
}

double Mouse::getDeltaX() {
    return Mouse::x - Mouse::lastX;
}
double Mouse::getDeltaY() {
    return Mouse::lastY - Mouse::y;
}

int Mouse::getMouseButton(int button) {
    assert(button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST);
    return Mouse::buttons[button];
}

bool Mouse::getMouseButtonDown(int button) {
    assert(button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST);
    return Mouse::buttons[button] == GLFW_PRESS;
}

bool Mouse::getMouseButtonUp(int button) {
    assert(button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST);
    return Mouse::buttons[button] == GLFW_RELEASE;
}

void Mouse::setMousePos(double nextX, double nextY) {
    nX = nextX;
    nY = nextY;
}

void Mouse::setMouseButton(int button, int action) {
    assert(button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST);
    Mouse::buttons[button] = action;
}