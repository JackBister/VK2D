#pragma once

#include <memory>

#include "Core/Components/component.h"
#include "Core/Rendering/Mesh.h"

struct MeshComponent : Component
{
	PROPERTY(LuaReadWrite)
	std::string Component::type;
	int LuaIndex(lua_State *) final override;
	int LuaNewIndex(lua_State *) final override;
	Deserializable * Deserialize(ResourceManager * resourceManager, const std::string & str, Allocator & alloc = Allocator::default_allocator) const final override;
	void OnEvent(std::string name, EventArgs args = {}) final override;

private:
	std::shared_ptr<Mesh> mesh;
};

COMPONENT_IMPL(MeshComponent)
