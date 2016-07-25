#pragma once

#include "glm/glm.hpp"
#include "lua/lua.hpp"

#include "component.h"
#include "luacomponent.h"

void PushCFuncs(lua_State *L);

//TODO: Make classes inheriting from glm:: and LuaSerializable and replace all uses of glm with them
int Quat_Index(lua_State *);
int Quat_NewIndex(lua_State *);
void Quat_Push(lua_State *, glm::quat *);
int Vec2_Index(lua_State *);
int Vec2_NewIndex(lua_State *);
void Vec2_Push(lua_State *, glm::vec2 *);
int Vec3_Index(lua_State *);
int Vec3_NewIndex(lua_State *);
void Vec3_Push(lua_State *, glm::vec3 *);
