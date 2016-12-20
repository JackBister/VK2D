#pragma once

#include "Core/Resource.h"

struct Shader : Resource
{
	enum Type
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

	void * rendererData;

private:
	Type type;
	std::string src;
};

RESOURCE_HEADER(Shader)
