#pragma once

#include <cstdint>

class RenderSystem;

class CameraHandle
{
    friend class RenderSystem;

private:
    size_t id;
};
