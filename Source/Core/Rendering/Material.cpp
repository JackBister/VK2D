#include "Material.h"

#include <json.hpp>

#include "Core/Rendering/Accessor.h"
#include "Core/Rendering/Program.h"

Material::Material() noexcept
{
}

Material::Material(ResourceManager *, const std::string &) noexcept
{
}

Deserializable * Material::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(Material));
	Material * ret = new (mem) Material();
	nlohmann::json j = nlohmann::json::parse(str);

	ret->accessor = resourceManager->LoadResourceOrConstruct<Accessor>(j["accessor"]);
	ret->program = resourceManager->LoadResourceOrConstruct<Program>(j["program"]);

	return ret;
}

std::shared_ptr<Accessor> Material::GetAccessor() const noexcept
{
	return accessor;
}

std::shared_ptr<Program> Material::GetProgram() const noexcept
{
	return program;
}
