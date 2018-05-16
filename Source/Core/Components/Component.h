#pragma once

#include <string>

#include "Core/Deserializable.h"
#include "Core/DllExport.h"
#include "Core/eventarg.h"
#include "Core/HashedString.h"
#include "Core/Reflect.h"

class Entity;

/*
	This must be put in the .cpp file for the component.
*/
#define COMPONENT_IMPL(str, fn) DESERIALIZABLE_IMPL(str, fn)

class EAPI Component : public Deserializable
{
public:
	virtual ~Component();

	virtual void OnEvent(HashedString name, EventArgs args = {}) = 0;

	Entity * entity;
	bool isActive = true;
	bool receiveTicks;

	REFLECT()
	REFLECT_INHERITANCE()
};
