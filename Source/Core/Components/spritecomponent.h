#pragma once

#include "Core/Components/component.h"
#include "Core/sprite.h"

struct SpriteComponent final : Component
{
	SpriteComponent() noexcept;

	Deserializable * Deserialize(ResourceManager *, const std::string& str, Allocator& alloc = Allocator::default_allocator) const override;
	void OnEvent(std::string name, EventArgs args) override;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	Sprite sprite;

private:
	BufferHandle * uvs;
	DescriptorSet * descriptorSet;
	glm::mat4 cachedMVP;
	std::atomic<bool> hasCreatedLocalResources;

	struct SpriteResources
	{
		BufferHandle * ebo;
		BufferHandle * vbo;

		PipelineHandle * pipeline;
		RenderPassHandle * renderPass;

		ShaderModuleHandle * vertexShader;
		ShaderModuleHandle * fragmentShader;

		VertexInputStateHandle * vertexInputState;
	};
	static std::atomic<bool> hasCreatedResources;
	static std::atomic<bool> hasFinishedCreatingResources;
	static SpriteResources resources;
};
