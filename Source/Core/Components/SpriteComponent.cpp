#include "Core/Components/SpriteComponent.h"

#include <cstring>

#include "nlohmann/json.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Core/entity.h"
#include "Core/Rendering/Backend/Renderer.h"
#include "Core/Rendering/SubmittedSprite.h"
#include "Core/scene.h"
#include "Core/sprite.h"

COMPONENT_IMPL(SpriteComponent, &SpriteComponent::s_Deserialize)

SpriteComponent::~SpriteComponent()
{
	GameModule::CreateResources([this](ResourceCreationContext& ctx) {
		ctx.DestroyBuffer(uvs);
		ctx.DestroyDescriptorSet(descriptorSet);
	});
}

Deserializable * SpriteComponent::s_Deserialize(ResourceManager * resourceManager, std::string const& str)
{
	SpriteComponent * ret = new SpriteComponent();
	auto j = nlohmann::json::parse(str);
	auto img = resourceManager->LoadResource<Image>(j["file"]);
	auto layout = resourceManager->GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/spritePt.layout");
	ret->file = j["file"].get<std::string>();
	ret->sprite = Sprite(img);
	
	{
		resourceManager->CreateResources([ret, img, layout](ResourceCreationContext& ctx) {
			ret->uvs = ctx.CreateBuffer({
				sizeof(glm::mat4),
				BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
				DEVICE_LOCAL_BIT
			});

			ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uvDescriptor = {
				ret->uvs,
				0,
				sizeof(glm::mat4)
			};

			ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imgDescriptor = {
				img->GetSampler(),
				img->GetImageView()
			};

			ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
				{
					DescriptorType::UNIFORM_BUFFER,
					0,
					uvDescriptor
				},
				{
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					1,
					imgDescriptor
				}
			};

			ret->descriptorSet = ctx.CreateDescriptorSet({
				2,
				descriptors,
				layout
			});

			ret->hasCreatedLocalResources = true;
		});
	}
	return ret;
}

std::string SpriteComponent::Serialize() const
{
	nlohmann::json j;
	j["type"] = this->type;
	j["file"] = file;
	return j.dump();
}

void SpriteComponent::OnEvent(HashedString name, EventArgs args)
{
	if (name == "Tick") {
		if (hasCreatedLocalResources && sprite.image->GetImage()) {
            SubmittedSprite submittedSprite;
            submittedSprite.descriptorSet = descriptorSet;
            submittedSprite.localToWorld = entity->transform.GetLocalToWorld();
            submittedSprite.uniforms = uvs;
            GameModule::SubmitSprite(submittedSprite);
		}
	}
}
