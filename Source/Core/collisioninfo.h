#pragma once
#include <vector>

#include "Core/entity.h"

class CollisionInfo
{
public:
	Entity * other;
	bool collisionStart = true;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> points;
};
