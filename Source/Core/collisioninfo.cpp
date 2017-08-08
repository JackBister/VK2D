#include "Core/collisioninfo.h"

#include "Core/entity.h"

int CollisionInfo::LuaIndex(lua_State *L)
{
	char const * const idx = lua_tostring(L, 2);
	if (idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}

	if (strcmp(idx, "other") == 0) {
		other_->PushToLua(L);
	} else if (strcmp(idx, "collisionStart") == 0) {
		lua_pushboolean(L, collision_start_);
	} else if (strcmp(idx, "normals") == 0) {
		if (normals_.size() == 0) {
			lua_pushnil(L);
		} else {
			lua_createtable(L, 0, static_cast<int>(normals_.size()));
			for (size_t i = 0; i < normals_.size(); ++i) {
				lua_pushnumber(L, static_cast<lua_Number>(i + 1));
				normals_[i].PushToLua(L);
				lua_settable(L, -3);
			}
		}
	} else if (strcmp(idx, "points") == 0) {
		if (points_.size() == 0) {
			lua_pushnil(L);
		} else {
			lua_createtable(L, 0, static_cast<int>(points_.size()));
			for (size_t i = 0; i < points_.size(); ++i) {
				lua_pushnumber(L, static_cast<lua_Number>(i + 1));
				points_[i].PushToLua(L);
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
