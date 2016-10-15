#pragma once

#include "component.h"
#include "entity.h"
#include "sprite.h"

struct SpriteComponent final : Component
{
	bool Component::receiveTicks = false;
	Sprite sprite;

	Deserializable * Deserialize(const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;

	//Component * Create(std::string json) override;
	void OnEvent(std::string name, EventArgs args) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

};
