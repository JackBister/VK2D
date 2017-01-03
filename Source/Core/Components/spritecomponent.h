#pragma once

#include "Core/Components/component.h"
#include "Core/sprite.h"

struct SpriteComponent final : Component
{
	Sprite sprite;

	SpriteComponent() noexcept;

	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;
	void OnEvent(std::string name, EventArgs args) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

};
