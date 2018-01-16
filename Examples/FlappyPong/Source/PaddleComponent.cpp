#include "PaddleComponent.h"

#include <nlohmann/json.hpp>

#include "Core/Entity.h"
#include "Core/GameModule.h"
#include "Core/Input.h"

COMPONENT_IMPL(PaddleComponent)

PaddleComponent::PaddleComponent()
{
	receiveTicks = true;
}

Deserializable * PaddleComponent::Deserialize(ResourceManager *, std::string const& str) const
{
	auto ret = new PaddleComponent();
	auto j = nlohmann::json::parse(str);
	if (j.find("flapSpeed") != j.end()) {
		ret->flapSpeed = j["flapSpeed"];
	} else {
		ret->flapSpeed = 40.f;
	}
	if (j.find("gravity") != j.end()) {
		ret->gravity = j["gravity"];
	} else {
		ret->gravity = 50.f;
	}
	return ret;
}

std::string PaddleComponent::Serialize() const
{
	nlohmann::json j;
	j["type"] = this->type;
	return j.dump();
}

void PaddleComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "Tick") {
		auto position = entity->transform.GetPosition();
		auto scale = entity->transform.GetScale();
		if (GameModule::GetInput()->GetButtonDown("Flap")) {
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


