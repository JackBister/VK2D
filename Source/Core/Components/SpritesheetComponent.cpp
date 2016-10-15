#include "spritesheetcomponent.h"

#include "json.hpp"

#include "entity.h"
#include "render.h"
#include "sprite.h"
#include "SpritesheetComponent.h.generated.h"

COMPONENT_IMPL(SpritesheetComponent)

Deserializable * SpritesheetComponent::Deserialize(const std::string & str, Allocator & alloc) const
{
	using nlohmann::json;
	void * mem = alloc.Allocate(sizeof(SpritesheetComponent));
	SpritesheetComponent * ret = new (mem) SpritesheetComponent();
	json j = json::parse(str);
	ret->receiveTicks = true;
	ret->sprite = Sprite::FromFile(nullptr, j["file"].get<std::string>().c_str());
	glm::ivec2 frameSize = glm::ivec2(j["frameSize"]["x"], j["frameSize"]["y"]);
	ret->frameSize = glm::vec2(frameSize.x / (float)ret->sprite.dimensions.x, frameSize.y / (float)ret->sprite.dimensions.y);
	for (int y = 0; y < ret->sprite.dimensions.y / frameSize.y; ++y) {
		for (int x = 0; x < ret->sprite.dimensions.x / frameSize.x; ++x) {
			ret->minUVs.push_back(glm::vec2(x * frameSize.x / (float)ret->sprite.dimensions.x, y * frameSize.y / (float)ret->sprite.dimensions.y));
		}
	}
	if (j.find("frameTimes") != j.end()) {
		ret->frameTimes = j["frameTimes"].get<std::vector<float>>();
	} else {
		ret->frameTimes.push_back(0.2f);
	}

	if (j.find("animations") != j.end()) {
		for (const json& animJSON : j["animations"]) {
			ret->animations[animJSON["name"]] = animJSON["frames"].get<std::vector<size_t>>();
		}
	}
	//TODO:
	ret->PlayAnimationByName("Run");

	ret->isFlipped = false;
	return ret;
}

glm::vec2 SpritesheetComponent::GetFrameSize() const
{
	return frameSize;
}

void SpritesheetComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "BeginPlay") {
		sprite.transform = &(entity->transform);
		sprite.minUV = minUVs[currentIndex];
		sprite.sizeUV = frameSize;
		Render_currentRenderer->AddSprite(&sprite);
	}
	if (name == "Tick") {
		timeSinceUpdate += args["deltaTime"].asFloat;
		if (timeSinceUpdate >= frameTimes[currentIndex % frameTimes.size()]) {
			currentNamedAnimIndex++;
			if (currentNamedAnimIndex == animations[currentNamedAnim].size()) {
				currentNamedAnimIndex = 0;
			}
			currentIndex = animations[currentNamedAnim][currentNamedAnimIndex];
			timeSinceUpdate = 0.f;
			if (isFlipped) {
				sprite.sizeUV.x = -frameSize.x;
				sprite.minUV = minUVs[currentIndex];
				sprite.minUV.x += frameSize.x;
			} else {
				sprite.sizeUV = frameSize;
				sprite.minUV = minUVs[currentIndex];
			}
		}
	}
}

void SpritesheetComponent::PlayAnimationByName(std::string name)
{
	currentNamedAnim = name;
	currentIndex = animations[name][0];
	currentNamedAnimIndex = 0;
}
