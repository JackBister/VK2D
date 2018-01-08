#include "Core/Components/component.h"

#include "rttr/registration.h"
#include "json.hpp"

RTTR_REGISTRATION
{
	rttr::registration::class_<Component>("Component")
	.property("is_active", &Component::isActive);
}

Component::~Component() {}
