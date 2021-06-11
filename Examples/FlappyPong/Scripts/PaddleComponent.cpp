#include "PaddleComponent.h"

#include "Core/Entity.h"
#include "Core/Input/Input.h"

REFLECT_STRUCT_BEGIN(PaddleComponent)
REFLECT_STRUCT_MEMBER(flapSpeed)
REFLECT_STRUCT_MEMBER(gravity)
REFLECT_STRUCT_MEMBER(isColliding)
REFLECT_STRUCT_MEMBER(velocityY)
REFLECT_STRUCT_END()

static SerializedObjectSchema const PADDLE_COMPONENT_SCHEMA =
    SerializedObjectSchema("PaddleComponent",
                           {
                               SerializedPropertySchema("flapSpeed", SerializedValueType::DOUBLE),
                               SerializedPropertySchema("gravity", SerializedValueType::DOUBLE),
                           },
                           {SerializedObjectFlag::IS_COMPONENT});

class PaddleComponentDeserializer : public Deserializer
{
    std::string constexpr GetOwner() final override { return "FlappyPong"; }

    SerializedObjectSchema GetSchema() final override { return PADDLE_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto ret = new PaddleComponent();
        ret->flapSpeed = obj.GetNumber("flapSpeed").value_or(40.f);
        ret->gravity = obj.GetNumber("gravity").value_or(50.f);
        return ret;
    }
};

COMPONENT_IMPL(PaddleComponent, new PaddleComponentDeserializer())

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
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            return;
        }
        auto position = e->GetTransform()->GetPosition();
        auto scale = e->GetTransform()->GetScale();
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
        e->GetTransform()->SetPosition(position);
    } else if (name == "OnCollisionStart") {
        isColliding = true;
    } else if (name == "OnCollisionEnd") {
        isColliding = false;
    }
}
