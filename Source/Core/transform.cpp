#include "transform.h"

#include <cstring>

#include "json.hpp"
#include "lua/lua.hpp"

#include "eventarg.h"
#include "lua_cfuncs.h"
#include "luaindex.h"

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


PUSH_TO_LUA(Transform)
LUA_INDEX(Transform, LuaSerializable, position, LuaSerializable, rotation, LuaSerializable, scale, bool, isToParentDirty, bool, isToWorldDirty)

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


int Transform::LuaNewIndex(lua_State * L)
{
	void ** transformPtr = static_cast<void **>(lua_touserdata(L, 1));
	Transform * ptr = static_cast<Transform *>(*transformPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		return 0;
	}
	if (strcmp(idx, "isToParentDirty") == 0) {
		if (!lua_isboolean(L, 3)) {
			return 0;
		}
		ptr->isToParentDirty = lua_toboolean(L, 3);
	} else if (strcmp(idx, "isToWorldDirty") == 0) {
		if (!lua_isboolean(L, 3)) {
			return 0;
		}
		ptr->isToWorldDirty = lua_toboolean(L, 3);
	}
	return 0;
}

