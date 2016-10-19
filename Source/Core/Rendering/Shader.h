#pragma once

#include <iostream>
#include <type_traits>

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
	Shader(const std::string&);
	Shader(const std::string&, std::istream&);

	Type GetType() const;
	const std::string& GetSource() const;

	void * rendererData;

private:
	Type type;
	std::string src;
};

RESOURCE_HEADER(Shader)
