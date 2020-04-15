#pragma once

class BufferHandle;
class DescriptorSet;
class RenderSystem;

using SpriteInstanceId = size_t;

class SpriteInstance
{
    friend class RenderSystem;

private:
    SpriteInstanceId id;
    BufferHandle * uniformBuffer;
    DescriptorSet * descriptorSet;
};