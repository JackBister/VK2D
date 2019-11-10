#pragma once

#include "Core/Rendering/SubmittedCamera.h"
#include "Core/Rendering/SubmittedSprite.h"

struct SubmittedFrame
{
	std::vector<SubmittedCamera> cameras;
	std::vector<SubmittedSprite> sprites;
};
