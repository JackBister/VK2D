#pragma once
#include <unordered_map>
#include <utility>
#include <vector>

#include "glm/glm.hpp"

#include "component.h"
#include "sprite.h"

struct SpritesheetComponent : Component
{
	PROPERTY(LuaRead)
	Sprite sprite;

	PROPERTY(LuaReadWrite)
	bool isFlipped;

//	PROPERTY(LuaReadWrite)
	std::vector<float> frameTimes;

	Component * Create(std::string) override;
	glm::vec2 GetFrameSize() const;
	void OnEvent(std::string, EventArgs) override;
	void PlayAnimationByName(std::string name);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

private:
	size_t currentIndex = 0;
	float timeSinceUpdate = 0.f;
	std::vector<glm::vec2> minUVs;
	glm::vec2 frameSize;
	std::string currentNamedAnim;
	size_t currentNamedAnimIndex;
	std::unordered_map<std::string, std::vector<size_t>> animations;
};
