#pragma once

#include <string>
#include <unordered_map>

#include "rttr/rttr_enable.h"

#include "Core/Deserializable.h"
#include "Core/eventarg.h"
#include "Core/Lua/luaserializable.h"
#include "Tools/HeaderGenerator/headergenerator.h"

class Entity;

/*
	This must be put in the .cpp file for the component.
*/
#define COMPONENT_IMPL(str) DESERIALIZABLE_IMPL(str)

class Component : public LuaSerializable, public Deserializable
{
	RTTR_ENABLE(LuaSerializable)
public:
	virtual ~Component();

	virtual void OnEvent(std::string name, EventArgs args = {}) = 0;

	Entity * entity;
	bool isActive = true;
	bool receiveTicks;
};
