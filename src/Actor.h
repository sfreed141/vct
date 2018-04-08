#ifndef ACTOR_H
#define ACTOR_H

#include <glm/glm.hpp>

#include <string>
#include <iostream>

#include "ResourceLoader.h"
#include "Graphics/Mesh.h"
#include "Transform.h"
#include "log.h"

class GLShaderProgram;
class Actor;

class ActorController {
public:
    virtual void update(Actor &actor, float dt) = 0;
};

class Actor {
public:
    virtual void update(float dt) { if (controller) controller->update(*this, dt); }
    virtual void draw(GLShaderProgram &program) const {}

    const glm::mat4 &getTransform() { return transform.getMatrix(); }

    ActorController *controller = nullptr;

    Transform transform;
};

class StaticMeshActor : public Actor {
public:
    StaticMeshActor(const std::string &meshname) : Actor(), mesh(ResourceLoader::loadMesh(meshname)) {}

    void draw(GLShaderProgram &program) const override { mesh->draw(program); }

    MeshResource mesh;
};

class LambdaActorController : public ActorController {
public:
    using UpdateFnType = void (*)(Actor &, float, float);

    LambdaActorController(UpdateFnType lambda) : updateFn(lambda) {}

    void update(Actor &actor, float dt) override {
        static float time = 0.0f;
        time += dt;
        updateFn(actor, dt, time);
    }

    UpdateFnType updateFn;
};

#endif
