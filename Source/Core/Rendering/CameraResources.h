#pragma once

class BufferHandle;
class DescriptorSet;

struct CameraResources {
    DescriptorSet * descriptorSet;
    BufferHandle * uniformBuffer;
};