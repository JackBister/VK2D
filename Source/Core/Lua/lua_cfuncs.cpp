#include "lua_cfuncs.h"

#include <cstring>
#include <string>

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "lua/lua.hpp"

static luaL_Reg argReg[] = {
	{ nullptr, nullptr },
};

void PushCFuncs(lua_State * L)
{
	luaL_newlib(L, argReg);
	lua_setglobal(L, "engine");
}
