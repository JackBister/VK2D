#include "Core/Rendering/Shader.h"

#include "Core/ResourceManager.h"

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

Shader::Shader(ResourceManager * resMan, const std::string& name, const std::vector<uint8_t>& input) : type(TypeFromFileName(name)), src(input.begin(), input.end())
{
	this->name = name;
	this->type = TypeFromFileName(name);
	resMan->PushRenderCommand(RenderCommand(RenderCommand::AddShaderParams(this)));
}

Shader::Shader(ResourceManager * resMan, const std::string&, Type type, const std::string& src)
	: type(type), src(src)
{
	this->name = name;
	resMan->PushRenderCommand(RenderCommand(RenderCommand::AddShaderParams(this)));
}

Shader::Type Shader::GetType() const
{
	return type;
}

const std::string & Shader::GetSource() const
{
	return src;
}
