#pragma once

#include <memory>
#include <string>

#include "btBulletDynamicsCommon.h"

struct PhysicsWorld
{
	std::unique_ptr<btDiscreteDynamicsWorld> world;
	std::unique_ptr<btCollisionConfiguration> collisionConfig;
	std::unique_ptr<btCollisionDispatcher> dispatcher;
	std::unique_ptr<btBroadphaseInterface> broadphase;
	std::unique_ptr<btConstraintSolver> constraintSolver;

	/*
	btDiscreteDynamicsWorld * world;
	btCollisionConfiguration * collisionConfig;
	btCollisionDispatcher * dispatcher;
	btBroadphaseInterface * broadphase;
	btConstraintSolver * constraintSolver;
	*/
	static PhysicsWorld Deserialize(std::string);
};
