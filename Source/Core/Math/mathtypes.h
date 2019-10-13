#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include "Core/DllExport.h"
#include "Core/Reflect.h"

struct EAPI Mat4 : glm::mat4
{
public:
    using glm::mat4::mat;

	REFLECT()
};

struct EAPI Vec2 : glm::vec2
{
public:
    using glm::vec2::vec;

	REFLECT()
};

struct EAPI IVec2 : glm::ivec2
{
public:
    using glm::ivec2::vec;

	REFLECT()
};

struct EAPI Vec3 : glm::vec3
{
public:
    using glm::vec3::vec;

	REFLECT()
};

struct EAPI Quat : glm::quat
{
public:
    using glm::quat::qua;

	REFLECT()
};
