#pragma once
#include <string>
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "rttr/rttr_enable.h"

#include "Core/Math/mathtypes.h"
#include "Core/Lua/luaserializable.h"

#include "Tools/HeaderGenerator/headergenerator.h"

class Transform final : public LuaSerializable
{
	RTTR_ENABLE(LuaSerializable)
public:
	static Transform Deserialize(std::string const&);

	glm::mat4 const& local_to_parent();
	glm::mat4 const& local_to_world();
	Transform * get_parent() const;
	Vec3 const& get_position() const;
	Quat const& get_rotation() const;
	Vec3 const& get_scale() const;

	void set_parent(Transform *);
	void set_position(Vec3 const&);
	void set_position(Vec3 *);
	void set_rotation(Quat const&);
	void set_scale(Vec3 const&);

private:
	Transform * parent = nullptr;

	PROPERTY(LuaRead)
		Vec3 position_ = Vec3(0.f, 0.f, 0.f);
	PROPERTY(LuaWrite)
		Quat rotation_ = Quat(0.f, 0.f, 0.f, 0.f);
	PROPERTY(LuaReadWrite)
		Vec3 scale_ = Vec3(1.f, 1.f, 1.f);

	PROPERTY(LuaReadWrite, NoJSON)
		bool is_parent_dirty_ = true;
	PROPERTY(LuaReadWrite, NoJSON)
		bool is_world_dirty_ = true;

	glm::mat4 to_parent_;
	glm::mat4 to_world_;
};
