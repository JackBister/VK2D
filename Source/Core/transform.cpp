#include "Core/transform.h"

#include "json.hpp"
#include "lua/lua.hpp"
#include "rttr/registration.h"

#include "Core/transform.h.generated.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Transform>("Transform")
	.constructor<>()
	.method("local_to_parent", &Transform::local_to_parent)
	.method("local_to_world", &Transform::local_to_world)
	.method("get_parent", &Transform::get_parent)
	.method("get_position", &Transform::get_position)
	.method("get_rotation", &Transform::get_rotation)
	.method("get_scale", &Transform::get_scale)
	.method("set_parent", &Transform::set_parent)
	.method("set_position", &Transform::set_position)
	.method("set_rotation", &Transform::set_rotation)
	.method("set_scale", &Transform::set_scale);
}

glm::mat4 const& Transform::local_to_parent()
{
	glm::mat4 trans = glm::translate(glm::mat4(), position_);
	glm::mat4 rotXYZ = glm::mat4_cast(rotation_);
	glm::mat4 transrot = trans * rotXYZ;
	glm::mat4 scalem = glm::scale(glm::mat4(), scale_);
	to_parent_ = transrot * scalem;
	is_parent_dirty_ = false;
	return to_parent_;
}

glm::mat4 const& Transform::local_to_world()
{
	if (is_world_dirty_) {
		if (parent == nullptr) {
			to_world_ = local_to_parent();
		} else {
			to_world_ = parent->local_to_world() * local_to_parent();
		}
		is_world_dirty_ = false;
	}
	return to_world_;
}

Transform * Transform::get_parent() const
{
	return parent;
}

Vec3 const& Transform::get_position() const
{
	return position_;
}

Quat const& Transform::get_rotation() const
{
	return rotation_;
}

Vec3 const& Transform::get_scale() const
{
	return scale_;
}

void Transform::set_parent(Transform * p)
{
	parent = p;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

void Transform::set_position(Vec3 const& p)
{
	position_ = p;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

void Transform::set_rotation(Quat const& r)
{
	rotation_ = r;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

void Transform::set_scale(Vec3 const& s)
{
	scale_ = s;
	is_parent_dirty_ = true;
	is_world_dirty_ = true;
}

Transform Transform::Deserialize(std::string const& s)
{
	Transform ret;
	auto j = nlohmann::json::parse(s);
	ret.position_.x = j["position"]["x"];
	ret.position_.y = j["position"]["y"];
	ret.position_.z = j["position"]["z"];

	ret.rotation_.x = j["rotation"]["x"];
	ret.rotation_.y = j["rotation"]["y"];
	ret.rotation_.z = j["rotation"]["z"];
	ret.rotation_.w = j["rotation"]["w"];

	ret.scale_.x = j["scale"]["x"];
	ret.scale_.y = j["scale"]["y"];
	ret.scale_.z = j["scale"]["z"];

	return ret;
}
