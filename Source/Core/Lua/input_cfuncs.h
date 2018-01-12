#pragma once
#include "lua.hpp"

/*
	This file should ONLY be included in input.cpp
	It contains macros which generate CFunctions for use in Lua which call the Input class' methods.
	If I knew how to do the same thing with templates I would.
*/

#define INPUSH(fncname, type) INPUSH_##type(fncname)
#define INPUSH_KEY(fncname) lua_pushboolean(L, ptr->fncname(strToKeycode[key]))
#define INPUSH_BUTTON(fncname) lua_pushboolean(L, ptr->fncname(key))

//Generates all the getter functions in Input
//Usage: INPUT_GET(method name, KEY or BUTTON)
#define INPUT_GET(fncname, type) if (lua_gettop(L) != 2 || !lua_isuserdata(L, -2) || !lua_isstring(L, -1)) { \
									lua_pushstring(L, "[ERROR] Incorrect arguments to Input:" #fncname ); \
									lua_error(L); \
									return 1; \
								} \
								void ** vpp = static_cast<void **>(lua_touserdata(L, -2)); \
								Input * ptr = static_cast<Input *>(*vpp); \
								char const * key = lua_tostring(L, -1); \
								INPUSH(fncname, type); \
								return 1;

/*
	Generates the AddKeybind and RemoveKeybind functions
	Usage: INPUT_KEYBIND(method name)
*/
#define INPUT_KEYBIND(fncname) if (lua_gettop(L) != 3 || !lua_isuserdata(L, -3) || !lua_isstring(L, -2) || !lua_isstring(L, -1)) { \
										lua_pushstring(L, "[ERROR] Incorrect arguments to Input:" #fncname); \
										lua_error(L); \
										return 1; \
									} \
									void ** vpp = static_cast<void **>(lua_touserdata(L, -3)); \
									Input * ptr = static_cast<Input *>(*vpp); \
									char const * button = lua_tostring(L, -2); \
									char const * kc = lua_tostring(L, -1); \
									ptr->fncname(button, strToKeycode[kc]); \
									return 0;
