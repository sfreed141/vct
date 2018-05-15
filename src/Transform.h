#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

class Transform {
public:
    Transform(glm::vec3 position = glm::vec3(0.0f), glm::vec3 scale = glm::vec3(1.0f)) :
        localPosition(position), localRotation(glm::quat()), localScale(scale),
        forward(worldForward),
        isDirty(true), currMatrix(glm::mat4(1.0f))
    {}

    void rotate(float angle, const glm::vec3 &axis) {
        glm::quat rotation = glm::angleAxis(angle, axis);
        this->localRotation = glm::normalize(rotation * this->localRotation);
        forward = glm::mat3_cast(localRotation) * worldForward;
        isDirty = true;
    }

    void rotate(const glm::quat &rotation) {
        this->localRotation = glm::normalize(rotation * this->localRotation);
        forward = glm::mat3_cast(localRotation) * worldForward;
        isDirty = true;
    }

    void scale(const glm::vec3 &scale) {
        this->localScale *= scale;
        isDirty = true;
    }

    void translate(const glm::vec3 &offset) {
        this->localPosition += offset;
        isDirty = true;
    }

    const glm::vec3 &getPosition() const { return this->localPosition; }
    glm::vec3 getEulerAngles() const { return glm::eulerAngles(this->localRotation); }
    const glm::vec3 &getScale() const { return this->localScale; }

    const glm::mat4 &getMatrix() {
        if (isDirty) {
            computeMatrix();
            isDirty = false;
        }
        return this->currMatrix;
    }

    const glm::vec3 &getForward() const { return this->forward; }
    glm::vec3 getRight() const { return glm::cross(this->forward, worldUp); }
    glm::vec3 getUp() const { return glm::cross(this->getRight(), this->forward); }

    void setPosition(const glm::vec3 &position) {
        this->localPosition = position;
        this->isDirty = true;
    }

    void setY(float y) {
        this->localPosition.y = y;
        this->isDirty = true;
    }

    void setRotation(const glm::vec3 &euler) {
        this->localRotation = glm::normalize(glm::quat(euler));
        this->forward = glm::normalize(glm::mat3_cast(this->localRotation) * worldForward);
        this->isDirty = true;
    }

    void setScale(const glm::vec3 &scale) {
        this->localScale = scale;
        this->isDirty = true;
    }

    void setForward(const glm::vec3 &forward) {
        this->forward = glm::normalize(forward);
        this->localRotation = glm::rotation(worldForward, this->forward);
        this->localRotation = glm::normalize(this->localRotation);
    }

private:
    glm::vec3 localPosition;
    glm::quat localRotation;
    glm::vec3 localScale;

    glm::vec3 forward;

    glm::mat4 currMatrix;
    bool isDirty;

    void computeMatrix() {
        glm::mat4 translation, rotation, scale;
        translation = glm::translate(translation, localPosition);
        rotation = glm::mat4_cast(localRotation);
        scale = glm::scale(scale, localScale);

        this->currMatrix = translation * rotation * scale;
        this->isDirty = false;
    }

    const glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 worldRight = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 worldForward = glm::vec3(0.0f, 0.0f, -1.0f);
};

#endif