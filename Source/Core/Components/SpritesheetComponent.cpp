#include "Core/Components/spritesheetcomponent.h"

#include "json.hpp"
#include "rttr/registration.h"

#include "Core/entity.h"
#include "Core/scene.h"
#include "Core/sprite.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<SpritesheetComponent>("SpritesheetComponent")
	.property("is_flipped_", &SpritesheetComponent::is_flipped_);
}

COMPONENT_IMPL(SpritesheetComponent)

bool SpritesheetComponent::has_created_resources_ = false;
SpritesheetComponent::SpriteResources SpritesheetComponent::resources_;

Deserializable * SpritesheetComponent::Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator & alloc) const
{
	void * mem = alloc.Allocate(sizeof(SpritesheetComponent));
	SpritesheetComponent * ret = new (mem) SpritesheetComponent();
	auto j = nlohmann::json::parse(str);
	ret->receive_ticks_ = true;
	auto img = resourceManager->LoadResource<Image>(j["file_"]);
	ret->sprite_ = Sprite(img);
	glm::ivec2 frame_size = glm::ivec2(j["frame_size_"]["x"], j["frame_size_"]["y"]);
	ret->frame_size_ = glm::vec2(frame_size.x / (float)img->get_width(), frame_size.y / (float)img->get_height());
	for (uint32_t y = 0; y < img->get_height() / frame_size.y; ++y) {
		for (uint32_t x = 0; x < img->get_width() / frame_size.x; ++x) {
			ret->min_uvs_.push_back(glm::vec2(x * frame_size.x / (float)img->get_width(), y * frame_size.y / (float)img->get_height()));
		}
	}
	if (j.find("frame_times_") != j.end()) {
		ret->frame_times_ = j["frame_times_"].get<std::vector<float>>();
	} else {
		ret->frame_times_.push_back(0.2f);
	}

	if (j.find("animations_") != j.end()) {
		for (const auto& anim : j["animations_"]) {
			ret->animations_[anim["name"]] = anim["frames"].get<std::vector<size_t>>();
		}
	}
	//TODO:
	//ret->play_animation_by_name("Run");

	ret->is_flipped_ = false;

	{
		resourceManager->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([ret, img](ResourceCreationContext& ctx) {
			float full_uv[4] = {
				0.f, 0.f,
				1.f, 1.f
			};
			ret->uvs_ = ctx.CreateBuffer({
				sizeof(glm::mat4) + sizeof(full_uv)
			});
			auto ubo = (float *)ctx.MapBuffer(ret->uvs_, sizeof(glm::mat4), sizeof(full_uv));
			memcpy(ubo, full_uv, sizeof(full_uv));
			ctx.UnmapBuffer(ret->uvs_);

			ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uv_descriptor = {
				ret->uvs_,
				0,
				sizeof(glm::mat4) + sizeof(full_uv)
			};

			ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor img_descriptor = {
				img->get_image_handle()
			};

			ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
				{
					DescriptorType::UNIFORM_BUFFER,
					0,
					uv_descriptor
				},
				{
					DescriptorType::SAMPLER,
					1,
					img_descriptor
				}
			};

			ret->descriptor_set_ = ctx.CreateDescriptorSet({
				2,
				descriptors
			});

			ret->has_created_local_resources_ = true;
		})));
	}
	if (!has_created_resources_) {
		resources_.vbo = resourceManager->GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");
		resources_.ebo = resourceManager->GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");

		resources_.pipeline = resourceManager->GetResource<PipelineHandle>("_Primitives/Pipelines/passthrough-transform.pipe");
	}
	return ret;
}

glm::vec2 SpritesheetComponent::get_frame_size() const
{
	return frame_size_;
}

void SpritesheetComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "BeginPlay") {
		sprite_.min_uv_ = min_uvs_[current_index_];
		sprite_.size_uv_ = frame_size_;
	}
	if (name == "Tick") {
		time_since_update_ += args["deltaTime"].as_float_;
		if (time_since_update_ >= frame_times_[current_index_ % frame_times_.size()]) {
			current_named_anim_index_++;
			if (current_named_anim_index_ == animations_[current_named_anim_].size()) {
				current_named_anim_index_ = 0;
			}
			current_index_ = animations_[current_named_anim_][current_named_anim_index_];
			time_since_update_ = 0.f;
			if (is_flipped_) {
				sprite_.size_uv_.x = -frame_size_.x;
				sprite_.min_uv_ = min_uvs_[current_index_];
				sprite_.min_uv_.x += frame_size_.x;
			} else {
				sprite_.size_uv_ = frame_size_;
				sprite_.min_uv_ = min_uvs_[current_index_];
			}
		}
	}
	if (name == "GatherRenderCommands") {

	}
}

void SpritesheetComponent::PlayAnimationByName(std::string name)
{
	current_named_anim_ = name;
	current_index_ = animations_[name][0];
	current_named_anim_index_ = 0;
}
