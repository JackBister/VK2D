#pragma once

#include "Core/Components/component.h"
#include "Core/sprite.h"

class SpriteComponent final : public Component
{
public:
	SpriteComponent() { receiveTicks = false; }
	~SpriteComponent() override;

	Deserializable * Deserialize(ResourceManager *, std::string const& str) const override;
	std::string Serialize() const override;

	void OnEvent(HashedString name, EventArgs args) override;

	Sprite sprite;

private:
	struct FrameInfo
	{
		CommandBuffer * preRenderCommandContext;
		CommandBuffer * mainCommandContext;
		glm::mat4 pvm;
	};

	struct SpriteResources
	{
		BufferHandle * ebo;
		BufferHandle * vbo;

		PipelineHandle * pipeline;
	};

	static bool s_hasCreatedResources;
	static SpriteResources s_resources;

	DescriptorSet * descriptorSet;
	std::atomic<bool> hasCreatedLocalResources{ false };
	BufferHandle * uvs;
	
	std::vector<FrameInfo> frameInfo;
	std::string file;
};
