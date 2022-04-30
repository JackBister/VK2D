#pragma once

class BufferHandle;
class DescriptorSet;
class RenderSystem;

using CameraInstanceId = size_t;

class CameraInstance
{
public:
    friend class RenderSystem;

    CameraInstanceId GetId() const { return id; }
    DescriptorSet * GetDecsriptorSet() const { return descriptorSet; }
    BufferHandle * GetUniformBuffer() const { return uniformBuffer; }
    bool IsActive() const { return isActive; }

private:
    CameraInstanceId id;
    DescriptorSet * descriptorSet;
    BufferHandle * uniformBuffer;
    bool isActive;
};
