#pragma once
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include "mathtypes.h"
#include "luaserializable.h"

#include "Tools/HeaderGenerator/headergenerator.h"

struct Transform final : LuaSerializable
{
	Transform * parent = nullptr;

	PROPERTY(LuaRead)
	Vec3 position = Vec3(0.f, 0.f, 0.f);
	PROPERTY(LuaWrite)
	Quat rotation = Quat(0.f, 0.f, 0.f, 0.f);
	PROPERTY(LuaReadWrite)
	Vec3 scale = Vec3(1.f, 1.f, 1.f);

	PROPERTY(LuaReadWrite, NoJSON)
	bool isToParentDirty = true;
	PROPERTY(LuaReadWrite, NoJSON)
	bool isToWorldDirty = true;

	//TODO: remove
	void MakeDirty();

	const glm::mat4& GetLocalToParentSpace();
	const glm::mat4& GetLocalToWorldSpace();
	//void PushToLua(lua_State *);

	static Transform Deserialize(std::string);
	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

private:
	glm::mat4 toParentSpace;
	glm::mat4 toWorldSpace;
};
