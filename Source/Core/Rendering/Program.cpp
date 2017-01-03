#include "Core/Rendering/Program.h"

#include "Core/Rendering/Shader.h"
#include "Core/ResourceManager.h"

Program::Program(ResourceManager * resMan, std::shared_ptr<Shader> vShader, std::shared_ptr<Shader> fShader)
{
	assert(vShader->GetType() == Shader::Type::VERTEX_SHADER);
	assert(fShader->GetType() == Shader::Type::FRAGMENT_SHADER);
	shaders.push_back(vShader);
	shaders.push_back(fShader);

	RenderCommand rc(RenderCommand::AddProgramParams(this));
	resMan->PushRenderCommand(rc);
}
