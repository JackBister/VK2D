#pragma once

#include <string>

struct lua_State;

struct LuaSerializer;
typedef int(*lua_CFunction) (lua_State *L);

struct LuaIndexable
{
	enum Type
	{
		BOOL,
		STRING,
		INT,
		FLOAT,
		DOUBLE,
		LUASERIALIZABLE,
		LUA_CFUNCTION
	};
	Type type;
	union
	{
		bool * asBool;
		std::string * asString;
		int * asInt;
		float * asFloat;
		double * asDouble;
		LuaSerializer * asLuaSerializable;
		lua_CFunction asLuaCFunction;
		uintptr_t asUintptr;
	};

	~LuaIndexable();
	//Necessary to work in initializer list for unordered_map
	LuaIndexable();
	LuaIndexable(bool *);
	LuaIndexable(std::string *);
	LuaIndexable(int *);
	LuaIndexable(float *);
	LuaIndexable(double *);
	LuaIndexable(LuaSerializer *);
	LuaIndexable(lua_CFunction);

	
	void PushToLua(lua_State *);
	void PullFromLua(lua_State *, int idx);
};