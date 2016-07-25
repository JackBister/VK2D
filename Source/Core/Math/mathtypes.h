#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "luaserializable.h"

struct Vec3 : glm::vec3
{
	using glm::vec3::tvec3;

	void PushToLua(lua_State *);

	static int LuaIndex(lua_State *);
	static int LuaNewIndex(lua_State *);
};

struct Quat : glm::quat
{
	using glm::quat::tquat;

	void PushToLua(lua_State *);

	static int LuaIndex(lua_State *);
	static int LuaNewIndex(lua_State *);
};
