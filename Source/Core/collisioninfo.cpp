#include "collisioninfo.h"

#include "entity.h"

int CollisionInfo::LuaIndex(lua_State *L)
{
	const char * const idx = lua_tostring(L, 2);
	if (idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}

	if (strcmp(idx, "other") == 0) {
		other->PushToLua(L);
	} else if (strcmp(idx, "collisionStart") == 0) {
		lua_pushboolean(L, collisionStart);
	} else if (strcmp(idx, "normals") == 0) {
		if (normals.size() == 0) {
			lua_pushnil(L);
		} else {
			lua_createtable(L, 0, static_cast<int>(normals.size()));
			for (size_t i = 0; i < normals.size(); ++i) {
				lua_pushnumber(L, i + 1);
				normals[i].PushToLua(L);
				lua_settable(L, -3);
			}
		}
	} else if (strcmp(idx, "points") == 0) {
		if (points.size() == 0) {
			lua_pushnil(L);
		} else {
			lua_createtable(L, 0, static_cast<int>(points.size()));
			for (size_t i = 0; i < points.size(); ++i) {
				lua_pushnumber(L, i + 1);
				points[i].PushToLua(L);
				lua_settable(L, -3);
			}
		}
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int CollisionInfo::LuaNewIndex(lua_State *)
{
	return 0;
}
