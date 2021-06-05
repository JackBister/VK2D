#include "Core/physicsworld.h"

#include <unordered_map>
#include <vector>

#include "Console/Console.h"
#include "Core/Components/physicscomponent.h"
#include "Core/Rendering/DebugDrawSystem.h"
#include "Core/entity.h"
#include "Logging/Logger.h"
#include "Util/Strings.h"

static const auto logger = Logger::Create("PhysicsWorld");

class DebugDrawBulletAdapter : public btIDebugDraw
{
public:
    DebugDrawBulletAdapter(DebugDrawSystem * debugDrawSystem) : debugDrawSystem(debugDrawSystem) {}

    virtual void drawLine(const btVector3 & from, const btVector3 & to, const btVector3 & color) override
    {
        debugDrawSystem->DrawLine(glm::vec3(from.x(), from.y(), from.z()),
                                  glm::vec3(to.x(), to.y(), to.z()),
                                  glm::vec3(color.x(), color.y(), color.z()),
                                  0.f);
    }

    virtual void drawContactPoint(const btVector3 & PointOnB, const btVector3 & normalOnB, btScalar distance,
                                  int lifeTime, const btVector3 & color) override
    {
        logger.Trace("STUB: DebugDrawBulletAdapter::drawContactPoint");
    }

    virtual void reportErrorWarning(const char * warningString) override
    {
        logger.Warn("Warning from Bullet debug draw: {}", warningString);
    }

    virtual void draw3dText(const btVector3 & location, const char * textString) override
    {
        logger.Trace("STUB: DebugDrawBulletAdapter::draw3dText");
    }

    virtual void setDebugMode(int newDebugMode) override { debugMode = newDebugMode; }

    virtual int getDebugMode() const override { return debugMode; }

private:
    DebugDrawSystem * debugDrawSystem;
    int debugMode = DBG_NoDebug;
};

// TODO: Move this somewhere else
static SerializedObjectSchema const VEC2_SCHEMA =
    SerializedObjectSchema("Vec2", {SerializedPropertySchema::Required("x", SerializedValueType::DOUBLE),
                                    SerializedPropertySchema::Required("y", SerializedValueType::DOUBLE)});

class Vec2Deserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return VEC2_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj)
    {
        auto ret = new glm::vec2();
        ret->x = obj.GetNumber("x").value();
        ret->y = obj.GetNumber("y").value();
        return ret;
    }
};

DESERIALIZABLE_IMPL(Vec2, new Vec2Deserializer());

static SerializedObjectSchema const VEC3_SCHEMA =
    SerializedObjectSchema("Vec3", {SerializedPropertySchema::Required("x", SerializedValueType::DOUBLE),
                                    SerializedPropertySchema::Required("y", SerializedValueType::DOUBLE),
                                    SerializedPropertySchema::Required("z", SerializedValueType::DOUBLE)});

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
    static PhysicsWorld singletonPhysicsWorld(DebugDrawSystem::GetInstance());
    return &singletonPhysicsWorld;
}

PhysicsWorld::PhysicsWorld(DebugDrawSystem * debugDrawSystem)
    : debugDrawSystem(debugDrawSystem), debugDraw(new DebugDrawBulletAdapter(debugDrawSystem))
{
    this->collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    this->dispatcher = std::make_unique<btCollisionDispatcher>(this->collisionConfig.get());
    this->broadphase = std::make_unique<btDbvtBroadphase>();
    this->constraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    this->world = std::make_unique<btDiscreteDynamicsWorld>(
        this->dispatcher.get(), this->broadphase.get(), this->constraintSolver.get(), this->collisionConfig.get());
    this->world->setInternalTickCallback(PhysicsWorld::s_TickCallback);
    this->world->setWorldUserInfo(this);
    this->world->setDebugDrawer(debugDraw.get());

    CommandDefinition debugPhysicsCommand("debug_physics",
                                          "debug_physics <0 or 1> - enables or disables drawing physics wireframes",
                                          1,
                                          [this](auto args) {
                                              auto arg = args[0];
                                              if (arg == "0") {
                                                  this->SetDebugDrawEnabled(false);
                                              } else if (arg == "1") {
                                                  this->SetDebugDrawEnabled(true);
                                              }
                                          });
    Console::RegisterCommand(debugPhysicsCommand);

    CommandDefinition raytestCommand(
        "physics_raytest",
        "physics_raytest x y z x2 y2 z2 - casts a ray from (x, y, z) to (x2, y2, z2) in "
        "the physics world and logs the result.",
        6,
        [this](auto args) {
            auto xs = args[0];
            auto ys = args[1];
            auto zs = args[2];
            auto x2s = args[3];
            auto y2s = args[4];
            auto z2s = args[5];

            auto x = Strings::Strtod(xs);
            auto y = Strings::Strtod(ys);
            auto z = Strings::Strtod(zs);
            auto x2 = Strings::Strtod(x2s);
            auto y2 = Strings::Strtod(y2s);
            auto z2 = Strings::Strtod(z2s);
            if (!x.has_value() || !y.has_value() || !z.has_value() || !x2.has_value() || !y2.has_value() ||
                !z2.has_value()) {
                logger.Warn("Could not perform raytest: input was invalid: one of the values was not a number");
                return;
            }
            btVector3 from(x.value(), y.value(), z.value());
            btVector3 to(x2.value(), y2.value(), z2.value());
            btCollisionWorld::AllHitsRayResultCallback raytestCallback(from, to);
            logger.Info("Performing ray test with from=({}, {}, {}), to=({}, {}, {})",
                        from.x(),
                        from.y(),
                        from.z(),
                        to.x(),
                        to.y(),
                        to.z());
            world->rayTest(from, to, raytestCallback);
            if (raytestCallback.m_collisionObject) {
                auto userPointer = (PhysicsComponent *)raytestCallback.m_collisionObject->getUserPointer();
                if (userPointer && userPointer->entity) {
                    auto entity = userPointer->entity.Get();
                    logger.Info("Ray hit entity with id={}, name={}", entity->GetId().ToString(), entity->GetName());
                }
            } else {
                logger.Info("Ray did not hit anything");
            }
        });
    Console::RegisterCommand(raytestCommand);
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
            logger.Warn("Got null entity entityPtr={}", collisions.first.ToString());
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
            logger.Warn("Got null entity for entityPtr={}", collisions.first.ToString());
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

void PhysicsWorld::SetDebugDrawEnabled(bool enabled)
{
    isDebugDrawEnabled = enabled;
    if (enabled) {
        this->debugDraw->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawAabb |
                                      btIDebugDraw::DBG_DrawNormals | btIDebugDraw::DBG_DrawConstraints |
                                      btIDebugDraw::DBG_DrawConstraintLimits);
    } else {
        this->debugDraw->setDebugMode(btIDebugDraw::DBG_NoDebug);
    }
}

RaytestResult PhysicsWorld::Raytest(Line line)
{
    btVector3 worldFrom(line.from.x, line.from.y, line.from.z);
    btVector3 worldTo(line.to.x, line.to.y, line.to.z);
    if (isDebugDrawEnabled) {
        debugDrawSystem->DrawPoint(line.from, glm::vec3(0.f, 1.f, 0.f), 5.f);
        debugDrawSystem->DrawPoint(line.to, glm::vec3(0.f, 0.f, 1.f), 5.f);
        debugDrawSystem->DrawLine(line.from, line.to, glm::vec3(1.f, 1.f, 1.f), 5.f);
    }
    btDynamicsWorld::ClosestRayResultCallback callback(worldFrom, worldTo);
    world->rayTest(worldFrom, worldTo, callback);
    if (!callback.hasHit()) {
        return RaytestResult(std::nullopt);
    }
    if (!callback.m_collisionObject) {
        logger.Warn("When raytesting: callback.hasHit() but m_collisionObject was null. Will treat it as a non-hit.");
        return RaytestResult(std::nullopt);
    }
    if (!callback.m_collisionObject->getUserPointer()) {
        logger.Warn("When raytesting: callback.hasHit() but user pointer was null. Will treat it as a non-hit.");
        return RaytestResult(std::nullopt);
    }
    PhysicsComponent * hitComponent = (PhysicsComponent *)callback.m_collisionObject->getUserPointer();
    auto hitFraction = callback.m_closestHitFraction;
    auto hitPoint = (line.from + line.to) * hitFraction;
    if (isDebugDrawEnabled) {
        debugDrawSystem->DrawPoint(hitPoint, glm::vec3(1.f, 0.f, 0.f), 5.f);
    }
    return RaytestResult(RaytestResult::Hit(hitComponent->entity, hitPoint));
}

void PhysicsWorld::Tick(float dt)
{
    world->stepSimulation(dt);
    world->debugDrawWorld();
}
