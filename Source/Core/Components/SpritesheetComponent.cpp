#include "Core/Components/SpritesheetComponent.h"

#include "nlohmann/json.hpp"

#include "Core/entity.h"
#include "Core/scene.h"
#include "Core/sprite.h"

COMPONENT_IMPL(SpritesheetComponent, &SpritesheetComponent::s_Deserialize)

bool SpritesheetComponent::s_hasCreatedResources = false;
SpritesheetComponent::SpriteResources SpritesheetComponent::s_resources;

Deserializable * SpritesheetComponent::s_Deserialize(ResourceManager * resourceManager, std::string const& str)
{
	SpritesheetComponent * ret = new SpritesheetComponent();
	auto j = nlohmann::json::parse(str);
	ret->receiveTicks = true;
	auto img = resourceManager->LoadResource<Image>(j["file_"]);
	ret->sprite_ = Sprite(img);
	glm::ivec2 frame_size = glm::ivec2(j["frameSize"]["x"], j["frameSize"]["y"]);
	ret->frameSize = glm::vec2(frame_size.x / (float)img->GetWidth(), frame_size.y / (float)img->GetHeight());
	for (uint32_t y = 0; y < img->GetHeight() / frame_size.y; ++y) {
		for (uint32_t x = 0; x < img->GetWidth() / frame_size.x; ++x) {
			ret->minUvs.push_back(glm::vec2(x * frame_size.x / (float)img->GetWidth(), y * frame_size.y / (float)img->GetHeight()));
		}
	}
	if (j.find("frameTimes") != j.end()) {
		ret->frameTimes = j["frameTimes"].get<std::vector<float>>();
	} else {
		ret->frameTimes.push_back(0.2f);
	}

	if (j.find("animations") != j.end()) {
		for (const auto& anim : j["animations"]) {
			ret->animations[anim["name"]] = anim["frames"].get<std::vector<size_t>>();
		}
	}
	//TODO:
	//ret->play_animation_by_name("Run");

	ret->is_flipped_ = false;

	{
		resourceManager->CreateResources([ret, img](ResourceCreationContext& ctx) {
			std::vector<float> full_uv{
				0.f, 0.f,
				1.f, 1.f
			};
			ret->uvs = ctx.CreateBuffer({
				sizeof(glm::mat4) + full_uv.size() * sizeof(float),
				BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
				DEVICE_LOCAL_BIT
			});

			ctx.BufferSubData(ret->uvs, (uint8_t *)&full_uv[0], sizeof(glm::mat4), full_uv.size() * sizeof(float));

			ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uv_descriptor = {
				ret->uvs,
				0,
				sizeof(glm::mat4) + full_uv.size() * sizeof(float)
			};

			ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor img_descriptor = {
				img->GetSampler(),
				img->GetImageView()
			};

			ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
				{
					DescriptorType::UNIFORM_BUFFER,
					0,
					uv_descriptor
				},
				{
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					1,
					img_descriptor
				}
			};

			ret->descriptorSet = ctx.CreateDescriptorSet({
				2,
				descriptors
			});

			ret->hasCreatedLocalResources = true;
		});
	}
	if (!s_hasCreatedResources) {
		s_resources.vbo = resourceManager->GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");
		s_resources.ebo = resourceManager->GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");

		s_resources.pipeline = resourceManager->GetResource<PipelineHandle>("_Primitives/Pipelines/passthrough-transform.pipe");
	}
	return ret;
}

std::string SpritesheetComponent::Serialize() const
{
	nlohmann::json j;
	j["type"] = this->type;

	printf("[WARNING] STUB: SpritesheetComponent::Serialize()");

	return j.dump();
}

glm::vec2 SpritesheetComponent::get_frame_size() const
{
	return frameSize;
}

void SpritesheetComponent::OnEvent(HashedString name, EventArgs args)
{
	if (name == "BeginPlay") {
		sprite_.minUv = minUvs[currentIndex];
		sprite_.sizeUv = frameSize;
	}
	if (name == "Tick") {
		timeSinceUpdate += args["deltaTime"].asFloat;
		if (timeSinceUpdate >= frameTimes[currentIndex % frameTimes.size()]) {
			currentNamedAnimIndex++;
			if (currentNamedAnimIndex == animations[currentNamedAnim].size()) {
				currentNamedAnimIndex = 0;
			}
			currentIndex = animations[currentNamedAnim][currentNamedAnimIndex];
			timeSinceUpdate = 0.f;
			if (is_flipped_) {
				sprite_.sizeUv.x = -frameSize.x;
				sprite_.minUv = minUvs[currentIndex];
				sprite_.minUv.x += frameSize.x;
			} else {
				sprite_.sizeUv = frameSize;
				sprite_.minUv = minUvs[currentIndex];
			}
		}
	}
}

void SpritesheetComponent::PlayAnimationByName(std::string name)
{
	currentNamedAnim = name;
	currentIndex = animations[name][0];
	currentNamedAnimIndex = 0;
}
