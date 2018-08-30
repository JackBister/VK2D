#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Core/DllExport.h"
#include "Core/Reflect.h"

struct EAPI Mat4 : glm::mat4
{
public:
	using glm::mat4::tmat4x4;

	REFLECT()
};

struct EAPI Vec2 : glm::vec2
{
public:
	using glm::vec2::tvec2;

	REFLECT()
};

struct EAPI IVec2 : glm::ivec2
{
public:
	using glm::ivec2::tvec2;

	REFLECT()
};

struct EAPI Vec3 : glm::vec3
{
public:
	using glm::vec3::tvec3;

	REFLECT()
};

struct EAPI Quat : glm::quat
{
public:
	using glm::quat::tquat;

	REFLECT()
};
