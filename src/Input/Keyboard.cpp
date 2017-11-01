#include "Keyboard.h"

bool Keyboard::key_tap[GLFW_KEY_LAST];
int Keyboard::keys[GLFW_KEY_LAST];

int Keyboard::getKey(int key) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    return Keyboard::keys[key];
}

bool Keyboard::getKeyDown(int key) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    return Keyboard::keys[key] >= GLFW_PRESS;
}

bool Keyboard::getKeyUp(int key) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    return Keyboard::keys[key] == GLFW_RELEASE;
}

void Keyboard::setKeyStatus(int key, int action) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    Keyboard::key_tap[key] = (Keyboard::keys[key] == GLFW_RELEASE && action == GLFW_PRESS);
    Keyboard::keys[key] = action;
}

bool Keyboard::getKeyTap(int key) {
    assert(key >= 0 && key <= GLFW_KEY_LAST);
    bool result = Keyboard::key_tap[key];
    Keyboard::key_tap[key] = false;
    return result;
}
