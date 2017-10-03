#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Deserializable.h"
#include "Core/eventarg.h"
#include "Core/Lua/luaserializable.h"
#include "Core/transform.h"

#include "Tools/HeaderGenerator/headergenerator.h"

class Component;
class Scene;

class Entity final : public LuaSerializable, public Deserializable
{
	RTTR_ENABLE(LuaSerializable)
public:
	Deserializable * Deserialize(ResourceManager *, std::string const& str, Allocator& alloc) const override;
	PROPERTY(LuaRead)
	void FireEvent(std::string name, EventArgs args = {});
	PROPERTY(LuaRead)
	Component * GetComponent(std::string type) const;

	PROPERTY(LuaReadWrite)
	std::string name_;
	//TODO: Necessary for now
	PROPERTY(LuaRead)
	Scene * scene_;
	PROPERTY(LuaReadWrite)
	Transform transform_;

	std::vector<Component *> components_;
};
