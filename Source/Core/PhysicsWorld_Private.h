#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>

#include "physicsworld.h"

#include <ThirdParty/bullet3/src/btBulletDynamicsCommon.h>

class MotionStateAdapter : public btMotionState
{
public:
    MotionStateAdapter(RigidbodyInstanceId id, EntityPtr entity) : id(id), entity(entity) {}

    RigidbodyInstanceId const id;
    EntityPtr const entity;

    /*
     * Set the transform of the physics object in the "bullet world".
     * Called by bullet automatically before stepping the simulation
     */
    void getWorldTransform(btTransform &) const override;
    /*
     * Sets the transform of the entity in the rendered world.
     */
    void setWorldTransform(const btTransform &) override;
};

class RigidbodyInstance
{
public:
    RigidbodyInstance() = default;
    RigidbodyInstance(RigidbodyInstanceId id, std::unique_ptr<MotionStateAdapter> && motionState,
                      std::unique_ptr<btRigidBody> && rigidBody, std::unique_ptr<btCollisionShape> && shape,
                      BroadphaseNativeTypes shapeType, bool isKinematic, float mass) noexcept
        : id(id), motionState(std::move(motionState)), rigidBody(std::move(rigidBody)), shape(std::move(shape)),
          shapeType(shapeType), isKinematic(isKinematic), mass(mass)
    {
    }

    RigidbodyInstanceId id;
    std::unique_ptr<MotionStateAdapter> motionState;
    std::unique_ptr<btRigidBody> rigidBody;
    std::unique_ptr<btCollisionShape> shape;
    BroadphaseNativeTypes shapeType;
    bool isKinematic;
    float mass;
};

class PhysicsWorld::Pimpl
{
public:
    Pimpl(DebugDrawSystem * debugDrawSystem);

    static void s_TickCallback(btDynamicsWorld * world, btScalar timestep);

    std::unordered_map<EntityPtr, std::unordered_map<EntityPtr, CollisionInfo>> collisionsLastFrame;

    std::unique_ptr<btBroadphaseInterface> broadphase;
    std::unique_ptr<btCollisionConfiguration> collisionConfig;
    std::unique_ptr<btConstraintSolver> constraintSolver;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDiscreteDynamicsWorld> world;
    std::unique_ptr<btIDebugDraw> debugDraw;

    std::unordered_map<RigidbodyInstanceId, RigidbodyInstance> rigidbodyInstances;
    std::atomic_uint64_t currentRigidbodyId{0};
};
