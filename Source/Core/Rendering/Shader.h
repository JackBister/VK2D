#pragma once
#include <vector>

#include "Core/Rendering/RendererData.h"
#include "Core/Resource.h"

struct ResourceManager;

struct Shader : Resource
{
	friend struct Renderer;

	enum class Type
	{
		FRAGMENT_SHADER = 35632,
		VERTEX_SHADER = 35633,
		GEOMETRY_SHADER = 36313,
		COMPUTE_SHADER = 37305
	};

	Shader() = delete;
	Shader(ResourceManager *, const std::string&);
	Shader(ResourceManager *, const std::string&, const std::vector<uint8_t>&);
	Shader(ResourceManager *, const std::string&, Type, const std::string& src);

	Type GetType() const;
	const std::string& GetSource() const;

	ShaderRendererData rendererData;

private:
	Type type;
	std::string src;
};
