#include "sprite.h"

#include "stb_image.h"

#include "lua_cfuncs.h"

#include "sprite.h.generated.h"

Sprite::Sprite(Transform * tfm, std::shared_ptr<Image> img) : image(img), transform(tfm)
{
}
