#pragma once
#include <cstddef>
#include <memory>

#include <glm/glm.hpp>

#include "Core/Lua/luaserializable.h"
#include "Core/Rendering/Image.h"
#include "Core/transform.h"

#include "Tools/HeaderGenerator/headergenerator.h"

class Sprite : public LuaSerializable
{
public:
	Sprite() = default;
	Sprite(std::shared_ptr<Image>);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	//The bottom left UV coordinate of what we want to render
	glm::vec2 min_uv_ = glm::vec2(0.f, 0.f);
	//The size of the subimage in UV coordinates
	glm::vec2 size_uv_ = glm::vec2(1.f, 1.f);
	//Whether this sprite should be rendered or not
	PROPERTY(LuaReadWrite)
	bool is_visible_;

	std::shared_ptr<Image> image_;
};
