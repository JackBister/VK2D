#pragma once

#include <vector>

#include "Core/Rendering/SubmittedCamera.h"
#include "Core/Rendering/SubmittedMesh.h"
#include "Core/Rendering/SubmittedSprite.h"

struct ViewDef
{
	//TODO: 1 viewdef = 1 camera (avoid sprite vector copying)
	std::vector<SubmittedCamera> camera;
	std::vector<SubmittedMesh> meshes;
	std::vector<SubmittedSprite> sprites;
};