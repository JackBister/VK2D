#pragma once

class RendererProperties
{
public:
    RendererProperties() : uniformBufferAlignment(0) {}
    RendererProperties(size_t uniformBufferAlignment) : uniformBufferAlignment(uniformBufferAlignment) {}

    /**
     * Gets the uniform buffer alignment requirement for this renderer. All offsets in uniform buffer descriptor sets
     * must be divisible by this.
     */
    inline size_t GetUniformBufferAlignment() const { return uniformBufferAlignment; }

private:
    size_t uniformBufferAlignment;
};
