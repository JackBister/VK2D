#include "Core/sprite.h"

#include "stb_image.h"

#include "Core/sprite.h.generated.h"

Sprite::Sprite(std::shared_ptr<Image> img) : image_(img)
{
}
