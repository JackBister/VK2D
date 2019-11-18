#include "Core/physicsworld.h"

#include <unordered_map>
#include <vector>

#include "nlohmann/json.hpp"

#include "Core/Components/physicscomponent.h"
#include "Core/Logging/Logger.h"
#include "Core/entity.h"

static const auto logger = Logger::Create("PhysicsWorld");

DESERIALIZABLE_IMPL(PhysicsWorld, &PhysicsWorld::s_Deserialize)

void PhysicsWorld::s_TickCallback(btDynamicsWorld * world, btScalar timestep)
{
    auto & collisionsLastFrame = static_cast<PhysicsWorld *>(world->getWorldUserInfo())->collisionsLastFrame;
    std::unordered_map<Entity *, std::unordered_map<Entity *, CollisionInfo>> collisionsThisFrame;
    int numManifolds = world->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; ++i) {
        btPersistentManifold * contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
        auto obA = static_cast<btCollisionObject const *>(contactManifold->getBody0());
        auto obB = static_cast<btCollisionObject const *>(contactManifold->getBody1());

        int numContacts = contactManifold->getNumContacts();
        bool collisionStart = true;
        // TODO: This is absolutely disgusting.
        std::vector<glm::vec3> pointsA(numContacts);
        std::vector<glm::vec3> pointsB(numContacts);
        std::vector<glm::vec3> normalsA(numContacts);
        std::vector<glm::vec3> normalsB(numContacts);

        for (int j = 0; j < numContacts; ++j) {
            btManifoldPoint & pt = contactManifold->getContactPoint(j);
            if (pt.getLifeTime() != 1) {
                collisionStart = false;
            }
            btVector3 const & ptA = pt.getPositionWorldOnA();
            btVector3 const & ptB = pt.getPositionWorldOnB();
            btVector3 const & normalOnA = pt.m_normalWorldOnB;
            btVector3 const & normalOnB = pt.m_normalWorldOnB;
            pointsA[j] = glm::vec3(ptA.x(), ptA.y(), ptA.z());
            pointsB[j] = glm::vec3(ptB.x(), ptB.y(), ptB.z());
            normalsA[j] = glm::vec3(normalOnA.x(), normalOnA.y(), normalOnA.z());
            normalsB[j] = glm::vec3(normalOnB.x(), normalOnB.y(), normalOnB.z());
        }

        auto compA = static_cast<PhysicsComponent *>(obA->getUserPointer());
        auto compB = static_cast<PhysicsComponent *>(obB->getUserPointer());
        auto entA = compA->entity;
        auto entB = compB->entity;

        {
            collisionsThisFrame[entA][entB].other = entB;
            // Only change collisionStart if it's true, so it can't flip back from false
            if (collisionsThisFrame[entA][entB].collisionStart) {
                collisionsThisFrame[entA][entB].collisionStart = collisionStart;
            }
            auto & aNorms = collisionsThisFrame[entA][entB].normals;
            aNorms.insert(aNorms.end(), normalsA.begin(), normalsA.end());
            auto & aPoints = collisionsThisFrame[entA][entB].points;
            aPoints.insert(aPoints.end(), pointsA.begin(), pointsA.end());
        }
        {
            collisionsThisFrame[entB][entA].other = entA;
            if (collisionsThisFrame[entB][entA].collisionStart) {
                collisionsThisFrame[entB][entA].collisionStart = collisionStart;
            }
            auto & bNorms = collisionsThisFrame[entB][entA].normals;
            bNorms.insert(bNorms.end(), normalsB.begin(), normalsB.end());
            auto & bPoints = collisionsThisFrame[entB][entA].points;
            bPoints.insert(bPoints.end(), pointsB.begin(), pointsB.end());
        }
    }
    for (auto & collisions : collisionsThisFrame) {
        for (auto & collisionInfo : collisions.second) {
            if (collisionInfo.second.collisionStart &&
                collisionsLastFrame[collisions.first].find(collisionInfo.first) ==
                    collisionsLastFrame[collisions.first].end()) {
                collisions.first->FireEvent("OnCollisionStart", {{"info", &collisionInfo.second}});
            } else {
                collisionsLastFrame[collisions.first].erase(
                    collisionsLastFrame[collisions.first].find(collisionInfo.first));
                collisions.first->FireEvent("OnCollision", {{"info", &collisionInfo.second}});
            }
        }
    }
    for (auto & collisions : collisionsLastFrame) {
        for (auto & collisionInfo : collisions.second) {
            collisions.first->FireEvent("OnCollisionEnd", {{"entity", collisionInfo.first}});
        }
    }

    collisionsLastFrame = collisionsThisFrame;
}

Deserializable * PhysicsWorld::s_Deserialize(std::string const & str)
{
    PhysicsWorld * ret = new PhysicsWorld();
    auto const j = nlohmann::json::parse(str);
    ret->collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();

    ret->dispatcher = std::make_unique<btCollisionDispatcher>(ret->collisionConfig.get());

    ret->broadphase = std::make_unique<btDbvtBroadphase>();
    ret->constraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    ret->world = std::make_unique<btDiscreteDynamicsWorld>(
        ret->dispatcher.get(), ret->broadphase.get(), ret->constraintSolver.get(), ret->collisionConfig.get());

    ret->world->setGravity(btVector3(j["gravity"]["x"], j["gravity"]["y"], j["gravity"]["z"]));
    ret->world->setInternalTickCallback(s_TickCallback);
    ret->world->setWorldUserInfo(ret);
    return ret;
}

std::string PhysicsWorld::Serialize() const
{
    nlohmann::json j;
    j["type"] = this->type;
    auto grav = world->getGravity();
    j["gravity"]["x"] = grav.getX();
    j["gravity"]["y"] = grav.getY();
    j["gravity"]["z"] = grav.getZ();

    return j.dump();
}

void PhysicsWorld::SetGravity(glm::vec3 const & grav)
{
    world->setGravity(btVector3(grav.x, grav.y, grav.z));
}
