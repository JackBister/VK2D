#include "Core/Components/spritecomponent.h"

#include <cstring>

#include "json.hpp"

#include "Core/entity.h"
#include "Core/scene.h"
#include "Core/sprite.h"

using nlohmann::json;

COMPONENT_IMPL(SpriteComponent)

SpriteComponent::SpriteComponent() noexcept
{
	receiveTicks = true;
}

Deserializable * SpriteComponent::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(SpriteComponent));
	SpriteComponent * ret = new (mem) SpriteComponent();
	json j = json::parse(str);
	auto img = resourceManager->LoadResource<Image>(j["file"]);
	ret->receiveTicks = true;
	ret->sprite = Sprite(nullptr, img);
	return ret;
}

void SpriteComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "BeginPlay") {
		sprite.transform = &(entity->transform);
	} else if (name == "Tick") {
		entity->scene->SubmitSprite(&sprite);
	}
}

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
