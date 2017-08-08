#include "Core/Lua/luacomponent.h"

#include "json.hpp"
#include "lua/lua.hpp"

#include "Core/entity.h"
#include "Core/input.h"
#include "Core/scene.h"

#include "Core/Lua/luacomponent.h.generated.h"

COMPONENT_IMPL(LuaComponent)

LuaComponent::LuaComponent()
{
	receive_ticks_ = true;
}

Deserializable * LuaComponent::Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(LuaComponent));
	LuaComponent * ret = new (mem) LuaComponent();
	auto j = nlohmann::json::parse(str);
	std::string infile = j["file"];
	ret->file_ = infile;
	ret->state_ = luaL_newstate();
	luaL_openlibs(ret->state_);
	int res = luaL_dofile(ret->state_, infile.c_str());
	switch (res) {
	case LUA_ERRFILE:
		printf("[WARNING] lua error opening file_ %s\n", infile.c_str());
		break;
	case LUA_ERRGCMM:
		printf("[WARNING] %s: Lua GC error.\n", file_.c_str());
		break;
	case LUA_ERRMEM:
		printf("[WARNING] lua memory error when opening file_ %s\n", infile.c_str());
		break;
	case LUA_ERRRUN:
		printf("[WARNING] %s: Lua runtime error.\n", file_.c_str());
		break;
	case LUA_ERRSYNTAX:
		printf("[WARNING] lua syntax error in file_ %s\n", infile.c_str());
		break;
	}
	if (res != LUA_OK) {
		printf("[WARNING] %s: Events to this LuaComponent will fail.\n", infile.c_str());
	}
	LuaComponent::StateMap()[ret->state_] = ret;
	return ret;
}

void LuaComponent::OnEvent(std::string name, EventArgs eargs)
{
	if (name == "BeginPlay") {
		entity_->PushToLua(state_);
		lua_setglobal(state_, "entity_");
		entity_->scene_->input_.PushToLua(state_);
		lua_setglobal(state_, "Input");
		entity_->scene_->PushToLua(state_);
		lua_setglobal(state_, "Scene");
		entity_->transform_.PushToLua(state_);
		lua_setglobal(state_, "transform");
	}
	args = eargs;
	lua_getglobal(state_, "OnEvent");
	PushToLua(state_);
	lua_pushstring(state_, name.c_str());
	//Push event args
	lua_createtable(state_, 0, static_cast<int>(eargs.size()));
	for (auto& eap : eargs) {
		eap.second.PushToLua(state_);
		lua_setfield(state_, -2, eap.first.c_str());
	}
	int res = lua_pcall(state_, 3, 0, 0);
	switch (res) {
	case LUA_ERRRUN:
		printf("[WARNING] %s: Lua runtime error.\n", file_.c_str());
		break;
	case LUA_ERRMEM:
		printf("[WARNING] %s: Lua memory error.\n", file_.c_str());
		break;
	case LUA_ERRERR:
		printf("[WARNING] %s: Lua error.\n", file_.c_str());
		break;
	case LUA_ERRGCMM:
		printf("[WARNING] %s: Lua GC error.\n", file_.c_str());
		break;
	}
	if (res != LUA_OK) {
		printf("%s\n", lua_tostring(state_, -1));
		//TODO: Deactivates to prevent stdout spam, is this good behavior?
		is_active_ = false;
	}
	if (name == "EndPlay") {
		lua_close(state_);
	}
}
