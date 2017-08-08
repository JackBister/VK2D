#pragma once

#include <string>
#include <unordered_map>

#include "Core/Deserializable.h"
#include "Core/eventarg.h"
#include "Core/Lua/luaserializable.h"
#include "Tools/HeaderGenerator/headergenerator.h"

class Entity;

/*
	TODO: Doesn't work
	Put this at the top of your class definition. Necessary for Lua serialization.
*/
#define COMPONENT_BODY() PROPERTY(LuaRead) std::string Component::type;\
						 PROPERTY(LuaReadWrite) Entity * Component::entity_;\
						 PROPERTY(LuaReadWrite) bool Component::is_active_;\
						 PROPERTY(LuaReadWrite) bool Component::receive_ticks_;

/*
	This must be put in the .cpp file for the component.
*/
#define COMPONENT_IMPL(str) DESERIALIZABLE_IMPL(str)

class Component : public LuaSerializable, public Deserializable
{
public:
	virtual ~Component();

	virtual void OnEvent(std::string name, EventArgs args = {}) = 0;

	Entity * entity_;
	bool is_active_ = true;
	bool receive_ticks_;
};
