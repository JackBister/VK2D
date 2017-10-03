#include "Core/Math/mathtypes.h"

#include "rttr/registration.h"

#include <cstring>

RTTR_REGISTRATION
{
	rttr::registration::class_<Vec3>("Vec3")
	.constructor<>()
	.constructor<float>()
	.constructor<float, float, float>()
	.property("x", &Vec3::get_x, &Vec3::set_x)
	.property("y", &Vec3::get_y, &Vec3::set_y)
	.property("z", &Vec3::get_z, &Vec3::set_z);

	rttr::registration::class_<Quat>("Quat")
	.constructor<>()
	.constructor<float, float, float, float>()
	.property("x", &Quat::get_x, &Quat::set_x)
	.property("y", &Quat::get_y, &Quat::set_y)
	.property("z", &Quat::get_z, &Quat::set_z)
	.property("w", &Quat::get_w, &Quat::set_w);
}

/*
LUA_INDEX(Vec3, float, x, float, y, float, z)
LUA_NEW_INDEX(Vec3, float, x, float, y, float, z)

LUA_INDEX(Quat, float, x, float, y, float, z, float, w)
LUA_NEW_INDEX(Quat, float, x, float, y, float, z, float, w)
*/