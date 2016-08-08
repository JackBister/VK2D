#include "mathtypes.h"

#include "luaindex.h"

//PUSH_TO_LUA(Vec3)
LUA_INDEX(Vec3, float, x, float, y, float, z)
LUA_NEW_INDEX(Vec3, float, x, float, y, float, z)

//PUSH_TO_LUA(Quat)
LUA_INDEX(Quat, float, x, float, y, float, z, float, w)
LUA_NEW_INDEX(Quat, float, x, float, y, float, z, float, w)
