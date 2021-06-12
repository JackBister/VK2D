#pragma once

#include <cstdint>

#include <ThirdParty/glm/glm/glm.hpp>

class RenderSystem;

using LightInstanceId = uint32_t;

enum class LightType : uint32_t { POINT = 0 };

class LightInstance
{
    friend class RenderSystem;

private:
    LightInstanceId id;

    bool isActive;
    glm::mat4 localToWorld;

    LightType type;
    glm::vec3 color;
};

struct LightGpuData {
    uint32_t isActive;
    uint32_t _padding[3]; // For alignment to match GLSL
    glm::mat4 localToWorld;
    glm::vec3 color;
    uint8_t _paddding2; // For alignment to match GLSL
};
