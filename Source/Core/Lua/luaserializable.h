#pragma once

#include "lua/lua.hpp"

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

/*
This implementation of LuaSerializable::PushToLua should work in every imaginable case. It assumes the class contains a static LuaIndex and a static LuaNewIndex function.
Simply put this in your implementation file with the class name as the parameter.
*/

#define PUSH_TO_LUA(str) void str::PushToLua(lua_State * L) { \
							void ** vpp = static_cast<void **>(lua_newuserdata(L, sizeof(void *))); \
							*vpp = this; \
							if (luaL_getmetatable(L, "_" #str "MT") == LUA_TNIL) { \
								lua_pop(L, 1); \
								luaL_newmetatable(L, "_" #str "MT"); \
								lua_pushcfunction(L, str::LuaIndex); \
								lua_setfield(L, -2, "__index"); \
								lua_pushcfunction(L, str::LuaNewIndex); \
								lua_setfield(L, -2, "__newindex"); \
							} \
							lua_setmetatable(L, -2); \
						}

struct LuaSerializable
{
	virtual ~LuaSerializable();
	//This could be done in the base class, but then objects of the same type could not share a metatable
	void PushToLua(lua_State *);

	virtual int LuaIndex(lua_State *) = 0;
	virtual int LuaNewIndex(lua_State *) = 0;

	static int StaticLuaIndex(lua_State *);
	static int StaticLuaNewIndex(lua_State *);
};