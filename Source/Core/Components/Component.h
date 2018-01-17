#pragma once

#include <string>

#include "Core/Deserializable.h"
#include "Core/DllExport.h"
#include "Core/eventarg.h"
#include "Core/HashedString.h"

class Entity;

/*
	This must be put in the .cpp file for the component.
*/
#define COMPONENT_IMPL(str) DESERIALIZABLE_IMPL(str)

class EAPI Component : public Deserializable
{
public:
	virtual ~Component();

	virtual void OnEvent(HashedString name, EventArgs args = {}) = 0;

	Entity * entity;
	bool isActive = true;
	bool receiveTicks;
};
