#include "Core/physicsworld.h"

#include <unordered_map>
#include <vector>

#include "Core/Components/physicscomponent.h"
#include "Core/entity.h"
#include "Logging/Logger.h"

static const auto logger = Logger::Create("PhysicsWorld");

static SerializedObjectSchema const VEC3_SCHEMA =
    SerializedObjectSchema("Vec3", {
                                       SerializedPropertySchema("x", SerializedValueType::DOUBLE, {}, "", true),
                                       SerializedPropertySchema("y", SerializedValueType::DOUBLE, {}, "", true),
                                       SerializedPropertySchema("z", SerializedValueType::DOUBLE, {}, "", true),
                                   });

// TODO: Move this somewhere else
class Vec3Deserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return VEC3_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj)
    {
        auto ret = new glm::vec3();
        ret->x = obj.GetNumber("x").value();
        ret->y = obj.GetNumber("y").value();
        ret->z = obj.GetNumber("z").value();
        return ret;
    }
};

DESERIALIZABLE_IMPL(Vec3, new Vec3Deserializer());

PhysicsWorld * PhysicsWorld::GetInstance()
{
    static PhysicsWorld singletonPhysicsWorld;
    return &singletonPhysicsWorld;
}

PhysicsWorld::PhysicsWorld()
{
    this->collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    this->dispatcher = std::make_unique<btCollisionDispatcher>(this->collisionConfig.get());
    this->broadphase = std::make_unique<btDbvtBroadphase>();
    this->constraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    this->world = std::make_unique<btDiscreteDynamicsWorld>(
        this->dispatcher.get(), this->broadphase.get(), this->constraintSolver.get(), this->collisionConfig.get());
    this->world->setInternalTickCallback(PhysicsWorld::s_TickCallback);
    this->world->setWorldUserInfo(this);
}

void PhysicsWorld::s_TickCallback(btDynamicsWorld * world, btScalar timestep)
{
    auto & collisionsLastFrame = static_cast<PhysicsWorld *>(world->getWorldUserInfo())->collisionsLastFrame;
    std::unordered_map<EntityPtr, std::unordered_map<EntityPtr, CollisionInfo>> collisionsThisFrame;
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
        auto firstEntity = collisions.first.Get();
        if (!firstEntity) {
            logger->Warnf("Got null entity entityPtr=%s", collisions.first.ToString().c_str());
            continue;
        }
        for (auto & collisionInfo : collisions.second) {
            if (collisionInfo.second.collisionStart &&
                collisionsLastFrame[collisions.first].find(collisionInfo.first) ==
                    collisionsLastFrame[collisions.first].end()) {
                firstEntity->FireEvent("OnCollisionStart", {{"info", &collisionInfo.second}});

            } else {
                collisionsLastFrame[collisions.first].erase(
                    collisionsLastFrame[collisions.first].find(collisionInfo.first));
                firstEntity->FireEvent("OnCollision", {{"info", &collisionInfo.second}});
            }
        }
    }
    for (auto & collisions : collisionsLastFrame) {
        auto e = collisions.first.Get();
        if (!e) {
            logger->Warnf("Got null entity for entityPtr=%s", collisions.first.ToString().c_str());
            continue;
        }
        for (auto & collisionInfo : collisions.second) {
            e->FireEvent("OnCollisionEnd", {{"entity", collisionInfo.first}});
        }
    }

    collisionsLastFrame = collisionsThisFrame;
}

glm::vec3 PhysicsWorld::GetGravity() const
{
    auto grav = world->getGravity();
    return glm::vec3(grav.x(), grav.y(), grav.z());
}

void PhysicsWorld::SetGravity(glm::vec3 const & grav)
{
    world->setGravity(btVector3(grav.x, grav.y, grav.z));
}

void PhysicsWorld::Tick(float dt)
{
    world->stepSimulation(dt);
}
