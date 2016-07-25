#pragma once
#include <string>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include "mathtypes.h"
#include "luaserializable.h"

struct Transform final : LuaSerializable
{
	Transform * parent = nullptr;

	Vec3 position = Vec3(0.f, 0.f, 0.f);
	Quat rotation = Quat(0.f, 0.f, 0.f, 0.f);
	Vec3 scale = Vec3(1.f, 1.f, 1.f);

	const glm::mat4& GetLocalToParentSpace();
	const glm::mat4& GetLocalToWorldSpace();
	void PushToLua(lua_State *);

	static Transform Deserialize(std::string);
	static int LuaIndex(lua_State *);
	static int LuaNewIndex(lua_State *);

private:
	bool isToParentDirty = true;
	bool isToWorldDirty = true;
	glm::mat4 toParentSpace;
	glm::mat4 toWorldSpace;

};
