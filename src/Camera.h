#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
    void update(float dt);
    glm::mat4 lookAt() const;

    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 front {0.0f, 0.0f, -1.0f};
    glm::vec3 up {0.0f, 1.0f, 0.0f};

    float pitch = 0.0f;
    float yaw = -90.0f;

    float fov = 45.0f;

    const float speed = 10.0f;
    const float sensitivity = 0.05f;
};

#endif
