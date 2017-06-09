#pragma once

#include <vector>

#include "Core/Rendering/SubmittedCamera.h"
#include "Core/Rendering/SubmittedMesh.h"
#include "Core/Rendering/SubmittedSprite.h"

struct ViewDef
{
	std::vector<std::unique_ptr<RenderCommandContext>> commandBuffers;
};