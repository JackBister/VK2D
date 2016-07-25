#include "luacomponent.h"

#include <cstring>

#include "json.hpp"
#include "lua/lua.hpp"

#include "entity.h"
#include "lua_cfuncs.h"
#include "luaindex.h"
#include "scene.h"

using nlohmann::json;
using namespace std;

inline LuaComponent::LuaComponent()
{
	receiveTicks = true;
}

Component * LuaComponent::Create(string s)
{
	LuaComponent * ret = new LuaComponent();
	json j = json::parse(s);
	string infile = j["file"];
	ret->file = infile;
	ret->state = luaL_newstate();
	luaL_openlibs(ret->state);
	PushCFuncs(ret->state);
	int res = luaL_dofile(ret->state, infile.c_str());
	switch (res) {
	case LUA_ERRFILE:
		printf("[WARNING] lua error opening file %s\n", infile.c_str());
		break;
	case LUA_ERRGCMM:
		printf("[WARNING] %s: Lua GC error.\n", file.c_str());
		break;	
	case LUA_ERRMEM:
		printf("[WARNING] lua memory error when opening file %s\n", infile.c_str());
		break;
	case LUA_ERRRUN:
		printf("[WARNING] %s: Lua runtime error.\n", file.c_str());
		break;
	case LUA_ERRSYNTAX:
		printf("[WARNING] lua syntax error in file %s\n", infile.c_str());
		break;
	}
	if (res != LUA_OK) {
		printf("[WARNING] %s: Events to this LuaComponent will fail.\n", infile.c_str());
	}
	LuaComponent::StateMap()[ret->state] = ret;
	return ret;
}

void LuaComponent::OnEvent(string name, EventArgs eargs)
{
	if (name == "BeginPlay") {
		entity->scene->input->PushToLua(state);
		lua_setglobal(state, "Input");
		entity->scene->PushToLua(state);
		lua_setglobal(state, "Scene");
		entity->transform.PushToLua(state);
		lua_setglobal(state, "transform");
	}
	if (entity->name == "Ball") {
		//printf("%f %f\n", entity->transform.position.x, entity->transform.position.y);
	}
	args = eargs;
	lua_getglobal(state, "OnEvent");
	PushToLua(state);
	lua_pushstring(state, name.c_str());
	//Push event args
	lua_createtable(state, 0, static_cast<int>(eargs.size()));
	for (auto& eap : eargs) {
		eap.second.PushToLua(state);
		lua_setfield(state, -2, eap.first.c_str());
	}
	int res = lua_pcall(state, 3, 0, 0);
	switch (res) {
	case LUA_ERRRUN:
		printf("[WARNING] %s: Lua runtime error.\n", file.c_str());
		break;
	case LUA_ERRMEM:
		printf("[WARNING] %s: Lua memory error.\n", file.c_str());
		break;
	case LUA_ERRERR:
		printf("[WARNING] %s: Lua error.\n", file.c_str());
		break;
	case LUA_ERRGCMM:
		printf("[WARNING] %s: Lua GC error.\n", file.c_str());
		break;
	}
	if (res != LUA_OK) {
		printf("%s\n", lua_tostring(state, -1));
		//TODO: Deactivates to prevent stdout spam, is this good behavior?
		isActive = false;
	}
	if (name == "EndPlay") {
		lua_close(state);
	}
}


PUSH_TO_LUA(LuaComponent)

//See Source/Core/Lua/luaindex.h
LUA_INDEX(LuaComponent, LuaSerializablePtr, entity, string, type, bool, isActive, bool, receiveTicks, string, file, EventArgs, args)

/*
//	Gonna leave this here for a while so we can look in awe at how much the LUA_INDEX macro saves
int LuaComponent::LuaIndex(lua_State * L)
{
	void ** luaCompPtr = static_cast<void **>(lua_touserdata(L, 1));
	LuaComponent * ptr = static_cast<LuaComponent *>(*luaCompPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(idx, "entity") == 0) {
		ptr->entity->PushToLua(L);
	} else if (strcmp(idx, "type") == 0) {
		lua_pushstring(L, ptr->type.c_str());
	} else if (strcmp(idx, "isActive") == 0) {
		lua_pushboolean(L, ptr->isActive);
	} else if (strcmp(idx, "receiveTicks") == 0) {
		lua_pushboolean(L, ptr->receiveTicks);
	} else if (strcmp(idx, "file") == 0) {
		lua_pushstring(L, ptr->file.c_str());
	} else if (strcmp(idx, "args") == 0) {
		lua_createtable(L, 0, ptr->args.size());
		for (auto& eap : ptr->args) {
			eap.second.PushToLua(L);
			lua_setfield(L, -2, eap.first.c_str());
		}
	} else {
		lua_pushnil(L);
	}
	return 1;
}
*/

LUA_NEW_INDEX(LuaComponent, bool, isActive, bool, receiveTicks)

/*
//TODO: Assigning to file is stupid since it wont have effect
//In future maybe use getter/setter
int LuaComponent::LuaNewIndex(lua_State * L)
{
	if (lua_gettop(L) != 3) {
		return 0;
	}
	void ** luaCompPtr = static_cast<void **>(lua_touserdata(L, 1));
	LuaComponent * ptr = static_cast<LuaComponent *>(*luaCompPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		return 0;
	}
	
	if (strcmp(idx, "isActive") == 0) {
		if (!lua_isboolean(L, 3)) {
			return 0;
		}
		ptr->isActive = lua_toboolean(L, 3);
	} else if (strcmp(idx, "receiveTicks") == 0) {
		if (!lua_isboolean(L, 3)) {
			return 0;
		}
		ptr->receiveTicks = lua_toboolean(L, 3);
	} 
	return 0;
}
*/

