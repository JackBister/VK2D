#include "Core/Lua/luacomponent.h"

#include "nlohmann/json.hpp"
#include "lua.hpp"
#include "rttr/registration.h"

#include "Core/entity.h"
#include "Core/input.h"
#include "Core/scene.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<LuaComponent>("LuaComponent")
	.constructor<>();
}

COMPONENT_IMPL(LuaComponent)

LuaComponent::LuaComponent()
{
	receiveTicks = true;
}

Deserializable * LuaComponent::Deserialize(ResourceManager * resourceManager, std::string const& str) const
{
	LuaComponent * ret = new LuaComponent();
	auto j = nlohmann::json::parse(str);
	std::string infile = j["file"];
	ret->file = infile;
	ret->state = luaL_newstate();
	luaL_openlibs(ret->state);
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

std::string LuaComponent::Serialize() const
{
	nlohmann::json j;
	j["type"] = this->type;
	j["file"] = file;

	return j.dump();
}

void LuaComponent::OnEvent(std::string name, EventArgs eargs)
{
	if (name == "BeginPlay") {
		entity->PushToLua(state);
		lua_setglobal(state, "entity_");
		GameModule::GetInput()->PushToLua(state);
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
