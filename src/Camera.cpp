#include "Camera.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include <glm/gtc/matrix_transform.hpp>

void Camera::update(float dt) {
    float move_speed = speed * dt;
    if (Keyboard::getKeyDown(GLFW_KEY_LEFT_SHIFT))
        move_speed *= 2;

    if (Keyboard::getKeyDown(GLFW_KEY_W))
        position += move_speed * front;
    if (Keyboard::getKeyDown(GLFW_KEY_S))
        position -= move_speed * front;
    if (Keyboard::getKeyDown(GLFW_KEY_A))
        position -= move_speed * glm::normalize(glm::cross(front, up));
    if (Keyboard::getKeyDown(GLFW_KEY_D))
        position += move_speed * glm::normalize(glm::cross(front, up));
    if (Keyboard::getKeyDown(GLFW_KEY_SPACE))
        position += move_speed * up;
    if (Keyboard::getKeyDown(GLFW_KEY_C))
        position -= move_speed * up;

    float xoffset = sensitivity * Mouse::getDeltaX();
    float yoffset = sensitivity * Mouse::getDeltaY();

    pitch = glm::clamp(pitch + yoffset, -89.0f, 89.0f);
    yaw = yaw + xoffset;

    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    front.y = sin(glm::radians(pitch));
    front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
}

glm::mat4 Camera::lookAt() const {
    return glm::lookAt(position, position + front, up);
}
