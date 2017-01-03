#pragma once

#include <glm/glm.hpp>

#include "Core/Rendering/RendererData.h"

struct SubmittedCamera
{
	glm::mat4 view;
	glm::mat4 projection;

	FramebufferRendererData renderTarget;

	SubmittedCamera(const glm::mat4& view, const glm::mat4& projection, const FramebufferRendererData& renderTarget)
		: view(view), projection(projection), renderTarget(renderTarget)
	{
	}
};
