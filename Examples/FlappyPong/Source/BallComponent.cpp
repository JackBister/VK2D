#include "BallComponent.h"

#include <cstdlib>

#include "Core/CollisionInfo.h"
#include "Core/Entity.h"
#include "Core/Logging/Logger.h"

static const auto logger = Logger::Create("BallComponent");

COMPONENT_IMPL(BallComponent, &BallComponent::s_Deserialize)

REFLECT_STRUCT_BEGIN(BallComponent)
REFLECT_STRUCT_MEMBER(velocityDir)
REFLECT_STRUCT_MEMBER(moveSpeed)
REFLECT_STRUCT_END();

Deserializable * BallComponent::s_Deserialize(SerializedObject const & obj)
{
    auto ret = new BallComponent();
    ret->moveSpeed = obj.GetNumber("moveSpeed").value_or(50.f);
    return ret;
}

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
        auto position = entity->transform.GetPosition();
        auto scale = entity->transform.GetScale();

        if (position.y <= -60.f + scale.y || position.y >= 60.f - scale.y) {
            velocityDir.y = -velocityDir.y;
        }

        if (position.x <= -80.f - scale.x || position.x >= 80.f + scale.x) {
            position.x = 0.f;
        }

        position.x = position.x + velocityDir.x * moveSpeed * args["deltaTime"].asFloat;
        position.y = position.y + velocityDir.y * moveSpeed * args["deltaTime"].asFloat;

        entity->transform.SetPosition(position);
    } else if (name == "OnCollisionStart") {
        auto collisionInfo = (CollisionInfo *)args["info"].asPointer;
        if (collisionInfo->normals.size() == 0) {
            logger->Warnf("OnCollisionStart with no normals. thisEntity='%s' otherEntity='%s'",
                          entity->name.c_str(),
                          collisionInfo->other->name.c_str());
            velocityDir.x = -velocityDir.x;
        } else {
            auto norm = glm::normalize(glm::vec2(collisionInfo->normals[0]));
            velocityDir -= 2 * glm::dot((glm::vec2)velocityDir, norm) * norm;
        }
    }
}
