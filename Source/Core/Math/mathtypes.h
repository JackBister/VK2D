#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Core/DllExport.h"

struct EAPI IVec2 : glm::ivec2
{
public:
	using glm::ivec2::tvec2;
};

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
