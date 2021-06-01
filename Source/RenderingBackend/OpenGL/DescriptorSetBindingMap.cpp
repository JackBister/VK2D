#ifdef USE_OGL_RENDERER
#include "DescriptorSetBindingMap.h"

#include "OpenGLContextStructs.h"

DescriptorSetBindingMap::DescriptorSetBindingMap(OpenGLPipelineLayoutHandle * layout)
{
    GLuint bufferBindingOffset = 0;
    GLuint samplerBindingOffset = 0;
    for (size_t i = 0; i < layout->descriptorLayouts.size(); ++i) {
        auto nativeDescriptorLayout =
            (OpenGLDescriptorSetLayoutHandle *)layout->descriptorLayouts[i];
        for (auto const & binding : nativeDescriptorLayout->bindings) {
            auto key = CreateKey(i, binding.binding);
            if (binding.descriptorType == DescriptorType::COMBINED_IMAGE_SAMPLER) {
                internalMap[key] = samplerBindingOffset;
                samplerBindingOffset++;
            } else if (binding.descriptorType == DescriptorType::UNIFORM_BUFFER) {
                internalMap[key] = bufferBindingOffset;
                bufferBindingOffset++;
            }
        }
    }
}

GLuint DescriptorSetBindingMap::GetBinding(size_t descriptorSet, uint32_t binding)
{
    auto key = CreateKey(descriptorSet, binding);
    return internalMap[key];
}

size_t DescriptorSetBindingMap::CreateKey(size_t descriptorSet, uint32_t binding)
{
    return descriptorSet * 1000000000 + binding;
}

#endif
