#include "luacomponent.h"

#include <cstring>

#include "json.hpp"
#include "lua/lua.hpp"

#include "entity.h"
#include "lua_cfuncs.h"
#include "luaindex.h"
#include "scene.h"

#include "luacomponent.h.generated.h"

using nlohmann::json;
using namespace std;

COMPONENT_IMPL(LuaComponent)

LuaComponent::LuaComponent()
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
		entity->PushToLua(state);
		lua_setglobal(state, "entity");
		entity->scene->input->PushToLua(state);
		lua_setglobal(state, "Input");
		entity->scene->PushToLua(state);
		lua_setglobal(state, "Scene");
		entity->transform.PushToLua(state);
		lua_setglobal(state, "transform");
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
