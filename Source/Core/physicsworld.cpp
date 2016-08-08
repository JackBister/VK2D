#include "physicsworld.h"

#include "json.hpp"

PhysicsWorld PhysicsWorld::Deserialize(std::string s)
{
	nlohmann::json j = nlohmann::json::parse(s);
	PhysicsWorld ret;
	ret.collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
	ret.dispatcher = std::make_unique<btCollisionDispatcher>(ret.collisionConfig.get());
	ret.broadphase = std::make_unique<btDbvtBroadphase>();
	ret.constraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
	ret.world = std::make_unique<btDiscreteDynamicsWorld>(ret.dispatcher.get(), ret.broadphase.get(), ret.constraintSolver.get(), ret.collisionConfig.get());

	/*
	ret.collisionConfig = new btDefaultCollisionConfiguration();
	ret.dispatcher = new btCollisionDispatcher(ret.collisionConfig);
	ret.broadphase = new btDbvtBroadphase();
	ret.constraintSolver = new btSequentialImpulseConstraintSolver();
	ret.world = new btDiscreteDynamicsWorld(ret.dispatcher, ret.broadphase, ret.constraintSolver, ret.collisionConfig);
	*/
	ret.world->setGravity(btVector3(j["gravity"]["x"], j["gravity"]["y"], j["gravity"]["z"]));
	return ret;
}
