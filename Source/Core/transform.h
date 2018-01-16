#pragma once
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
//#include "rttr/rttr_enable.h"

#include "Core/DllExport.h"
#include "Core/Math/mathtypes.h"
#include "Core/Lua/luaserializable.h"

class EAPI Transform final : public LuaSerializable
{
//RTTR_ENABLE(LuaSerializable)
public:
	static Transform Deserialize(std::string const&);

	glm::mat4 const& GetLocalToParent();
	glm::mat4 const& GetLocalToWorld();
	Transform * GetParent() const;
	Vec3 const& GetPosition() const;
	Quat const& GetRotation() const;
	Vec3 const& GetScale() const;

	std::string Serialize() const;

	void SetParent(Transform *);
	void SetPosition(Vec3 const&);
	void SetPosition(Vec3 *);
	void SetRotation(Quat const&);
	void SetScale(Vec3 const&);

private:
	Transform * parent = nullptr;

	Vec3 position = Vec3(0.f, 0.f, 0.f);
	Quat rotation = Quat(0.f, 0.f, 0.f, 0.f);
	Vec3 scale = Vec3(1.f, 1.f, 1.f);

	bool isParentDirty = true;
	bool isWorldDirty = true;

	glm::mat4 toParent;
	glm::mat4 toWorld;
};
