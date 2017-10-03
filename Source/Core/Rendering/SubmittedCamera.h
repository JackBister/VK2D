#pragma once

#include <glm/glm.hpp>

#include "Core/Lua/luaserializable.h"

struct SubmittedCamera : LuaSerializable
{
	SubmittedCamera(glm::mat4 const& view, glm::mat4 const& projection)
		: view(view), projection(projection)
	{
	}
	glm::mat4 view;
	glm::mat4 projection;
};
