#pragma once

#include "component.h"
#include "entity.h"
#include "sprite.h"

struct SpriteComponent final : Component
{
	Sprite sprite;

	SpriteComponent()
	{
		receiveTicks = false;
	}

	Component * Create(std::string json) override;
	void OnEvent(std::string name, EventArgs args) override;
	
//	void PushToLua(lua_State *) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

};

COMPONENT_HEADER(SpriteComponent);
