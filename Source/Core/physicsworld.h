#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "btBulletDynamicsCommon.h"

#include "Core/Deserializable.h"
#include "Core/collisioninfo.h"

class Entity;
class PhysicsComponent;
class PhysicsWorldDeserializer;

class PhysicsWorld : public Deserializable
{
public:
    friend class PhysicsComponent;
    friend class PhysicsWorldDeserializer;

    PhysicsWorld();

    SerializedObject Serialize() const override;
    glm::vec3 GetGravity() const;
    void SetGravity(glm::vec3 const &);

    void Tick(float dt);

private:
    static void s_TickCallback(btDynamicsWorld * world, btScalar timestep);

    std::unordered_map<Entity *, std::unordered_map<Entity *, CollisionInfo>> collisionsLastFrame;

    std::unique_ptr<btBroadphaseInterface> broadphase;
    std::unique_ptr<btCollisionConfiguration> collisionConfig;
    std::unique_ptr<btConstraintSolver> constraintSolver;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDiscreteDynamicsWorld> world;
};
