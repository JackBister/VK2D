#include "entity.h"

#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "json.hpp"

#include "luaindex.h"

using nlohmann::json;
using namespace std;

void Entity::FireEvent(std::string ename, EventArgs args)
{
	for (auto& c : components) {
		if (c->isActive) {
			//Only send tick event if component::receiveTicks is true
			if (ename != "Tick" || c->receiveTicks) {
				c->OnEvent(ename, args);
			}
			if (ename == "EndPlay") {
				c->isActive = false;
			}
		}
	}
}

//PUSH_TO_LUA(Entity);

//See Source/Core/Lua/luaindex.h
LUA_INDEX(Entity, string, name, LuaSerializable, transform, VECTOR(LuaSerializablePtr), components, CFunction_local, FireEvent)

int Entity::LuaNewIndex(lua_State *)
{
	return 0;
}

Entity * Entity::Deserialize(string s)
{
	Entity * ret = new Entity();
	json j = json::parse(s);
	ret->name = j["name"].get<string>();
	ret->transform = Transform::Deserialize(j["transform"].dump());
	json t = j["components"];
	for (auto& js : j["components"]) {
		Component * c = Component::Deserialize(js.dump());
		c->entity = ret;
		ret->components.push_back(c);
	}
	return ret;
}

int Entity::FireEvent_Lua(lua_State * L)
{
	int top = lua_gettop(L);
	//3rd argument is optional
	if (top < 2 || top > 3 || !lua_isuserdata(L, 1) || !lua_isstring(L, 2)) {
		lua_pushstring(L, "[ERROR] Incorrect arguments to Entity:FireEvent");
		lua_error(L);
		return 1;
	}
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	Entity * ptr = static_cast<Entity *>(*vpp);
	const char * ename = lua_tostring(L, 2);
	if (top == 3) {
		if (!lua_istable(L, 3)) {
			lua_pushstring(L, "[ERROR] Incorrect arguments to Entity:FireEvent");
			lua_error(L);
			return 1;
		}
		EventArgs eargs;
		lua_pushnil(L);
		while (lua_next(L, 3) != 0) {
			if (lua_type(L, -2) != LUA_TSTRING) {
				continue;
			}
			const char * key = lua_tostring(L, -2);
			switch (lua_type(L, -1)) {
			case LUA_TNUMBER:
				eargs[key] = lua_tonumber(L, -1);
				break;
			case LUA_TBOOLEAN:
				eargs[key] = lua_toboolean(L, -1);
				break;
			case LUA_TSTRING:
				eargs[key] = lua_tostring(L, -1);
				break;
			case LUA_TTABLE:
				//TODO:
				printf("[STUB] FireEvent_Lua case LUA_TTABLE\n");
				break;
			case LUA_TFUNCTION:
				//TODO
				printf("[STUB] FireEvent_Lua case LUA_TCFUNCTION\n");
				break;
			case LUA_TUSERDATA:
				printf("[STUB] FireEvent_Lua case LUA_TUSERDATA\n");
				break;
			case LUA_TTHREAD:
				printf("[STUB] FireEvent_Lua case LUA_TTHREAD\n");
				break;
			case LUA_TLIGHTUSERDATA:
				printf("[STUB] FireEvent_Lua case LUA_TLIGHTUSERDATA\n");
				break;
			default:
				printf("[WARNING] FireEvent_Lua default case\n");
			}
			lua_pop(L, 1);
		}
		ptr->FireEvent(ename, eargs);
	} else {
		ptr->FireEvent(ename);
	}

	return 0;
}

