#pragma once

#include <memory>
#include <vector>

#include "Core/Deserializable.h"
#include "Core/Rendering/RendererData.h"

struct ResourceManager;
struct Shader;

//TODO:
struct Program : Deserializable
{
	friend class Renderer;

	Program() {}
	Program(ResourceManager *, const std::string&);
	Program(ResourceManager *, const std::string& name, std::shared_ptr<Shader>, std::shared_ptr<Shader>);

	Deserializable * Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc = Allocator::default_allocator) const final override;
	ProgramRendererData GetRendererData() const noexcept;

private:
	ProgramRendererData rendererData;
	std::vector<std::shared_ptr<Shader>> shaders;

	// Inherited via Deserializable
	
};

DESERIALIZABLE_IMPL(Program)
