#pragma once

#include <ThirdParty/glm/glm/glm.hpp>

enum class PresentMode { IMMEDIATE, FIFO, MAILBOX };

class RendererConfig
{
public:
    glm::uvec2 windowResolution;
    PresentMode presentMode;
};
