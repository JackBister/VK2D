#include "scene.h"

#include <fstream>
#include <sstream>

#include "json.hpp"

#include "entity.h"
#include "luaindex.h"

using nlohmann::json;
using namespace std;

//PUSH_TO_LUA(Scene)

LUA_INDEX(Scene, CFunction_local, GetEntityByName, CFunction_local, BroadcastEvent)

int Scene::LuaNewIndex(lua_State * L)
{
	return 0;
}

string SlurpFile(string fn)
{
	ifstream in = ifstream(fn);
	stringstream sstr;
	sstr << in.rdbuf();
	return sstr.str();
}

Entity * Scene::GetEntityByName(std::string name)
{
	for (auto& ep : entities) {
		if (ep->name == name) {
			return ep;
		}
	}
	return nullptr;
}

int Scene::GetEntityByName_Lua(lua_State * L)
{
	int top = lua_gettop(L);
	if (top != 2 || !lua_isuserdata(L, 1) || !lua_isstring(L, 2)) {
		lua_pushstring(L, "[ERROR] Incorrect arguments to Scene:GetEntityByName");
		lua_error(L);
		return 1;
	}
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	Scene * ptr = static_cast<Scene *>(*vpp);
	const char * name = lua_tostring(L, 2);
	Entity * ret = ptr->GetEntityByName(name);
	if (ret == nullptr) {
		lua_pushnil(L);
	} else {
		ret->PushToLua(L);
	}
	return 1;
}

void Scene::BroadcastEvent(std::string ename, EventArgs eas)
{
	for (auto& ep : entities) {
		ep->FireEvent(ename, eas);
	}
}

int Scene::BroadcastEvent_Lua(lua_State * L)
{
	int top = lua_gettop(L);
	//3rd argument is optional
	if (top < 2 || top > 3 || !lua_isuserdata(L, 1) || !lua_isstring(L, 2)) {
		lua_pushstring(L, "[ERROR] Incorrect arguments to Scene:BroadcastEvent");
		lua_error(L);
		return 1;
	}
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	Scene * ptr = static_cast<Scene *>(*vpp);
	const char * ename = lua_tostring(L, 2);
	if (top == 3) {
		if (!lua_istable(L, 3)) {
			lua_pushstring(L, "[ERROR] Incorrect arguments to Scene:BroadcastEvent");
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
			//TODO: Break into own function
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
				printf("[STUB] BroadcastEvent_Lua case LUA_TTABLE\n");
				break;
			case LUA_TFUNCTION:
				//TODO
				printf("[STUB] BroadcastEvent_Lua case LUA_TCFUNCTION\n");
				break;
			case LUA_TUSERDATA:
				printf("[STUB] BroadcastEvent_Lua case LUA_TUSERDATA\n");
				break;
			case LUA_TTHREAD:
				printf("[STUB] BroadcastEvent_Lua case LUA_TTHREAD\n");
				break;
			case LUA_TLIGHTUSERDATA:
				printf("[STUB] BroadcastEvent_Lua case LUA_TLIGHTUSERDATA\n");
				break;
			default:
				printf("[WARNING] BroadcastEvent_Lua default case\n");
			}
			lua_pop(L, 1);
		}
		for (auto& ep : ptr->entities) {
			ep->FireEvent(ename, eargs);
		}
	} else {
		for (auto& ep : ptr->entities) {
			ep->FireEvent(ename);
		}
	}
	return 0;
}

Scene * Scene::FromFile(string fn)
{
	Scene * ret = new Scene();
	string fileContent = SlurpFile(fn);
	json j = json::parse(fileContent);
	ret->input = Input::Deserialize(j["input"].dump());
	ret->physicsWorld = PhysicsWorld::Deserialize(j["physics"].dump());
	for (auto& je : j["entities"]) {
		Entity * tmp = Entity::Deserialize(je.dump());
		tmp->scene = ret;
		ret->entities.push_back(tmp);
	}
	return ret;
}

