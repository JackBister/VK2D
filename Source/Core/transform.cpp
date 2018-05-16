#include "Core/transform.h"

#include "nlohmann/json.hpp"

REFLECT_STRUCT_BEGIN(Transform)
REFLECT_STRUCT_MEMBER(position)
REFLECT_STRUCT_MEMBER(rotation)
REFLECT_STRUCT_MEMBER(scale)
REFLECT_STRUCT_END()

glm::mat4 const& Transform::GetLocalToParent()
{
	glm::mat4 trans = glm::translate(glm::mat4(), position);
	glm::mat4 rotXYZ = glm::mat4_cast(rotation);
	glm::mat4 transrot = trans * rotXYZ;
	glm::mat4 scalem = glm::scale(glm::mat4(), scale);
	toParent = transrot * scalem;
	isParentDirty = false;
	return toParent;
}

glm::mat4 const& Transform::GetLocalToWorld()
{
	if (isWorldDirty) {
		if (parent == nullptr) {
			toWorld = GetLocalToParent();
		} else {
			toWorld = parent->GetLocalToWorld() * GetLocalToParent();
		}
		isWorldDirty = false;
	}
	return toWorld;
}

Transform * Transform::GetParent() const
{
	return parent;
}

Vec3 const& Transform::GetPosition() const
{
	return position;
}

Quat const& Transform::GetRotation() const
{
	return rotation;
}

Vec3 const& Transform::GetScale() const
{
	return scale;
}

std::string Transform::Serialize() const
{
	nlohmann::json j; 

	j["position"]["x"] = position.x;
	j["position"]["y"] = position.y;
	j["position"]["z"] = position.z;

	j["rotation"]["x"] = rotation.x;
	j["rotation"]["y"] = rotation.y;
	j["rotation"]["z"] = rotation.z;
	j["rotation"]["w"] = rotation.w;

	j["scale"]["x"] = scale.x;
	j["scale"]["y"] = scale.y;
	j["scale"]["z"] = scale.z;

	return j.dump();
}

void Transform::SetParent(Transform * p)
{
	parent = p;
	isParentDirty = true;
	isWorldDirty = true;
}

void Transform::SetPosition(Vec3 const& p)
{
	position = p;
	isParentDirty = true;
	isWorldDirty = true;
}

void Transform::SetRotation(Quat const& r)
{
	rotation = r;
	isParentDirty = true;
	isWorldDirty = true;
}

void Transform::SetScale(Vec3 const& s)
{
	scale = s;
	isParentDirty = true;
	isWorldDirty = true;
}

Transform Transform::Deserialize(std::string const& s)
{
	Transform ret;
	auto j = nlohmann::json::parse(s);
	ret.position.x = j["position"]["x"];
	ret.position.y = j["position"]["y"];
	ret.position.z = j["position"]["z"];

	ret.rotation.x = j["rotation"]["x"];
	ret.rotation.y = j["rotation"]["y"];
	ret.rotation.z = j["rotation"]["z"];
	ret.rotation.w = j["rotation"]["w"];

	ret.scale.x = j["scale"]["x"];
	ret.scale.y = j["scale"]["y"];
	ret.scale.z = j["scale"]["z"];

	return ret;
}
