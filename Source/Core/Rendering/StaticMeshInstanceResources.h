#pragma once

class BufferHandle;
class DescriptorSet;
class RenderSystem;

class StaticMeshInstanceResources
{
    friend class RenderSystem;

private:
    BufferHandle * uniformBuffer;
    DescriptorSet * descriptorSet;
};