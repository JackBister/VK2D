#pragma once

class BufferHandle;
class DescriptorSet;
class RenderSystem;

using CameraInstanceId = size_t;

class CameraInstance
{
    friend class RenderSystem;

private:
    CameraInstanceId id;
    DescriptorSet * descriptorSet;
    BufferHandle * uniformBuffer;
    bool isActive;
};
