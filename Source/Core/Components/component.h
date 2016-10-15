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
	Putting this at the bottom of your component's header with its name as the parameter should add it to the component map.
	This does pollute the header with a function and a static variable which I'm not happy about but not sure how to fix.
	Means every file that includes a header that uses this will waste the size of a pointer of memory. Not the end of the world but still unclean.
	Static would be avoidable with extern + putting another macro in the .cpp file but I want to keep this as easy to use as possible.
*/
#define COMPONENT_IMPL(str) DESERIALIZABLE_IMPL(str)

struct Component : LuaSerializable, Deserializable
{
	/*
		The type name as a string. This is set by the deserializer based on the ComponentMap, meaning classes inheriting from Component do not need to set it
		as long as they are otherwise correctly defined.
	*/
	//std::string type;

	Entity * entity;
	bool isActive = true;
	bool receiveTicks;
	
	virtual ~Component();
	/*
		The Create method will be run on a dummy instance of the class. The Create method must allocate a new instance of the class, deserialize the provided
		json into the new instance, and return the new instance.
		!!!DO NOT RETURN THE THIS POINTER !!! ALWAYS ALLOCATE A NEW INSTANCE!!!
	*/
	//virtual Component * Create(std::string json) = 0;
	virtual void OnEvent(std::string name, EventArgs args = {}) = 0;

	//Every component must have an instance in this map to deserialize properly
	//The values contained is irrelevant, only type matters
	//static std::unordered_map<std::string, Component *>& ComponentMap();
	//Uses ComponentMap to find the type to deserialize to
	//static Component * Deserialize(std::string s);
};
