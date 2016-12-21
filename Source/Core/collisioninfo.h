#pragma once
#include <vector>

#include "Core/entity.h"
#include "Core/Lua/luaserializable.h"
#include "Core/Math/mathtypes.h"

//TODO: Cannot use headergenerator because of std::vector shenanigans
struct CollisionInfo : LuaSerializable
{
	Entity * other;
	bool collisionStart = true;
	std::vector<Vec3> normals;
	std::vector<Vec3> points;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
};
