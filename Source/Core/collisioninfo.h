#pragma once
#include <vector>

#include "entity.h"
#include "luaserializable.h"
#include "mathtypes.h"

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
