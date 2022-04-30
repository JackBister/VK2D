#pragma once

#include <ThirdParty/glm/glm/glm.hpp>

constexpr size_t MAX_VERTEX_WEIGHTS = 4;

struct ParticleVertex {
    glm::vec2 pos;
    glm::vec2 uv;
};

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

struct VertexWithSkinning {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec2 uv;
    uint32_t bones[MAX_VERTEX_WEIGHTS];
    float weights[MAX_VERTEX_WEIGHTS];
};
