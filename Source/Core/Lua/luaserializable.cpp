#include "Core/Lua/luaserializable.h"

void LuaSerializable::PushToLua(lua_State * L)
{
		void ** vpp = static_cast<void **>(lua_newuserdata(L, sizeof(void *)));
		*vpp = this;
		if (luaL_getmetatable(L, "_LuaSerializableMT") == LUA_TNIL) {
				lua_pop(L, 1);
				luaL_newmetatable(L, "_LuaSerializableMT");
				lua_pushcfunction(L, StaticLuaIndex);
				lua_setfield(L, -2, "__index");
				lua_pushcfunction(L, StaticLuaNewIndex);
				lua_setfield(L, -2, "__newindex");
		}
		lua_setmetatable(L, -2);
}

int LuaSerializable::StaticLuaIndex(lua_State * L)
{
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	LuaSerializable * lsp = static_cast<LuaSerializable *>(*vpp);
	return lsp->LuaIndex(L);
}

int LuaSerializable::StaticLuaNewIndex(lua_State * L)
{
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1));
	LuaSerializable * lsp = static_cast<LuaSerializable *>(*vpp);
	return lsp->LuaNewIndex(L);
}

LuaSerializable::~LuaSerializable() {}
