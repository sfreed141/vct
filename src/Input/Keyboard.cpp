#include "Keyboard.h"

int Keyboard::keys[GLFW_KEY_LAST];

int Keyboard::getKey(int key) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    return Keyboard::keys[key];
}

int Keyboard::getKeyDown(int key) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    return Keyboard::keys[key] >= GLFW_PRESS;
}

int Keyboard::getKeyUp(int key) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    return Keyboard::keys[key] == GLFW_RELEASE;
}

void Keyboard::setKeyStatus(int key, int action) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    Keyboard::keys[key] = action;
}