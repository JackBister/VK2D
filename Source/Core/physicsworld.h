#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "btBulletDynamicsCommon.h"

#include "Core/collisioninfo.h"
#include "Core/Deserializable.h"

struct Entity;

struct PhysicsWorld : Deserializable
{
	std::unique_ptr<btDiscreteDynamicsWorld> world;
	std::unique_ptr<btCollisionConfiguration> collisionConfig;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btConstraintSolver> constraintSolver;

	std::unordered_map<Entity *, std::unordered_map<Entity *, CollisionInfo>> collisionsLastFrame;

	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;
};
