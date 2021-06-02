#pragma once
#include <vector>

#include "btBulletDynamicsCommon.h"

#include "Core/Components/Component.h"

class PhysicsComponentDeserializer;

class PhysicsComponent : public Component, public btMotionState
{
public:
    friend class PhysicsComponentDeserializer;

    PhysicsComponent(bool isKinematic, float mass, std::unique_ptr<btCollisionShape> && shape,
                     BroadphaseNativeTypes shapeType)
        : isKinematic(isKinematic), mass(mass), shape(std::move(shape)), shapeType(shapeType)
    {
        receiveTicks = false;
        type = "PhysicsComponent";
    }
    ~PhysicsComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args = {}) override;

    /*
     * Set the transform of the physics object in the "bullet world".
     * Called by bullet automatically before stepping the simulation
     */
    void getWorldTransform(btTransform &) const override;
    /*
     * Sets the transform of the entity in the rendered world.
     */
    void setWorldTransform(const btTransform &) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    bool isKinematic = false;
    float mass;
    std::unique_ptr<btRigidBody> rigidBody;
    std::unique_ptr<btCollisionShape> shape;
    BroadphaseNativeTypes shapeType;
};
