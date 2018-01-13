#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "btBulletDynamicsCommon.h"

#include "Core/collisioninfo.h"
#include "Core/Deserializable.h"

class Entity;

class PhysicsWorld : public Deserializable
{
public:
	Deserializable * Deserialize(ResourceManager *, std::string const& str, Allocator& alloc = Allocator::default_allocator) const override;
	std::string Serialize() const override;

	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btCollisionConfiguration> collisionConfig;
	std::unique_ptr<btConstraintSolver> constraintSolver;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btDiscreteDynamicsWorld> world;
private:
	static void s_TickCallback(btDynamicsWorld * world, btScalar timestep);

	std::unordered_map<Entity *, std::unordered_map<Entity *, CollisionInfo>> collisionsLastFrame;
};
