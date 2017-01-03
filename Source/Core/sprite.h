#pragma once
#include <cstddef>
#include <memory>

#include <glm/glm.hpp>

#include "Core/Lua/luaserializable.h"
#include "Core/Rendering/Image.h"
#include "Core/transform.h"

#include "Tools/HeaderGenerator/headergenerator.h"

/*
	A sprite object should contain everything the renderer needs to put a sprite on the screen.
*/
struct Sprite : LuaSerializable
{
	//A transform representing a world space position the sprite is located at
	PROPERTY(LuaRead)
	Transform * transform;
	//The bottom left UV coordinate of what we want to render
	glm::vec2 minUV = glm::vec2(0.f, 0.f);
	//The size of the subimage in UV coordinates
	glm::vec2 sizeUV = glm::vec2(1.f, 1.f);
	//Whether this sprite should be rendered or not
	PROPERTY(LuaReadWrite)
	bool isVisible;

	std::shared_ptr<Image> image;

	Sprite() = default;
	Sprite(Transform *, std::shared_ptr<Image>);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
};
