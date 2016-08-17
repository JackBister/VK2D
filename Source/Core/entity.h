#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "component.h"
#include "eventarg.h"
#include "luaserializable.h"
#include "transform.h"

#include "Tools/HeaderGenerator/headergenerator.h"

struct Scene;

//The 11th commandment: Thou shalt not inherit from the Entity class; use components like a normal person instead.
struct Entity final : LuaSerializable
{
	PROPERTY(LuaReadWrite)
	std::string name;
	//TODO: Necessary for now
	PROPERTY(LuaRead)
	Scene * scene;
	PROPERTY(LuaReadWrite)
	Transform transform;

	std::vector<Component *> components;

	PROPERTY(LuaRead)
	void FireEvent(std::string name, EventArgs args = {});
	static int FireEvent_Lua(lua_State *);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	//JSON deserializer
	static Entity * Deserialize(std::string);
};
