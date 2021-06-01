#pragma once

#include <glm/glm.hpp>

enum class PresentMode { IMMEDIATE, FIFO, MAILBOX };

class RendererConfig
{
public:
    glm::ivec2 windowResolution;
    PresentMode presentMode;
};
