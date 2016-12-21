#pragma once
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Core/Math/mathtypes.h"
#include "Core/Lua/luaserializable.h"

#include "Tools/HeaderGenerator/headergenerator.h"

struct Transform final : LuaSerializable
{
	const glm::mat4& GetLocalToParentSpace();
	const glm::mat4& GetLocalToWorldSpace();
	Transform * GetParent() const;
	const Vec3& GetPosition() const;
	const Quat& GetRotation() const;
	const Vec3& GetScale() const;

	void SetParent(Transform *);
	void SetPosition(const Vec3&);
	void SetRotation(const Quat&);
	void SetScale(const Vec3&);

	static Transform Deserialize(std::string);
	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

private:
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

	glm::mat4 toParentSpace;
	glm::mat4 toWorldSpace;
};
