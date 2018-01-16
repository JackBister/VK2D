#pragma once
#include <vector>

#include "Core/entity.h"
#include "Core/Math/mathtypes.h"

class CollisionInfo
{
public:
	Entity * other;
	bool collisionStart = true;
	std::vector<Vec3> normals;
	std::vector<Vec3> points;
};
