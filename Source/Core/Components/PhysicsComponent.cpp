#include "PhysicsComponent.h"

#include <ThirdParty/bullet3/src/BulletCollision/CollisionShapes/btBox2dShape.h>
#include <ThirdParty/optick/src/optick.h>

#include "Core/entity.h"
#include "Core/physicsworld.h"
#include "Logging/Logger.h"

static const auto logger = Logger::Create("PhysicsComponent");

REFLECT_STRUCT_BEGIN(PhysicsComponent)
REFLECT_STRUCT_END()

static SerializedObjectSchema const PHYSICS_COMPONENT_SCHEMA =
    SerializedObjectSchema("PhysicsComponent", {SerializedPropertySchema::RequiredObject("rigidbody", "Rigidbody")});

class PhysicsComponentDeserializer : public Deserializer
{
    SerializedObjectSchema GetSchema() final override { return PHYSICS_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        if (!obj.GetObject("rigidbody").has_value()) {
            logger.Error("Failed to deserialize PhysicsComponent: rigidbody property was not present");
            return nullptr;
        }
        return new PhysicsComponent(PhysicsWorld::GetInstance(), *ctx, obj.GetObject("rigidbody").value());
    }
};

COMPONENT_IMPL(PhysicsComponent, new PhysicsComponentDeserializer())

PhysicsComponent::~PhysicsComponent()
{
    if (rigidbodyInstanceId.has_value()) {
        physicsWorld->RemoveRigidbody(rigidbodyInstanceId.value());
    }
}

SerializedObject PhysicsComponent::Serialize() const
{
    return SerializedObject::Builder()
        .WithString("type", this->Reflection.name)
        .WithObject("rigidbody", serializedRigidbody)
        .Build();
}

void PhysicsComponent::OnEvent(HashedString name, EventArgs args)
{
    OPTICK_EVENT();
#if _DEBUG
    OPTICK_TAG("EventName", name.c_str());
#endif
    if (name == "BeginPlay") {
        logger.Info("PhysicsComponent::BeginPlay, entity={}", entity.Get()->GetName());
        rigidbodyInstanceId = physicsWorld->AddRigidbody(this, &deserializationContext, serializedRigidbody);
    }
}
