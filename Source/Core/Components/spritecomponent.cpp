#include "spritecomponent.h"

#include <cstring>

#include "json.hpp"

#include "component.h"
#include "entity.h"
#include "render.h"
#include "sprite.h"

using nlohmann::json;

Component * SpriteComponent::Create(std::string s)
{
	SpriteComponent * ret = new SpriteComponent();
	json j = json::parse(s);
	ret->sprite = Sprite::FromFile(nullptr, j["file"].get<std::string>().c_str());
	return ret;
}

void SpriteComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "BeginPlay") {
		sprite.transform = &(entity->transform);
		Render_currentRenderer->AddSprite(&sprite);
	} else if (name == "EndPlay") {
		Render_currentRenderer->DeleteSprite(&sprite);
	}
}

//PUSH_TO_LUA(SpriteComponent)

int SpriteComponent::LuaIndex(lua_State * L)
{
	void ** spriteCompPtr = static_cast<void **>(lua_touserdata(L, 1));
	SpriteComponent * ptr = static_cast<SpriteComponent *>(*spriteCompPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(idx, "entity") == 0) {
		ptr->entity->PushToLua(L);
	} else if (strcmp(idx, "type") == 0) {
		lua_pushstring(L, ptr->type.c_str());
	} else if(strcmp(idx, "isActive") == 0) {
		lua_pushboolean(L, ptr->isActive);
	} else if (strcmp(idx, "sprite") == 0) {
		ptr->sprite.PushToLua(L);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

//TODO:
int SpriteComponent::LuaNewIndex(lua_State *)
{
	return 0;
}
