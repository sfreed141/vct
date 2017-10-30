/*
 * http://www.glfw.org/docs/latest/input_guide.html#input_keyboard
 * https://docs.unity3d.com/ScriptReference/Input.html
 */
#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <GLFW/glfw3.h>
#include <cassert>

class Keyboard {
public:
    static int getKey(int key);

    static int getKeyDown(int key);

    static int getKeyUp(int key);
    
    static void setKeyStatus(int key, int action);

private:
    static int keys[GLFW_KEY_LAST];
};

#endif