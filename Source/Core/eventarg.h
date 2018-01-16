#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "lua.hpp"

#include "Core/DllExport.h"

class EventArg;
class LuaSerializable;

typedef class std::unordered_map<std::string, EventArg> EventArgs;

/*
	An EventArg represents an argument to an OnEvent call.
	It can hold different types of values. The type of the contained value can be read from EventArg::type.
	The special case with this class is when it's holding a string value. In that case, it makes a copy of the passed string value using new.
	It does this because it needs a pointer for the union, but the user shouldn't need to think about making a pointer before creating the eventarg.
	This way, the user can hardcode strings as arguments.

	TODO: Make the union private and use getters? If you're an idiot you can break it right now by assigning to the union.
	TODO: The inclusion of the LuaSerializable and CFunction types inside here is a bit weird, the idea is that the events should be passable to normal
	C++ code which could be annoying if everything being passed around is LuaSerializable or CFunction. However, C++ code can just downcast the LuaSerializable to
	the expected type.
*/

class EAPI EventArg
{
public:
	enum Type
	{
		STRING,
		INT,
		FLOAT,
		DOUBLE,
		LUASERIALIZABLE,
		LUA_CFUNCTION,
		EVENTARGS,
		VECTOR,
	};

	//For string pointer
	~EventArg();
	//Necessary to work in initializer list for unordered_map
	EventArg();
	EventArg(EventArg const&);
	EventArg(EventArg&&);
	EventArg(std::string);
	EventArg(char const *);
	EventArg(int);
	EventArg(float);
	EventArg(double);
	EventArg(LuaSerializable *);
	EventArg(lua_CFunction);
	EventArg(EventArgs);
	EventArg(std::vector<EventArg> const&);

	EventArg& operator=(EventArg const&);
	EventArg& operator=(EventArg&& ea);

	/*
		EventArgs does not push userdata, instead it directly pushes the wrapped value.
	*/
	//TODO: This may still be necessary and usable
	void PushToLua(lua_State *);

	Type type;
	union
	{
		std::string * asString;
		int asInt;
		float asFloat;
		double asDouble;
		LuaSerializable * asLuaSerializable;
		lua_CFunction asLuaCFunction;
		EventArgs * asEventArgs;
		std::vector<EventArg> * asVector;
	};
private:
	void Delete();
};

EventArgs PullEventArgs(lua_State *, int);
