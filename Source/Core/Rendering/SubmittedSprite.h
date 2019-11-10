#pragma once

#include <glm/glm.hpp>

struct SubmittedSprite
{
    glm::mat4 localToWorld;
	// TODO: These should go away, the component shouldn't have direct references to GPU types
    BufferHandle * uniforms;
	DescriptorSet * descriptorSet;
};
