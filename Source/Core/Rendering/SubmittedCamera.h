#pragma once

#include <glm/glm.hpp>

#include "Core/Lua/luaserializable.h"

struct SubmittedCamera : LuaSerializable
{
	SubmittedCamera(glm::mat4 const& view, glm::mat4 const& projection)
		: view(view), projection(projection)
	{
	}

	int LuaIndex(lua_State * L) override
	{
		printf("[STUB] SubmittedCamera::LuaIndex\n");
		lua_pushnil(L);
		return 1;
	}
	int LuaNewIndex(lua_State * l) override
	{
		printf("[STUB] SubmittedCamera::LuaNewIndex\n");
		return 0;
	}

	glm::mat4 view;
	glm::mat4 projection;
};
