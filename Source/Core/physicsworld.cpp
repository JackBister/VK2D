#include "Core/physicsworld.h"

#include <unordered_map>
#include <vector>

#include "nlohmann/json.hpp"

#include "Core/entity.h"
#include "Core/Components/physicscomponent.h"

DESERIALIZABLE_IMPL(PhysicsWorld)

void PhysicsWorld::s_TickCallback(btDynamicsWorld * world, btScalar timestep)
{
	auto& collisionsLastFrame = static_cast<PhysicsWorld *>(world->getWorldUserInfo())->collisionsLastFrame;
	std::unordered_map<Entity *, std::unordered_map<Entity *, CollisionInfo>> collisionsThisFrame;
	int numManifolds = world->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; ++i) {
		btPersistentManifold * contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
		auto obA = static_cast<btCollisionObject const *>(contactManifold->getBody0());
		auto obB = static_cast<btCollisionObject const *>(contactManifold->getBody1());

		int numContacts = contactManifold->getNumContacts();
		bool collisionStart = true;
		//TODO: This is absolutely disgusting.
		std::vector<Vec3> pointsA(numContacts);
		std::vector<Vec3> pointsB(numContacts);
		std::vector<Vec3> normalsA(numContacts);
		std::vector<Vec3> normalsB(numContacts);

		for (int j = 0; j < numContacts; ++j) {
			btManifoldPoint& pt = contactManifold->getContactPoint(j);
			if (pt.getLifeTime() != 1) {
				collisionStart = false;
			}
			btVector3 const& ptA = pt.getPositionWorldOnA();
			btVector3 const& ptB = pt.getPositionWorldOnB();
			btVector3 const& normalOnA = pt.m_normalWorldOnB;
			btVector3 const& normalOnB = pt.m_normalWorldOnB;
			pointsA[j] = Vec3(ptA.x(), ptA.y(), ptA.z());
			pointsB[j] = Vec3(ptB.x(), ptB.y(), ptB.z());
			normalsA[j] = Vec3(normalOnA.x(), normalOnA.y(), normalOnA.z());
			normalsB[j] = Vec3(normalOnB.x(), normalOnB.y(), normalOnB.z());
		}

		auto compA = static_cast<PhysicsComponent *>(obA->getUserPointer());
		auto compB = static_cast<PhysicsComponent *>(obB->getUserPointer());
		auto entA = compA->entity;
		auto entB = compB->entity;

		{
			collisionsThisFrame[entA][entB].other = entB;
			//Only change collisionStart if it's true, so it can't flip back from false
			if (collisionsThisFrame[entA][entB].collisionStart) {
				collisionsThisFrame[entA][entB].collisionStart = collisionStart;
			}
			auto& aNorms = collisionsThisFrame[entA][entB].normals;
			aNorms.insert(aNorms.end(), normalsA.begin(), normalsA.end());
			auto& aPoints = collisionsThisFrame[entA][entB].points;
			aPoints.insert(aPoints.end(), pointsA.begin(), pointsA.end());
		}
		{
			collisionsThisFrame[entB][entA].other = entA;
			if (collisionsThisFrame[entB][entA].collisionStart) {
				collisionsThisFrame[entB][entA].collisionStart = collisionStart;
			}
			auto& bNorms = collisionsThisFrame[entB][entA].normals;
			bNorms.insert(bNorms.end(), normalsB.begin(), normalsB.end());
			auto& bPoints = collisionsThisFrame[entB][entA].points;
			bPoints.insert(bPoints.end(), pointsB.begin(), pointsB.end());
		}
	}
	for (auto& collisions : collisionsThisFrame) {
		for (auto& collisionInfo : collisions.second) {
			if (collisionInfo.second.collisionStart && collisionsLastFrame[collisions.first].find(collisionInfo.first) == collisionsLastFrame[collisions.first].end()) {
				collisions.first->FireEvent("OnCollisionStart", { { "info", &collisionInfo.second } });
			} else {
				collisionsLastFrame[collisions.first].erase(collisionsLastFrame[collisions.first].find(collisionInfo.first));
				collisions.first->FireEvent("OnCollision", { { "info", &collisionInfo.second } });
			}
		}
	}
	for (auto& collisions : collisionsLastFrame) {
		for (auto& collisionInfo : collisions.second) {
			collisions.first->FireEvent("OnCollisionEnd", { { "entity", collisionInfo.first } });
		}
	}

	collisionsLastFrame = collisionsThisFrame;
}

Deserializable * PhysicsWorld::Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(PhysicsWorld));
	PhysicsWorld * ret = new (mem) PhysicsWorld();
	auto const j = nlohmann::json::parse(str);
	ret->collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();

	ret->dispatcher = std::make_unique<btCollisionDispatcher>(ret->collisionConfig.get());

	ret->broadphase = std::make_unique<btDbvtBroadphase>();
	ret->constraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
	ret->world = std::make_unique<btDiscreteDynamicsWorld>(ret->dispatcher.get(), ret->broadphase.get(), ret->constraintSolver.get(), ret->collisionConfig.get());

	ret->world->setGravity(btVector3(j["gravity"]["x"], j["gravity"]["y"], j["gravity"]["z"]));
	ret->world->setInternalTickCallback(s_TickCallback);
	ret->world->setWorldUserInfo(ret);
	return ret;
}
