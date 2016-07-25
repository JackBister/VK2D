#include "lua_cfuncs.h"

#include <cstring>
#include <string>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "lua/lua.hpp"

using namespace std;

static luaL_Reg argReg[] = {
	{nullptr, nullptr},
};

void PushCFuncs(lua_State * L)
{
}

int Quat_Index(lua_State * L)
{
	void ** qPtr = static_cast<void **>(lua_touserdata(L, 1));
	glm::quat * ptr = static_cast<glm::quat *>(*qPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	//TODO: r, g, b, etc.
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

int Quat_NewIndex(lua_State * L)
{
	void ** qPtr = static_cast<void **>(lua_touserdata(L, 1));
	glm::quat * ptr = static_cast<glm::quat *>(*qPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		return 0;
	}
	//TODO: r, g, b, etc.
	if (strcmp(idx, "x") == 0) {
		ptr->x = static_cast<float>(lua_tonumber(L, 3));
	} else if (strcmp(idx, "y") == 0) {
		ptr->y = static_cast<float>(lua_tonumber(L, 3));
	} else if (strcmp(idx, "z") == 0) {
		ptr->z = static_cast<float>(lua_tonumber(L, 3));
	} else if (strcmp(idx, "w") == 0) {
		ptr->w = static_cast<float>(lua_tonumber(L, 3));
	}
	return 0;
}

void Quat_Push(lua_State *L, glm::quat * q)
{
	void ** vpp = static_cast<void **>(lua_newuserdata(L, sizeof(void *)));
	*vpp = q;
	if (luaL_getmetatable(L, "_QMT") == LUA_TNIL) {
		lua_pop(L, 1);
		luaL_newmetatable(L, "_QMT");
		lua_pushcfunction(L, Quat_Index);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, Quat_NewIndex);
		lua_setfield(L, -2, "__newindex");
	}
	lua_setmetatable(L, -2);
}

int Vec2_Index(lua_State * L)
{
	void ** v2Ptr = static_cast<void **>(lua_touserdata(L, 1));
	glm::vec2 * ptr = static_cast<glm::vec2 *>(*v2Ptr);
	const char * idx = lua_tostring(L, 2);
	if (v2Ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	//TODO: r, g, b, etc.
	if (strcmp(idx, "x") == 0) {
		lua_pushnumber(L, ptr->x);
	} else if (strcmp(idx, "y") == 0) {
		lua_pushnumber(L, ptr->y);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Vec2_NewIndex(lua_State * L)
{
	void ** v2Ptr = static_cast<void **>(lua_touserdata(L, 1));
	glm::vec2 * ptr = static_cast<glm::vec2 *>(*v2Ptr);
	const char * idx = lua_tostring(L, 2);
	if (v2Ptr == nullptr || idx == nullptr) {
		return 0;
	}
	//TODO: r, g, b, etc.
	if (strcmp(idx, "x") == 0) {
		ptr->x = static_cast<float>(lua_tonumber(L, 3));
	} else if (strcmp(idx, "y") == 0) {
		ptr->y = static_cast<float>(lua_tonumber(L, 3));
	}
	return 0;
}

void Vec2_Push(lua_State * L, glm::vec2 * v2)
{
	void ** vpp = static_cast<void **>(lua_newuserdata(L, sizeof(void *)));
	*vpp = v2;
	if (luaL_getmetatable(L, "_V2MT") == LUA_TNIL) {
		lua_pop(L, 1);
		luaL_newmetatable(L, "_V2MT");
		lua_pushcfunction(L, Vec2_Index);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, Vec2_NewIndex);
		lua_setfield(L, -2, "__newindex");
	}
	lua_setmetatable(L, -2);

}

int Vec3_Index(lua_State * L)
{
	void ** v3Ptr = static_cast<void **>(lua_touserdata(L, 1));
	glm::vec3 * ptr = static_cast<glm::vec3 *>(*v3Ptr);
	const char * idx = lua_tostring(L, 2);
	if (v3Ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	//TODO: r, g, b, etc.
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

int Vec3_NewIndex(lua_State * L)
{
	void ** v3Ptr = static_cast<void **>(lua_touserdata(L, 1));
	glm::vec3 * ptr = static_cast<glm::vec3 *>(*v3Ptr);
	const char * idx = lua_tostring(L, 2);
	if (v3Ptr == nullptr || idx == nullptr) {
		return 0;
	}
	//TODO: r, g, b, etc.
	if (strcmp(idx, "x") == 0) {
		ptr->x = static_cast<float>(lua_tonumber(L, 3));
	} else if (strcmp(idx, "y") == 0) {
		ptr->y = static_cast<float>(lua_tonumber(L, 3));
	} else if (strcmp(idx, "z") == 0) {
		ptr->z = static_cast<float>(lua_tonumber(L, 3));
	}
	return 0;
}

void Vec3_Push(lua_State * L, glm::vec3 * v3)
{
	void ** vpp = static_cast<void **>(lua_newuserdata(L, sizeof(void *)));
	*vpp = v3;
	if (luaL_getmetatable(L, "_V3MT") == LUA_TNIL) {
		lua_pop(L, 1);
		luaL_newmetatable(L, "_V3MT");
		lua_pushcfunction(L, Vec3_Index);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, Vec3_NewIndex);
		lua_setfield(L, -2, "__newindex");
	}
	lua_setmetatable(L, -2);
}
