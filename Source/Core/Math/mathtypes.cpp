#include "Core/Math/mathtypes.h"

#include <cstring>

//#include "luaindex.h"

int Vec3::LuaIndex(lua_State * L)
{
	void ** v3Ptr = static_cast<void **>(lua_touserdata(L, 1));
	Vec3 * ptr = static_cast<Vec3 *>(*v3Ptr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(idx, "x") == 0) {
		lua_pushnumber(L, ptr->x);
	} else if (strcmp(idx, "y") == 0) {
		lua_pushnumber(L, ptr->y);
	} else if (strcmp(idx, "z") == 0) {
		lua_pushnumber(L, ptr->z);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Vec3::LuaNewIndex(lua_State * L)
{
	if (lua_gettop(L) != 3) {
		return 0;
	}
	void ** v3Ptr = static_cast<void **>(lua_touserdata(L, 1));
	Vec3 * ptr = static_cast<Vec3 *>(*v3Ptr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		return 0;
	}
	if (strcmp(idx, "x") == 0) {
		if (!lua_isnumber(L, 3)) {
			return 0;
		}
		ptr->x = lua_tonumber(L, 3);
	} else if (strcmp(idx, "y") == 0) {
		if (!lua_isnumber(L, 3)) {
			return 0;
		}
		ptr->y = lua_tonumber(L, 3);
	} else if (strcmp(idx, "z") == 0) {
		if (!lua_isnumber(L, 3)) {
			return 0;
		}
		ptr->z = lua_tonumber(L, 3);
	}
	return 0;
}

int Quat::LuaIndex(lua_State * L)
{
	void ** qPtr = static_cast<void **>(lua_touserdata(L, 1));
	Quat * ptr = static_cast<Quat *>(*qPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(idx, "x") == 0) {
		lua_pushnumber(L, ptr->x);
	} else if (strcmp(idx, "y") == 0) {
		lua_pushnumber(L, ptr->y);
	} else if (strcmp(idx, "z") == 0) {
		lua_pushnumber(L, ptr->z);
	} else if (strcmp(idx, "w") == 0) {
		lua_pushnumber(L, ptr->w);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Quat::LuaNewIndex(lua_State * L)
{
	if (lua_gettop(L) != 3) {
		return 0;
	}
	void ** qPtr = static_cast<void **>(lua_touserdata(L, 1));
	Quat * ptr = static_cast<Quat *>(*qPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		return 0;
	}
	if (strcmp(idx, "x") == 0) {
		if (!lua_isnumber(L, 3)) {
			return 0;
		}
		ptr->x = lua_tonumber(L, 3);
	} else if (strcmp(idx, "y") == 0) {
		if (!lua_isnumber(L, 3)) {
			return 0;
		}
		ptr->y = lua_tonumber(L, 3);
	} else if (strcmp(idx, "z") == 0) {
		if (!lua_isnumber(L, 3)) {
			return 0;
		}
		ptr->z = lua_tonumber(L, 3);
	} else if (strcmp(idx, "w") == 0) {
		if (!lua_isnumber(L, 3)) {
			return 0;
		}
		ptr->w = lua_tonumber(L, 3);
	}
	return 0;
}

/*
LUA_INDEX(Vec3, float, x, float, y, float, z)
LUA_NEW_INDEX(Vec3, float, x, float, y, float, z)

LUA_INDEX(Quat, float, x, float, y, float, z, float, w)
LUA_NEW_INDEX(Quat, float, x, float, y, float, z, float, w)
*/