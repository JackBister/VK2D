#pragma once

#include <assert.h>

#include "lua.hpp"
#include "rttr/method.h"
#include "rttr/property.h"
#include "rttr/rttr_enable.h"
#include "rttr/type.h"
#include "rttr/variant.h"

/*
	A LuaSerializable must do the following:
	When PushToLua is called, the LuaSerializable will put a pointer to itself at the top of the lua stack, with a metatable that allows lua to
	access its members as if it were a type. This is done via the LuaIndex and LuaNewIndex functions. These must be defined as static methods of the derived class.
	The metatable should map __index to LuaIndex and __newindex to LuaNewIndex.

	LuaIndex receives two arguments: The userdata at -2 on the stack and an index at -1 on the stack. Note that just doing lua_touserdata will just give you
	a pointer to the memory block containing the userdata, meaning you must dereference it twice to get to the LuaSerializable instance. From there, the index
	should be used to return a representation of the variable with the name index on the lua stack.
	In short: obj.x in lua will call LuaIndex which will receive a pointer to a pointer to obj in C++. LuaIndex should return the value of obj.x on the Lua stack.
	If the class has no member with that name, it should return nil.

	LuaNewIndex receives three arguments: Userdata at -3, index at -2, and a new value at -1. Like above, it should map to a variable in C++, but instead
	of returning it assign the value at -1 to it. This function should never return anything, wether the index exists or not.
*/

int TypeError(lua_State * L);

int VPushToLua(lua_State * L, rttr::variant const& v);

class LuaSerializable
{
	RTTR_ENABLE()
public:
	virtual ~LuaSerializable() {}

	int LuaIndex(lua_State * L);


	int LuaNewIndex(lua_State * L);
	void PushToLua(void * Lx);


	static int StaticLuaIndex(lua_State * L);

	static int StaticLuaNewIndex(lua_State * L);


	static int StaticStackLuaIndex(lua_State * L);

	static int StaticStackLuaNewIndex(lua_State * L);
};
