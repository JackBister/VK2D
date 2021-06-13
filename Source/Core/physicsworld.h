#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include <ThirdParty/bullet3/src/btBulletDynamicsCommon.h>
#include <ThirdParty/glm/glm/glm.hpp>

#include "Core/collisioninfo.h"
#include "Util/Line.h"

class DebugDrawSystem;
class PhysicsComponent;

class RaytestResult
{
public:
    struct Hit {
        Hit(EntityPtr entity, glm::vec3 collisionPoint) : entity(entity), collisionPoint(collisionPoint) {}

        EntityPtr const entity;
        glm::vec3 const collisionPoint;
    };

    RaytestResult(std::optional<Hit> hit) : hit(hit) {}

    std::optional<Hit> const & GetClosestHit() const { return hit; }

private:
    std::optional<Hit> hit;
};

class EAPI PhysicsWorld
{
public:
    friend class PhysicsComponent;

    static PhysicsWorld * GetInstance();

    PhysicsWorld(DebugDrawSystem * debugDrawSystem);

    glm::vec3 GetGravity() const;
    void SetGravity(glm::vec3 const &);

    bool GetDebugDrawEnabled() const { return isDebugDrawEnabled; }
    void SetDebugDrawEnabled(bool enabled);

    RaytestResult Raytest(Line line);

    void Tick(float dt);

private:
    static void s_TickCallback(btDynamicsWorld * world, btScalar timestep);

    DebugDrawSystem * debugDrawSystem;

    std::unordered_map<EntityPtr, std::unordered_map<EntityPtr, CollisionInfo>> collisionsLastFrame;

    std::unique_ptr<btBroadphaseInterface> broadphase;
    std::unique_ptr<btCollisionConfiguration> collisionConfig;
    std::unique_ptr<btConstraintSolver> constraintSolver;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDiscreteDynamicsWorld> world;
    std::unique_ptr<btIDebugDraw> debugDraw;

    bool isDebugDrawEnabled = false;
};
