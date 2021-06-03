#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

#include "Core/collisioninfo.h"
#include "Serialization/Deserializable.h"

class PhysicsComponent;
class PhysicsWorldDeserializer;

class RaytestResult
{
public:
    class Hit
    {
        EntityPtr entity;
    };

    RaytestResult(std::optional<Hit> hit) : hit(hit) {}

    std::optional<Hit> const & GetClosestHit() const { return hit; }

private:
    std::optional<Hit> hit;
};

class PhysicsWorld
{
public:
    friend class PhysicsComponent;

    static PhysicsWorld * GetInstance();

    PhysicsWorld();

    glm::vec3 GetGravity() const;
    void SetGravity(glm::vec3 const &);

    void Tick(float dt);

private:
    static void s_TickCallback(btDynamicsWorld * world, btScalar timestep);

    std::unordered_map<EntityPtr, std::unordered_map<EntityPtr, CollisionInfo>> collisionsLastFrame;

    std::unique_ptr<btBroadphaseInterface> broadphase;
    std::unique_ptr<btCollisionConfiguration> collisionConfig;
    std::unique_ptr<btConstraintSolver> constraintSolver;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDiscreteDynamicsWorld> world;
    std::unique_ptr<btIDebugDraw> debugDraw;
};
