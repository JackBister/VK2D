#pragma once

#include "Core/Components/component.h"
#include "Core/sprite.h"

class SpriteComponent final : public Component
{
public:
	SpriteComponent() noexcept;

	Deserializable * Deserialize(ResourceManager *, std::string const& str, Allocator& alloc = Allocator::default_allocator) const override;

	void OnEvent(std::string name, EventArgs args) override;

	Sprite sprite_;

private:
	struct FrameInfo
	{
		RenderCommandContext * preRenderCommandContext;
		RenderCommandContext * mainCommandContext;
		glm::mat4 pvm;
	};

	struct SpriteResources
	{
		BufferHandle * ebo;
		BufferHandle * vbo;

		PipelineHandle * pipeline;
	};

	static bool has_created_resources_;
	static SpriteResources resources_;

	//glm::mat4 cached_MVP_;
	DescriptorSet * descriptor_set_;
	std::atomic<bool> has_created_local_resources_{ false };
	//TODO: FrameInfo?
	BufferHandle * uvs_;
	//RenderCommandContext * command_context_;
	
	std::vector<FrameInfo> frameInfo;
};
