#pragma once

#include <string>
#include <unordered_map>

#include "Core/Deserializable.h"
#include "Core/eventarg.h"
#include "Core/Lua/luaserializable.h"
#include "Tools/HeaderGenerator/headergenerator.h"

struct Entity;

/*
	TODO: Doesn't work
	Put this at the top of your class definition. Necessary for Lua serialization.
*/
#define COMPONENT_BODY() PROPERTY(LuaRead) std::string Component::type;\
						 PROPERTY(LuaReadWrite) Entity * Component::entity;\
						 PROPERTY(LuaReadWrite) bool Component::isActive;\
						 PROPERTY(LuaReadWrite) bool Component::receiveTicks;

/*
	This must be put in the .cpp file for the component.
*/
#define COMPONENT_IMPL(str) DESERIALIZABLE_IMPL(str)

struct Component : LuaSerializable, Deserializable
{
	virtual ~Component();

	virtual void OnEvent(std::string name, EventArgs args = {}) = 0;

	Entity * entity;
	bool isActive = true;
	bool receiveTicks;
};
