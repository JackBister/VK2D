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
public:
	Deserializable * Deserialize(ResourceManager *, std::string const& str, Allocator& alloc) const override;
	PROPERTY(LuaRead)
	void FireEvent(std::string name, EventArgs args = {});
	PROPERTY(LuaRead)
	Component * GetComponent(std::string type) const;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;


	PROPERTY(LuaReadWrite)
	std::string name_;
	//TODO: Necessary for now
	PROPERTY(LuaRead)
	Scene * scene_;
	PROPERTY(LuaReadWrite)
	Transform transform_;

	std::vector<Component *> components_;
};
