#pragma once

#include <glm/glm.hpp>

#include "Core/Rendering/RendererData.h"

struct ImageHandle;

struct SubmittedSprite
{
	glm::mat4 transform;
	glm::vec2 minUV, sizeUV;
	ImageHandle * img;

	SubmittedSprite(const glm::mat4& transform, const glm::vec2& minUV, const glm::vec2& sizeUV, ImageHandle * img)
		: transform(transform), minUV(minUV), sizeUV(sizeUV), img(img)
	{
	}
};
