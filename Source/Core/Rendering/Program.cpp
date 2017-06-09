#include "Core/Rendering/Program.h"

#include <json.hpp>

#include "Core/Rendering/Shader.h"
#include "Core/ResourceManager.h"

Program::Program(ResourceManager * resMan, const std::string& name)
{
}

Program::Program(ResourceManager * resMan, const std::string& name, std::shared_ptr<Shader> vShader, std::shared_ptr<Shader> fShader)
{
	assert(vShader->GetType() == Shader::Type::VERTEX_SHADER);
	assert(fShader->GetType() == Shader::Type::FRAGMENT_SHADER);
	shaders.push_back(vShader);
	shaders.push_back(fShader);

	resMan->PushRenderCommand(RenderCommand(RenderCommand::AddProgramParams(this)));
}

Deserializable * Program::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(Program));
	Program * ret = new (mem) Program();
	nlohmann::json j = nlohmann::json::parse(str);

	if (j.find("vertshader") != j.end()) {
		ret->shaders.push_back(resourceManager->LoadResource<Shader>(j["vertshader"]));
	}

	if (j.find("fragshader") != j.end()) {
		ret->shaders.push_back(resourceManager->LoadResource<Shader>(j["fragshader"]));
	}

	return ret;
}

ProgramRendererData Program::GetRendererData() const noexcept
{
	return rendererData;
}
