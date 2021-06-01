#pragma once
#include <vector>

#include <glm/glm.hpp>

#include "Core/EntityPtr.h"

class CollisionInfo
{
public:
    EntityPtr other;
    bool collisionStart = true;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> points;
};
