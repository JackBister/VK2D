#pragma once

#include "Core/Rendering/RendererData.h"
#include "Core/Resource.h"

struct Shader : Resource
{
	friend class Renderer;

	enum class Type
	{
		VERTEX_SHADER,
		FRAGMENT_SHADER,
		GEOMETRY_SHADER,
		COMPUTE_SHADER
	};

	Shader() = delete;
	Shader(ResourceManager *, const std::string&);
	Shader(ResourceManager *, const std::string&, const std::vector<char>&);

	Type GetType() const;
	const std::string& GetSource() const;

	ShaderRendererData rendererData;

private:
	Type type;
	std::string src;
};

RESOURCE_HEADER(Shader)
