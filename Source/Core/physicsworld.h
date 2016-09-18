#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "btBulletDynamicsCommon.h"

#include "collisioninfo.h"

struct Entity;

struct PhysicsWorld
{
	std::unique_ptr<btDiscreteDynamicsWorld> world;
	std::unique_ptr<btCollisionConfiguration> collisionConfig;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btConstraintSolver> constraintSolver;

	std::unordered_map<Entity *, std::unordered_map<Entity *, CollisionInfo>> collisionsLastFrame;

	static PhysicsWorld * Deserialize(std::string);
};
