#include "sprite.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "lua_cfuncs.h"

//PUSH_TO_LUA(Sprite)

Sprite Sprite::FromFile(Transform * const transform, const char * const fileName, const int forcedComponents)
{
	Sprite ret;
	ret.transform = transform;
	ret.data = stbi_load(fileName, &ret.dimensions.x, &ret.dimensions.y, &ret.components, forcedComponents);
	ret.isVisible = true;
	ret.rendererData = nullptr;
	return ret;
}

int Sprite::LuaIndex(lua_State * L)
{
	void ** spritePtr = static_cast<void **>(lua_touserdata(L, 1));
	Sprite * ptr = static_cast<Sprite *>(*spritePtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(idx, "transform") == 0) {
		ptr->transform->PushToLua(L);
	} else if (strcmp(idx, "layer")) {
		lua_pushnumber(L, ptr->layer);
	} else if (strcmp(idx, "data")) {
		//TODO:
		printf("[STUB] sprite.cpp LuaIndex push data\n");
		lua_pushnil(L);
	} else if (strcmp(idx, "components")) {
		lua_pushnumber(L, ptr->components);
	} else if (strcmp(idx, "dimensions")) {
		//TODO:
		printf("[STUB] sprite.cpp LuaIndex push dimensions\n");
		lua_pushnil(L);
	} else if (strcmp(idx, "isVisible")) {
		lua_pushboolean(L, ptr->isVisible);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Sprite::LuaNewIndex(lua_State * L)
{
	void ** spritePtr = static_cast<void **>(lua_touserdata(L, 1));
	Sprite * ptr = static_cast<Sprite *>(*spritePtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(idx, "transform") == 0) {
		//TODO:
		printf("[STUB] sprite.cpp LuaNewIndex transform pull\n");
	} else if (strcmp(idx, "layer")) {
		ptr->layer = static_cast<int>(lua_tonumber(L, 3));
	} else if (strcmp(idx, "data")) {
		//TODO:
		printf("[STUB] sprite.cpp LuaNewIndex data pull\n");
	} else if (strcmp(idx, "components")) {
		ptr->components = static_cast<int>(lua_tonumber(L, 3));
	} else if (strcmp(idx, "dimensions")) {
		//TODO:
		printf("[STUB] Sprite push dimensions\n");
		lua_pushnil(L);
	} else if (strcmp(idx, "isVisible")) {
		ptr->isVisible = lua_toboolean(L, 3);
	}
	return 0;
}
