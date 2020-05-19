#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "btBulletDynamicsCommon.h"

#include "Core/Deserializable.h"
#include "Core/collisioninfo.h"

class Entity;
class PhysicsWorldDeserializer;

class PhysicsWorld : public Deserializable
{
public:
    friend class PhysicsWorldDeserializer;

    static Deserializable * s_Deserialize(DeserializationContext * deserializationContext,
                                          SerializedObject const & str);

    PhysicsWorld() { this->type = "PhysicsWorld"; }

    SerializedObject Serialize() const override;
    void SetGravity(glm::vec3 const &);

    std::unique_ptr<btBroadphaseInterface> broadphase;
    std::unique_ptr<btCollisionConfiguration> collisionConfig;
    std::unique_ptr<btConstraintSolver> constraintSolver;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDiscreteDynamicsWorld> world;

private:
    static void s_TickCallback(btDynamicsWorld * world, btScalar timestep);

    std::unordered_map<Entity *, std::unordered_map<Entity *, CollisionInfo>> collisionsLastFrame;
};
