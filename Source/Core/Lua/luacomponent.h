#pragma once
#include <string>
#include <unordered_map>

#include "lua.hpp"
#include "rttr/rttr_enable.h"

#include "Core/Components/component.h"

class LuaComponent : public Component
{
	RTTR_ENABLE(Component)
public:
	LuaComponent();
	
	Deserializable * Deserialize(ResourceManager *, std::string const& str, Allocator& alloc = Allocator::default_allocator) const override;
	std::string Serialize() const override;
	void OnEvent(std::string name, EventArgs args) override;

	//This maps a lua_State to a LuaComponent. This is necessary to conform with the lua_CFunction prototype but still be able to access different parameters.
	//This is ugly.
	static std::unordered_map<lua_State *, LuaComponent *>& StateMap()
	{
		static auto ret = std::unordered_map<lua_State *, LuaComponent *>();
		return ret;
	}

private:
	std::string file;
	lua_State * state;

	//The args passed for the current OnEvent call.
	//When an event call is received, the args are saved here so that the lua script can use getter functions to retreive them
	EventArgs args;
};
