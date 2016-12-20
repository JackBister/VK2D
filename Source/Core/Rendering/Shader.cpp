#include "Shader.h"

#include "Core/Rendering/render.h"

Shader::Type TypeFromFileName(const std::string& fn)
{
	auto fileExtension = fn.substr(fn.find_last_of('.'), fn.length() - fn.find_last_of('.'));
	if (fileExtension == ".vert") {
		return Shader::Type::VERTEX_SHADER;
	} else if (fileExtension == ".frag") {
		return Shader::Type::FRAGMENT_SHADER;
	} else if (fileExtension == ".geom") {
		return Shader::Type::GEOMETRY_SHADER;
	} else if (fileExtension == ".comp") {
		return Shader::Type::COMPUTE_SHADER;
	} else {
		printf("[ERROR] Loading shader %s: Unknown file format.\n", fn.c_str());
		return Shader::Type::VERTEX_SHADER;
	}
}

Shader::Shader(ResourceManager * _, const std::string& name)
{
	this->name = name;
	this->type = TypeFromFileName(name);
}

Shader::Shader(ResourceManager * _, const std::string& name, const std::vector<char>& input) : src(input.begin(), input.end())
{
	this->name = name;
	this->type = TypeFromFileName(name);
	Render_currentRenderer->AddShader(this);
}

Shader::Type Shader::GetType() const
{
	return type;
}

const std::string & Shader::GetSource() const
{
	return src;
}
