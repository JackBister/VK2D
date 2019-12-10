#pragma once

class BufferHandle;
class DescriptorSet;
class RenderSystem;

class SpriteInstanceResources
{
    friend class RenderSystem;

private:
    BufferHandle * uniformBuffer;
    DescriptorSet * descriptorSet;
};