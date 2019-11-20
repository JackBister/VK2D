#include "PaddleComponent.h"

#include "Core/Entity.h"
#include "Core/GameModule.h"
#include "Core/Input.h"

COMPONENT_IMPL(PaddleComponent, &PaddleComponent::s_Deserialize)

REFLECT_STRUCT_BEGIN(PaddleComponent)
REFLECT_STRUCT_MEMBER(flapSpeed)
REFLECT_STRUCT_MEMBER(gravity)
REFLECT_STRUCT_MEMBER(isColliding)
REFLECT_STRUCT_MEMBER(velocityY)
REFLECT_STRUCT_END()

Deserializable * PaddleComponent::s_Deserialize(DeserializationContext * deserializationContext,
                                                SerializedObject const & obj)
{
    auto ret = new PaddleComponent();
    ret->flapSpeed = obj.GetNumber("flapSpeed").value_or(40.f);
    ret->gravity = obj.GetNumber("gravity").value_or(50.f);
    return ret;
}

SerializedObject PaddleComponent::Serialize() const
{
    return SerializedObject::Builder()
        .WithString("type", this->Reflection.name)
        .WithNumber("flapSpeed", this->flapSpeed)
        .WithNumber("gravity", this->gravity)
        .Build();
}

void PaddleComponent::OnEvent(HashedString name, EventArgs args)
{
    if (name == "Tick") {
        auto position = entity->transform.GetPosition();
        auto scale = entity->transform.GetScale();
        if (Input::GetButtonDown("Flap")) {
            if (velocityY < 0.f) {
                velocityY = 0.f;
            }
            velocityY += flapSpeed;
        }
        if (position.y <= -60.f + scale.y && velocityY <= 0.f) {
            return;
        }
        if (position.y >= 60.f - scale.y && velocityY >= 0.f) {
            velocityY = 0.f;
        }
        if (!isColliding) {
            position.y += velocityY * args["deltaTime"].asFloat;
            velocityY = velocityY - gravity * args["deltaTime"].asFloat;
        }
        entity->transform.SetPosition(position);
    } else if (name == "OnCollisionStart") {
        isColliding = true;
    } else if (name == "OnCollisionEnd") {
        isColliding = false;
    }
}
