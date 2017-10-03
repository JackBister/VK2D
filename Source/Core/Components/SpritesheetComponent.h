#pragma once
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"
#include "rttr/rttr_enable.h"

#include "Core/Components/component.h"
#include "Core/sprite.h"

class SpritesheetComponent : public Component
{
	RTTR_ENABLE(Component)
public:
	Deserializable * Deserialize(ResourceManager *, std::string const& str, Allocator& alloc = Allocator::default_allocator) const override;
	glm::vec2 get_frame_size() const;
	void OnEvent(std::string, EventArgs) override;
	void PlayAnimationByName(std::string name);

	PROPERTY(LuaRead)
		Sprite sprite_;

	PROPERTY(LuaReadWrite)
		bool is_flipped_;

	std::vector<float> frame_times_;
private:
	struct SpriteResources
	{
		BufferHandle * ebo;
		BufferHandle * vbo;

		PipelineHandle * pipeline;
	};

	static bool has_created_resources_;
	static SpriteResources resources_;

	glm::mat4 cached_MVP_;
	DescriptorSet * descriptor_set_;
	std::atomic<bool> has_created_local_resources_{ false };
	BufferHandle * uvs_;

	std::unordered_map<std::string, std::vector<size_t>> animations_;
	size_t current_index_ = 0;
	std::string current_named_anim_;
	size_t current_named_anim_index_;
	glm::vec2 frame_size_;
	std::vector<glm::vec2> min_uvs_;
	float time_since_update_ = 0.f;
};
