#pragma once
#include <string>
#include <unordered_map>

#include "lua/lua.hpp"

#include "component.h"

#include "Tools/HeaderGenerator/headergenerator.h"

struct LuaComponent : Component
{
	LuaComponent();

	PROPERTY(LuaRead)
	std::string file;
	//TODO: Each component has its own state - I'm not sure if that's good or if it's a waste.
	lua_State * state;

	//The args passed for the current OnEvent call.
	//When an event call is received, the args are saved here so that the lua script can use getter functions to retreive them
	//TODO: Note that this means that a LuaComponent can't run multiple OnEvents concurrently, although that may also be problematic for other reasons.
	EventArgs args;

	Component * Create(std::string json) override;
	void OnEvent(std::string name, EventArgs args) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	//This maps a lua_State to a LuaComponent. This is necessary to conform with the lua_CFunction prototype but still be able to access different parameters.
	//This is ugly.
	static std::unordered_map<lua_State *, LuaComponent *>& StateMap()
	{
		static auto ret = std::unordered_map<lua_State *, LuaComponent *>();
		return ret;
	}
};
