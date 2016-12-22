#include "Core/sprite.h"

#include "stb_image.h"

#include "Core/sprite.h.generated.h"

Sprite::Sprite(Transform * tfm, std::shared_ptr<Image> img) : transform(tfm), image(img)
{
}
