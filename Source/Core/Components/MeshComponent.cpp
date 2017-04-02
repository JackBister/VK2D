#include "MeshComponent.h"

#include <json.hpp>

#include "Core/Entity.h"
#include "Core/Scene.h"

#include "Core/Components/MeshComponent.h.generated.h"

Deserializable * MeshComponent::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(MeshComponent));
	auto ret = new (mem) MeshComponent();
	nlohmann::json j = nlohmann::json::parse(str);

	ret->mesh = resourceManager->LoadResourceOrConstruct<Mesh>(j["mesh"]);

	return ret;
}

void MeshComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "Tick") {
		entity->scene->SubmitMesh(mesh.get());
	}
}
