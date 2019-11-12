#pragma once

#include <glm/glm.hpp>

struct SubmittedCamera
{
	glm::mat4 view;
	glm::mat4 projection;

    BufferHandle * uniforms;
    DescriptorSet * descriptorSet;
};
