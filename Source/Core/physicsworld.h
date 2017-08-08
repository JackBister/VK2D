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

	std::unique_ptr<btDiscreteDynamicsWorld> world_;
	std::unique_ptr<btCollisionConfiguration> collision_config_;
	std::unique_ptr<btCollisionDispatcher> dispatcher_;
	std::unique_ptr<btBroadphaseInterface> broadphase_;
	std::unique_ptr<btConstraintSolver> constraint_solver_;

	std::unordered_map<Entity *, std::unordered_map<Entity *, CollisionInfo>> collisions_last_frame_;
};
