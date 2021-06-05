#pragma once

#include <glm/glm.hpp>

struct Line {
    Line(glm::vec3 from, glm::vec3 to) : from(from), to(to) {}

    glm::vec3 const from;
    glm::vec3 const to;
};
