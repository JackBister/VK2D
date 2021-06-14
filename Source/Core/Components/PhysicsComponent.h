#pragma once
#include <vector>

#include "Component.h"
#include "Core/RigidbodyInstance.h"
#include "Serialization/DeserializationContext.h"
#include "Serialization/SerializedValue.h"
#include "Util/DllExport.h"

class PhysicsWorld;

class EAPI PhysicsComponent : public Component
{
public:
    PhysicsComponent(PhysicsWorld * physicsWorld, DeserializationContext deserializationContext,
                     SerializedObject serializedRigidbody)
        : physicsWorld(physicsWorld), deserializationContext(deserializationContext),
          serializedRigidbody(serializedRigidbody)
    {
        receiveTicks = false;
        type = "PhysicsComponent";
    }
    ~PhysicsComponent() override;

    SerializedObject Serialize() const override;

    void OnEvent(HashedString name, EventArgs args = {}) override;

    REFLECT()
    REFLECT_INHERITANCE()
private:
    PhysicsWorld * physicsWorld;
    DeserializationContext deserializationContext;
    SerializedObject serializedRigidbody;
    std::optional<RigidbodyInstanceId> rigidbodyInstanceId;
};
