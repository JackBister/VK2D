#pragma once

#include <glm/glm.hpp>

struct VertexWithColorAndUv {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 uv;
};

struct VertexWithNormal {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 uv;
};
