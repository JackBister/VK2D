#pragma once
#include <memory>

#include <glm/glm.hpp>

#include "Core/Lua/luaserializable.h"
#include "Core/Rendering/Image.h"

class Sprite : public LuaSerializable
{
	//RTTR_ENABLE(LuaSerializable)
public:
	Sprite() = default;
	Sprite(std::shared_ptr<Image>);

	//The bottom left UV coordinate of what we want to render
	glm::vec2 minUv = glm::vec2(0.f, 0.f);
	//The size of the subimage in UV coordinates
	glm::vec2 sizeUv = glm::vec2(1.f, 1.f);
	//Whether this sprite should be rendered or not
	bool isVisible;

	std::shared_ptr<Image> image;
};
