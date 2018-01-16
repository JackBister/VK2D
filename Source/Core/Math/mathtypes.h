#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Core/DllExport.h"

struct EAPI Vec3 : glm::vec3
{
public:
	using glm::vec3::tvec3;
};

struct EAPI Quat : glm::quat
{
public:
	using glm::quat::tquat;
};
