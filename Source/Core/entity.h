#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Deserializable.h"
#include "Core/eventarg.h"
#include "Core/Lua/luaserializable.h"
#include "Core/transform.h"

#include "Tools/HeaderGenerator/headergenerator.h"

struct Component;
struct Scene;

//The 11th commandment: Thou shalt not inherit from the Entity class; use components like a normal person instead.
struct Entity final : LuaSerializable, Deserializable
{
	PROPERTY(LuaReadWrite)
	std::string name;
	//TODO: Necessary for now
	PROPERTY(LuaRead)
	Scene * scene;
	PROPERTY(LuaReadWrite)
	Transform transform;

	std::vector<Component *> components;

	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc) const override;
	PROPERTY(LuaRead)
	void FireEvent(std::string name, EventArgs args = {});
	PROPERTY(LuaRead)
	Component * GetComponent(std::string type) const;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;
};
