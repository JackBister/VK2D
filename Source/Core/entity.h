#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Deserializable.h"
#include "Core/eventarg.h"
#include "Core/Lua/luaserializable.h"
#include "Core/transform.h"

class Component;
class Scene;

class Entity final : public LuaSerializable, public Deserializable
{
	//RTTR_ENABLE(LuaSerializable)
public:
	Deserializable * Deserialize(ResourceManager *, std::string const& str) const override;
	std::string Serialize() const override;
	void FireEvent(std::string name, EventArgs args = {});
	Component * GetComponent(std::string type) const;

	std::string name;
	Scene * scene;
	Transform transform;

	std::vector<std::unique_ptr<Component>> components;
};
