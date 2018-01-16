#include "Core/sprite.h"

//#include "rttr/registration.h"
#include "stb_image.h"

/*
RTTR_REGISTRATION
{
	rttr::registration::class_<Sprite>("Sprite")
	.constructor<std::shared_ptr<Image>>()
	.property("isVisible", &Sprite::isVisible);
}
*/

Sprite::Sprite(std::shared_ptr<Image> img) : image(img)
{
}
