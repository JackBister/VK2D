#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "luaserializable.h"

struct Vec3 : glm::vec3, LuaSerializable
{
	using glm::vec3::tvec3;

	//void PushToLua(lua_State *);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
};

struct Quat : glm::quat, LuaSerializable
{
	using glm::quat::tquat;

	//void PushToLua(lua_State *);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
};
