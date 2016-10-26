#pragma once

#include "component.h"
#include "entity.h"
#include "sprite.h"

struct SpriteComponent final : Component
{
	bool Component::receiveTicks = false;
	Sprite sprite;

	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;
	void OnEvent(std::string name, EventArgs args) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

};
