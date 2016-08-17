#include "sprite.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "lua_cfuncs.h"

#include "sprite.h.generated.h"

Sprite Sprite::FromFile(Transform * const transform, const char * const fileName, const int forcedComponents)
{
	Sprite ret;
	ret.transform = transform;
	ret.data = stbi_load(fileName, &ret.dimensions.x, &ret.dimensions.y, &ret.components, forcedComponents);
	ret.isVisible = true;
	ret.rendererData = nullptr;
	return ret;
}
