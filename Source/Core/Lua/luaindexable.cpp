#include "luaindexable.h"

#include <cassert>
#include <string>

#include "lua/lua.hpp"

#include "luaserializable.h"

LuaIndexable::~LuaIndexable()
{
}

LuaIndexable::LuaIndexable()
{
}

LuaIndexable::LuaIndexable(bool * b) : type(Type::BOOL), asBool(b)
{
}


LuaIndexable::LuaIndexable(std::string * s) : type(Type::STRING), asString(s)
{
}

LuaIndexable::LuaIndexable(int * i) : type(Type::INT), asInt(i)
{
}

LuaIndexable::LuaIndexable(float * f) : type(Type::FLOAT), asFloat(f)
{
}

LuaIndexable::LuaIndexable(double * d) : type(Type::DOUBLE), asDouble(d)
{
}

LuaIndexable::LuaIndexable(LuaSerializer * l) : type(Type::LUASERIALIZABLE), asLuaSerializable(l)
{
}

LuaIndexable::LuaIndexable(lua_CFunction cf) : type(Type::LUA_CFUNCTION), asLuaCFunction(cf)
{
}

void LuaIndexable::PushToLua(lua_State * L)
{
	switch (type) {
	case LuaIndexable::Type::STRING:
		lua_pushstring(L, asString->c_str());
		break;
	case LuaIndexable::Type::INT:
		lua_pushnumber(L, *asInt);
		break;
	case LuaIndexable::Type::FLOAT:
		lua_pushnumber(L, *asFloat);
		break;
	case LuaIndexable::Type::DOUBLE:
		lua_pushnumber(L, *asDouble);
		break;
	case LuaIndexable::Type::LUASERIALIZABLE:
		asLuaSerializable->PushToLua(L);
		break;
	case LuaIndexable::Type::LUA_CFUNCTION:
		lua_pushcfunction(L, asLuaCFunction);
		break;
	default:
		printf("[ERROR] Unknown arg type in LuaIndexable::PushToLua: %d\n", type);
		lua_pushnil(L);
	}
}

void LuaIndexable::PullFromLua(lua_State * L, int idx)
{
	switch (type) {
	case LuaIndexable::Type::STRING:
		if (!lua_isstring(L, idx)) {
			return;
		}
		*asString = lua_tostring(L, idx);
		break;
	case LuaIndexable::Type::INT:
		if (!lua_isnumber(L, idx)) {
			return;
		}
		*asInt = static_cast<int>(lua_tonumber(L, idx));
		break;
	case LuaIndexable::Type::FLOAT:
		if (!lua_isnumber(L, idx)) {
			return;
		}
		*asFloat = static_cast<float>(lua_tonumber(L, idx));
		break;
	case LuaIndexable::Type::DOUBLE:
		if (!lua_isnumber(L, idx)) {
			return;
		}
		*asFloat = lua_tonumber(L, idx);
		break;
	case LuaIndexable::Type::LUASERIALIZABLE:
		printf("[TODO] LuaIndexable::PullFromLua LuaSerializable\n");
		break;
	case LuaIndexable::Type::LUA_CFUNCTION:
		printf("[TODO] LuaIndexable PullFromLua CFunction\n");
		break;
	default:
		printf("[ERROR] Unknown arg type in LuaIndexable::PushToLua: %d\n", type);
		lua_pushnil(L);
	}
}
