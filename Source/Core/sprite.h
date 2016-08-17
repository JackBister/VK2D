#pragma once
#include <cstddef>

#include "glm/glm.hpp"
#include "transform.h"

#include "luaserializable.h"

#include "Tools/HeaderGenerator/headergenerator.h"

/*
	A sprite object should contain everything the renderer needs to put a sprite on the screen.
	The rendererdata pointer is for renderers to store their own associated data in, such as a texture ID.
*/
struct Sprite : LuaSerializable
{
	//A transform representing a world space position the sprite is located at
	PROPERTY(LuaRead)
	Transform * transform;
	//The layer the sprite is on, used for parallaxing
	//lower number = closer to the camera
	PROPERTY(LuaReadWrite)
	int layer;
	//The bytes representing the image, one byte per channel
	uint8_t * data;
	//1: gray 2: gray, alpha 3: RGB 4: RGBA
	//This determines how to interpret the data. e.g. mode 1 means 1 byte = 1 pixel, mode 4 means 1 pixel = 4 bytes
	PROPERTY(LuaRead)
	int components;
	//width and height of the image
	glm::ivec2 dimensions;
	//Wether this sprite should be rendered or not
	PROPERTY(LuaReadWrite)
	bool isVisible;
	//Renderer-specific data
	void * rendererData;

	static Sprite FromFile(Transform * const transform, const char * const fileName, const int forcedComponents = 0);	
	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
};
