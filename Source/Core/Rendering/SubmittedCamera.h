#pragma once

#include <glm/glm.hpp>

#include "Core/Lua/luaserializable.h"
#include "Core/Rendering/RendererData.h"

struct SubmittedCamera : LuaSerializable
{
	glm::mat4 view;
	glm::mat4 projection;

	FramebufferRendererData renderTarget;

	SubmittedCamera(const glm::mat4& view, const glm::mat4& projection, const FramebufferRendererData& renderTarget)
		: view(view), projection(projection), renderTarget(renderTarget)
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
};
