#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Core/Lua/luaserializable.h"

struct Vec3 : glm::vec3, LuaSerializable
{
	using glm::vec3::tvec3;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
};

struct Quat : glm::quat, LuaSerializable
{
	using glm::quat::tquat;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
};
