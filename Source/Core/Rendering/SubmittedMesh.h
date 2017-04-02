#pragma once

#include "Core/Rendering/BufferView.h"
#include "Core/Rendering/RendererData.h"

struct SubmittedMesh
{
	AccessorRendererData accessor;
	BufferView view;
	ProgramRendererData program;
};
