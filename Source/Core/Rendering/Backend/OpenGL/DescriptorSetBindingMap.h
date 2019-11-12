#pragma once
#ifdef USE_OGL_RENDERER

#include <cstdint>
#include <gl/glew.h>
#include <memory>
#include <unordered_map>

struct OpenGLPipelineLayoutHandle;

/*
 * DescriptorSetBindingMap maps the two dimensional combination of (set, binding) to just a binding, since OpenGL does not(?) support layout(set) in GLSL
 */
class DescriptorSetBindingMap
{
public:
    DescriptorSetBindingMap(OpenGLPipelineLayoutHandle * layout);

	GLuint GetBinding(size_t descriptorSet, uint32_t binding);

private:
    size_t CreateKey(size_t descriptorSet, uint32_t binding);

    std::unordered_map<size_t, GLuint> internalMap;
};

#endif
