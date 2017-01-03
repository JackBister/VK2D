#pragma once

#include <memory>
#include <vector>

#include "Core/Rendering/RendererData.h"

struct ResourceManager;
struct Shader;

//TODO:
struct Program
{
	friend class Renderer;

	Program() {}
	Program(ResourceManager *, std::shared_ptr<Shader>, std::shared_ptr<Shader>);

private:
	ProgramRendererData rendererData;
	std::vector<std::shared_ptr<Shader>> shaders;
};
