#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include <ThirdParty/glm/glm/glm.hpp>

#include "Core/collisioninfo.h"
#include "RigidbodyInstance.h"
#include "Serialization/SerializedValue.h"
#include "Util/Line.h"

class DebugDrawSystem;
class DeserializationContext;
class PhysicsComponent;

class RaytestResult
{
public:
    struct Hit {
        Hit(EntityPtr entity, glm::vec3 collisionPoint) : entity(entity), collisionPoint(collisionPoint) {}

        EntityPtr const entity;
        glm::vec3 const collisionPoint;
    };

    RaytestResult(std::optional<Hit> hit) : hit(hit) {}

    std::optional<Hit> const & GetClosestHit() const { return hit; }

private:
    std::optional<Hit> hit;
};

class EAPI PhysicsWorld
{
public:
    friend class PhysicsComponent;

    static PhysicsWorld * GetInstance();

    PhysicsWorld(DebugDrawSystem * debugDrawSystem);
    ~PhysicsWorld();

    glm::vec3 GetGravity() const;
    void SetGravity(glm::vec3 const &);

    bool GetDebugDrawEnabled() const { return isDebugDrawEnabled; }
    void SetDebugDrawEnabled(bool enabled);

    std::optional<RigidbodyInstanceId> AddRigidbody(PhysicsComponent * component, DeserializationContext * ctx,
                                                    SerializedObject obj);
    void RemoveRigidbody(RigidbodyInstanceId);

    RaytestResult Raytest(Line line);

    void Tick(float dt);

private:
    class Pimpl;

    std::unique_ptr<Pimpl> pimpl;

    DebugDrawSystem * debugDrawSystem;

    bool isDebugDrawEnabled = false;
};
