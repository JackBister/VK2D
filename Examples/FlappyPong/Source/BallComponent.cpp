#include "BallComponent.h"

#include <cstdlib>
#include <nlohmann/json.hpp>

#include "Core/CollisionInfo.h"
#include "Core/Entity.h"

COMPONENT_IMPL(BallComponent)

BallComponent::BallComponent()
{
	receiveTicks = true;
}

Deserializable * BallComponent::Deserialize(ResourceManager *, std::string const& str) const
{
	auto ret = new BallComponent();
	auto j = nlohmann::json::parse(str);
	if (j.find("moveSpeed") != j.end()) {
		ret->moveSpeed = j["moveSpeed"];
	} else {
		ret->moveSpeed = 50.f;
	}
	return ret;
}

std::string BallComponent::Serialize() const
{
	nlohmann::json j;
	j["type"] = this->type;
	return j.dump();
}

void BallComponent::OnEvent(std::string name, EventArgs args)
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
			printf("[WARNING] OnCollisionStart with no normals.");
			velocityDir.x = -velocityDir.x;
		} else {
			auto norm = glm::normalize(glm::vec2(collisionInfo->normals[0]));
			velocityDir -= 2 * glm::dot(velocityDir, norm) * norm;
		}
	}
}
