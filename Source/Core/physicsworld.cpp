#include "Core/physicsworld.h"

#include <unordered_map>
#include <vector>

#include "json.hpp"

#include "Core/entity.h"
#include "Core/Components/physicscomponent.h"

DESERIALIZABLE_IMPL(PhysicsWorld)

void TickCallback(btDynamicsWorld * world, btScalar timestep)
{
	auto& collisionsLastFrame = static_cast<PhysicsWorld *>(world->getWorldUserInfo())->collisions_last_frame_;
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
		auto entA = compA->entity_;
		auto entB = compB->entity_;

		{
			collisionsThisFrame[entA][entB].other_ = entB;
			//Only change collisionStart if it's true, so it can't flip back from false
			if (collisionsThisFrame[entA][entB].collision_start_) {
				collisionsThisFrame[entA][entB].collision_start_ = collisionStart;
			}
			auto& aNorms = collisionsThisFrame[entA][entB].normals_;
			aNorms.insert(aNorms.end(), normalsA.begin(), normalsA.end());
			auto& aPoints = collisionsThisFrame[entA][entB].points_;
			aPoints.insert(aPoints.end(), pointsA.begin(), pointsA.end());
		}
		{
			collisionsThisFrame[entB][entA].other_ = entA;
			if (collisionsThisFrame[entB][entA].collision_start_) {
				collisionsThisFrame[entB][entA].collision_start_ = collisionStart;
			}
			auto& bNorms = collisionsThisFrame[entB][entA].normals_;
			bNorms.insert(bNorms.end(), normalsB.begin(), normalsB.end());
			auto& bPoints = collisionsThisFrame[entB][entA].points_;
			bPoints.insert(bPoints.end(), pointsB.begin(), pointsB.end());
		}
	}
	for (auto& collisions : collisionsThisFrame) {
		for (auto& collisionInfo : collisions.second) {
			if (collisionInfo.second.collision_start_ && collisionsLastFrame[collisions.first].find(collisionInfo.first) == collisionsLastFrame[collisions.first].end()) {
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
	ret->collision_config_ = std::make_unique<btDefaultCollisionConfiguration>();

	ret->dispatcher_ = std::make_unique<btCollisionDispatcher>(ret->collision_config_.get());

	ret->broadphase_ = std::make_unique<btDbvtBroadphase>();
	ret->constraint_solver_ = std::make_unique<btSequentialImpulseConstraintSolver>();
	ret->world_ = std::make_unique<btDiscreteDynamicsWorld>(ret->dispatcher_.get(), ret->broadphase_.get(), ret->constraint_solver_.get(), ret->collision_config_.get());

	ret->world_->setGravity(btVector3(j["gravity"]["x"], j["gravity"]["y"], j["gravity"]["z"]));
	ret->world_->setInternalTickCallback(TickCallback);
	ret->world_->setWorldUserInfo(ret);
	return ret;
}
