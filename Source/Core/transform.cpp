#include "Core/transform.h"

#include "json.hpp"
#include "lua/lua.hpp"

#include "Core/transform.h.generated.h"

using nlohmann::json;

const glm::mat4& Transform::GetLocalToParentSpace()
{
	glm::mat4 trans = glm::translate(glm::mat4(), position);
	glm::mat4 rotXYZ = glm::mat4_cast(rotation);
	glm::mat4 transrot = trans * rotXYZ;
	glm::mat4 scalem = glm::scale(glm::mat4(), scale);
	toParentSpace = transrot * scalem;
	isToParentDirty = false;
	return toParentSpace;
}

const glm::mat4& Transform::GetLocalToWorldSpace()
{
	if (isToWorldDirty) {
		if (parent == nullptr) {
			toWorldSpace = GetLocalToParentSpace();
		} else {
			toWorldSpace = parent->GetLocalToWorldSpace() * GetLocalToParentSpace();
		}
		isToWorldDirty = false;
	}
	return toWorldSpace;
}

Transform * Transform::GetParent() const
{
	return parent;
}

const Vec3& Transform::GetPosition() const
{
	return position;
}

const Quat& Transform::GetRotation() const
{
	return rotation;
}

const Vec3& Transform::GetScale() const
{
	return scale;
}

void Transform::SetParent(Transform * p)
{
	parent = p;
	isToParentDirty = true;
	isToWorldDirty = true;
}

void Transform::SetPosition(const Vec3& p)
{
	position = p;
	isToParentDirty = true;
	isToWorldDirty = true;
}

void Transform::SetRotation(const Quat& r)
{
	rotation = r;
	isToParentDirty = true;
	isToWorldDirty = true;
}

void Transform::SetScale(const Vec3& s)
{
	scale = s;
	isToParentDirty = true;
	isToWorldDirty = true;
}

Transform Transform::Deserialize(std::string s)
{
	Transform ret;
	json j = json::parse(s);
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
