/*
	!!!HERE BE DRAGONS!!!
	This header file is based on http://jhnet.co.uk/articles/cpp_magic
	As the title implies, this is some straight up voodoo magic preprocessor shit.
	To avoid polluting a bunch of files with these crazy macros, only include this header in .cpp files.
	That's the only place where it is useful.
	The important macro here for users is LUA_INDEX, which generates the body of the _LuaIndex function.
	There is some limitation to how many arguments the macro can take, but I haven't hit it yet.
*/

#include <cstring>

/*
	How to use:
	(1) In your header file, ensure that the class inherits from LuaSerializable.
	(2) Create a static method in the class with the following signature: int LuaIndex(lua_State *);
	(3) In your .cpp file, #include this file, then outside of any scope call the LUA_INDEX macro. It will generate a complete
	function definition including signature for you.

	For example, our header file might look like:
	#include "luaserializable.h"
	struct Example : LuaSerializable
	{
		int x;

		void PushToLua(lua_State *);

		static int LuaIndex(lua_State *);
		static int LuaNewIndex(lua_State *);
	}
	and our .cpp file looks like:
	#include "example.h"
	#include "luaindex.h"

	PUSH_TO_LUA(Example)

	LUA_INDEX(Example, int, x);

	int Example::LuaNewIndex(lua_State *);

	Arguments:
	basetype: The type of the object this function is indexing for.
	...: A series of typenames and variable names. The variable name and type should correspond with a variable in the basetype class.
	The only exception is CFunctions, whose type can be given either as CFunction_local or CFunction_global: If global is used, the
	macro will look for the CFunction in the global namespace instead of inside the class.
	If local is used, the macro will compare the index argument to the macro name argument, but it will look in the class for
	name + _Lua - This is because it is presumed that the method with that name will be the C++ implementation while the Lua implementation
	needs to be separate.
	So for example if you have a method named GetKey for a class named Class and you want to use it from Lua, you would write a static GetKey_Lua method that takes
	a lua_state containing a userdata pointer to an instance of that class and whatever arguments GetKey needs and calls GetKey with it, returning
	the results on the Lua stack. Then you would use this macro like LUA_INDEX(Class, CFunction_local, GetKey).

	Example calls:
	LUA_INDEX(Vec3, float, x, float, y, float, z)
	LUA_INDEX(LuaComponent, LuaSerializablePtr, entity, string, type, bool, isActive, bool, receiveTicks, string, file, EventArgs, args)
	LUA_INDEX(Entity, string, name, LuaSerializable, transform, VECTOR(LuaSerializablePtr), components, CFunction_local, FireEvent)
	
	//TODO:
	Limitations:
	//TODO: The following can be fixed if the compiler allows special symbols in macro names:
	!LuaSerializables' type must be given as LuaSerializable or LuaSerializablePtr instead of any derived class. This is unlikely to ever change!
	String type must be given as "string" rather than std::string
	Vectors must be given as VECTOR(type)
*/

#define LUA_INDEX(basetype, ...) INDEX_TOP(basetype) EVAL(DEFER3(MAP) (PUSH, EVAL(__VA_ARGS__))) { lua_pushnil(L); } return 1; }

#define LUA_NEW_INDEX(basetype, ...) NEWINDEX_TOP(basetype) EVAL(DEFER3(MAP) (PULL, EVAL(__VA_ARGS__))) { } return 0; }

#define INDEX_TOP(basetype) int basetype::LuaIndex(lua_State * L) { \
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1)); \
	basetype * ptr = static_cast< basetype * >(*vpp); \
	const char * idx = lua_tostring(L, 2); \
	if (ptr == nullptr || idx == nullptr) { \
		lua_pushnil(L); \
		return 1; \
	}

#define NEWINDEX_TOP(basetype) int basetype::LuaNewIndex(lua_State * L) { \
	 if (lua_gettop(L) != 3) { \
		return 0; \
	} \
	void ** vpp = static_cast<void **>(lua_touserdata(L, 1)); \
	basetype * ptr = static_cast< basetype *>(*vpp); \
	const char * idx = lua_tostring(L, 2); \
	if (ptr == nullptr || idx == nullptr) { \
		return 0; \
	}


#define VECTOR(type) VECTOR_##type

#define STRCMP(name) if (strcmp(idx, #name) == 0) {
#define PUSH(type, name) PUSH_##type(name) } else
#define PUSH_bool(name) STRCMP(name) lua_pushboolean(L, ptr->name);
#define PUSH_CFunction_global(name) STRCMP(name) lua_pushcfunction(L, name);
#define PUSH_CFunction_local(name) STRCMP(name) lua_pushcfunction(L, ptr->name##_Lua);
#define PUSH_EventArgs(name) STRCMP(name) lua_createtable(L, 0, static_cast<int>(ptr->name.size())); \
		for (auto& eap : ptr->name) { \
			eap.second.PushToLua(L); \
			lua_setfield(L, -2, eap.first.c_str()); \
		}
#define PUSH_float(name) STRCMP(name) lua_pushnumber(L, ptr->name);
#define PUSH_LuaSerializable(name) STRCMP(name) ptr->name.PushToLua(L);
#define PUSH_LuaSerializablePtr(name) STRCMP(name) ptr->name->PushToLua(L);
#define PUSH_string(name) STRCMP(name) lua_pushstring(L, ptr->name.c_str());
#define PUSH_VECTOR_LuaSerializable(name) STRCMP(name) lua_createtable(L, 0, static_cast<int>(ptr->name.size())); \
		for (size_t i = 0; i < ptr->name.size(); ++i) { \
			ptr->name[i].PushToLua(L); \
			lua_rawseti(L, -2, i + 1); \
		}
#define PUSH_VECTOR_LuaSerializablePtr(name) STRCMP(name) lua_createtable(L, 0, static_cast<int>(ptr->name.size())); \
		for (size_t i = 0; i < ptr->name.size(); ++i) { \
			ptr->name[i]->PushToLua(L); \
			lua_rawseti(L, -2, i + 1); \
		}

#define PULL(type, name) PULL_##type(name) } else
#define PULL_bool(name) STRCMP(name) if (!lua_isboolean(L, 3)) { \
			return 0; \
		} \
		ptr->name = lua_toboolean(L, 3);
#define PULL_CFunction STRCMP(name) if (!lua_iscfunction(L, 3)) { \
			return 0; \
		} \
		ptr->name = lua_tocfunction(L, 3);
#define PULL_float(name) STRCMP(name) if (!lua_isnumber(L, 3)) { \
			return 0; \
		} \
		ptr->name = static_cast<float>(lua_tonumber(L, 3));
#define PULL_string(name) STRCMP(name) if (!lua_isstring(L, 3)) { \
			return 0; \
		} \
		ptr->name = lua_tostring(L, 3);

#define FIRST(a, ...) a
#define SECOND(a, b, ...) b

#define EMPTY()

#define EVAL(...) EVAL1024(__VA_ARGS__)
#define EVAL1024(...) EVAL512(EVAL512(__VA_ARGS__))
#define EVAL512(...) EVAL256(EVAL256(__VA_ARGS__))
#define EVAL256(...) EVAL128(EVAL128(__VA_ARGS__))
#define EVAL128(...) EVAL64(EVAL64(__VA_ARGS__))
#define EVAL64(...) EVAL32(EVAL32(__VA_ARGS__))
#define EVAL32(...) EVAL16(EVAL16(__VA_ARGS__))
#define EVAL16(...) EVAL8(EVAL8(__VA_ARGS__))
#define EVAL8(...) EVAL4(EVAL4(__VA_ARGS__))
#define EVAL4(...) EVAL2(EVAL2(__VA_ARGS__))
#define EVAL2(...) EVAL1(EVAL1(__VA_ARGS__))
#define EVAL1(...) __VA_ARGS__

#define DEFER1(m) m EMPTY()
#define DEFER2(m) m EMPTY EMPTY()()
#define DEFER3(m) m EMPTY EMPTY EMPTY()()()
#define DEFER4(m) m EMPTY EMPTY EMPTY EMPTY()()()()

#define IS_PROBE(...) SECOND(__VA_ARGS__, 0)
#define PROBE() ~, 1

#define CAT(a,b) a ## b

#define NOT(x) IS_PROBE(CAT(_NOT_, x))
#define _NOT_0 PROBE()

#define BOOL(x) NOT(NOT(x))

#define IF_ELSE(condition) _IF_ELSE(BOOL(condition))
#define _IF_ELSE(condition) CAT(_IF_, condition)

#define _IF_1(...) __VA_ARGS__ _IF_1_ELSE
#define _IF_0(...)             _IF_0_ELSE

#define _IF_1_ELSE(...)
#define _IF_0_ELSE(...) __VA_ARGS__

#define HAS_ARGS(...) BOOL(FIRST(_END_OF_ARGUMENTS_ __VA_ARGS__)())
#define _END_OF_ARGUMENTS_() 0

#define MAP(m, first, second, ...)           \
  m(first, second)                           \
  IF_ELSE(HAS_ARGS(__VA_ARGS__))(    \
    DEFER2(_MAP)()(m, __VA_ARGS__)   \
  )(                                 \
    /* Do nothing, just terminate */ \
  )
#define _MAP() MAP

