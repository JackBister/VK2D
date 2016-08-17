#include "transform.h"

#include <cstring>

#include "json.hpp"
#include "lua/lua.hpp"

#include "eventarg.h"
#include "lua_cfuncs.h"
#include "luaindex.h"

#include "transform.h.generated.h"

using nlohmann::json;

void Transform::MakeDirty()
{
	isToParentDirty = true;
	isToWorldDirty = true;
}

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
