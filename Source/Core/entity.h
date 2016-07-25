#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "component.h"
#include "luaserializable.h"
#include "transform.h"

struct Scene;

//The 11th commandment: Thou shalt not inherit from the Entity class; use components like a normal person instead.
struct Entity final : LuaSerializable
{
	std::string name;
	//TODO: Necessary for now
	Scene * scene;
	Transform transform;

	std::vector<Component *> components;
	
	void FireEvent(std::string name, EventArgs args = {});
	static int FireEvent_Lua(lua_State *);

	void PushToLua(lua_State *) override;
	static int LuaIndex(lua_State *);
	static int LuaNewIndex(lua_State *);

	//JSON deserializer
	static Entity * Deserialize(std::string);
};
