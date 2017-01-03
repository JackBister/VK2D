#pragma once

#include <glm/glm.hpp>

#include "Core/Rendering/RendererData.h"

struct SubmittedSprite
{
	glm::mat4 transform;
	glm::vec2 minUV, sizeUV;
	ImageRendererData rendererData;

	SubmittedSprite(const glm::mat4& transform, const glm::vec2& minUV, const glm::vec2& sizeUV, const ImageRendererData& rendererData)
		: transform(transform), minUV(minUV), sizeUV(sizeUV), rendererData(rendererData)
	{
	}
};
