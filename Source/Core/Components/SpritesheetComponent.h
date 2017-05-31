#pragma once
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"

#include "Core/Components/component.h"
#include "Core/sprite.h"

struct SpritesheetComponent : Component
{
	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;
	glm::vec2 GetFrameSize() const;
	void OnEvent(std::string, EventArgs) override;
	void PlayAnimationByName(std::string name);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	PROPERTY(LuaRead)
		Sprite sprite;

	PROPERTY(LuaReadWrite)
		bool isFlipped;

	std::vector<float> frameTimes;
private:
	std::unordered_map<std::string, std::vector<size_t>> animations;
	size_t currentIndex = 0;
	std::string currentNamedAnim;
	size_t currentNamedAnimIndex;
	glm::vec2 frameSize;
	std::vector<glm::vec2> minUVs;
	float timeSinceUpdate = 0.f;
};
