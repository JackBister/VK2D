#include "Core/Rendering/Accessor.h"

#include <json.hpp>

Accessor::Accessor() noexcept
{
}

Accessor::Accessor(ResourceManager * resMan, const std::string& name) noexcept
{
	printf("[WARNING] Accessor resource constructor called.\n");
}

Deserializable * Accessor::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(Accessor));
	Accessor * ret = new (mem) Accessor();
	nlohmann::json j = nlohmann::json::parse(str);

	for (auto& attrib : j["attribs"]) {
		ComponentType type = static_cast<ComponentType>(attrib["type"].get<size_t>());
		ret->attribs.push_back(Attribute{attrib["attribIndex"].get<size_t>(),
							   attrib["size"].get<size_t>(),
							   type,
							   attrib["normalized"].get<bool>(),
							   attrib["relativeOffset"].get<size_t>()});
	}

	return ret;
}

AccessorRendererData Accessor::GetRendererData() const noexcept
{
	return rendererData;
}
