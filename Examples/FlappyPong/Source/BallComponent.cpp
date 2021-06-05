#include "BallComponent.h"

#include <cstdlib>

#include "Core/CollisionInfo.h"
#include "Core/Entity.h"
#include "Logging/Logger.h"

static const auto logger = Logger::Create("BallComponent");

REFLECT_STRUCT_BEGIN(BallComponent)
REFLECT_STRUCT_MEMBER(velocityDir)
REFLECT_STRUCT_MEMBER(moveSpeed)
REFLECT_STRUCT_END();

static SerializedObjectSchema const BALL_COMPONENT_SCHEMA =
    SerializedObjectSchema("BallComponent",
                           {
                               SerializedPropertySchema("moveSpeed", SerializedValueType::DOUBLE),
                           },
                           {SerializedObjectFlag::IS_COMPONENT});

class BallComponentDeserializer : public Deserializer
{
    std::string constexpr GetOwner() final override { return "FlappyPong"; }

    SerializedObjectSchema GetSchema() final override { return BALL_COMPONENT_SCHEMA; }

    void * Deserialize(DeserializationContext * ctx, SerializedObject const & obj) final override
    {
        auto ret = new BallComponent();
        ret->moveSpeed = obj.GetNumber("moveSpeed").value_or(50.f);
        return ret;
    }
};

COMPONENT_IMPL(BallComponent, new BallComponentDeserializer())

SerializedObject BallComponent::Serialize() const
{
    return SerializedObject::Builder()
        .WithString("type", this->Reflection.name)
        .WithNumber("moveSpeed", this->moveSpeed)
        .Build();
}

void BallComponent::OnEvent(HashedString name, EventArgs args)
{
    if (name == "BeginPlay") {
        velocityDir = glm::vec2((float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX);
    } else if (name == "Tick") {
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            return;
        }
        auto position = e->GetTransform()->GetPosition();
        auto scale = e->GetTransform()->GetScale();

        if (position.y <= -60.f + scale.y || position.y >= 60.f - scale.y) {
            velocityDir.y = -velocityDir.y;
        }

        if (position.x <= -80.f - scale.x || position.x >= 80.f + scale.x) {
            position.x = 0.f;
        }

        position.x = position.x + velocityDir.x * moveSpeed * args["deltaTime"].asFloat;
        position.y = position.y + velocityDir.y * moveSpeed * args["deltaTime"].asFloat;

        e->GetTransform()->SetPosition(position);
    } else if (name == "OnCollisionStart") {
        auto e = entity.Get();
        if (!e) {
            LogMissingEntity();
            return;
        }
        auto collisionInfo = (CollisionInfo *)args["info"].asPointer;
        if (collisionInfo->normals.size() == 0) {
            auto other = collisionInfo->other.Get();
            logger.Warn("OnCollisionStart with no normals. thisEntity='{}' otherEntity='{}'",
                        e->GetName(),
                        (other ? other->GetName() : ""));
            velocityDir.x = -velocityDir.x;
        } else {
            auto norm = glm::normalize(glm::vec2(collisionInfo->normals[0]));
            velocityDir -= 2 * glm::dot((glm::vec2)velocityDir, norm) * norm;
        }
    }
}
