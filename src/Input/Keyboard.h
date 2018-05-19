/*
 * http://www.glfw.org/docs/latest/input_guide.html#input_keyboard
 * https://docs.unity3d.com/ScriptReference/Input.html
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <GLFW/glfw3.h>
#include <cassert>

class Keyboard {
public:
    static int getKey(int key);

    static bool getKeyDown(int key);

    static bool getKeyUp(int key);

    static bool getKeyTap(int key);

    static void setKeyStatus(int key, int action);

private:
    Keyboard() {}

    static bool key_tap[GLFW_KEY_LAST];
    static int keys[GLFW_KEY_LAST];
};

#endif
