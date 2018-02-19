#include "Core/Components/spritecomponent.h"

#include <cstring>

#include "nlohmann/json.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Core/entity.h"
#include "Core/Rendering/Renderer.h"
#include "Core/scene.h"
#include "Core/sprite.h"

COMPONENT_IMPL(SpriteComponent, &SpriteComponent::s_Deserialize)

bool SpriteComponent::s_hasCreatedResources = false;
SpriteComponent::SpriteResources SpriteComponent::s_resources;

SpriteComponent::~SpriteComponent()
{
	auto capture = frameInfo;
	
	std::vector<CommandBuffer *> preRenderCommandBuffers;
	std::vector<CommandBuffer *> mainCommandBuffers;
	for (auto& fi : frameInfo) {
		preRenderCommandBuffers.push_back(fi.preRenderCommandContext);
		mainCommandBuffers.push_back(fi.mainCommandContext);
	}
	GameModule::DestroySecondaryCommandContexts(preRenderCommandBuffers);
	GameModule::DestroySecondaryCommandContexts(mainCommandBuffers);

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
	auto layout = resourceManager->GetResource<DescriptorSetLayoutHandle>("_Primitives/DescriptorSetLayouts/passthrough-transform.layout");
	ret->file = j["file"].get<std::string>();
	ret->sprite = Sprite(img);
	
	{
		resourceManager->CreateResources([ret, img, layout](ResourceCreationContext& ctx) {
			std::vector<float> fullUv{
				0.f, 0.f,
				1.f, 1.f
			};
			ret->uvs = ctx.CreateBuffer({
				sizeof(glm::mat4) + fullUv.size() * sizeof(float),
				BufferUsageFlags::UNIFORM_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
				DEVICE_LOCAL_BIT
			});
			ctx.BufferSubData(ret->uvs, (uint8_t *)&fullUv[0], sizeof(glm::mat4), fullUv.size() * sizeof(float));

			ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uvDescriptor = {
				ret->uvs,
				0,
				sizeof(glm::mat4) + fullUv.size() * sizeof(float)
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
	if (!s_hasCreatedResources) {
		s_resources.vbo = resourceManager->GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");
		s_resources.ebo = resourceManager->GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");

		s_resources.pipeline = resourceManager->GetResource<PipelineHandle>("_Primitives/Pipelines/passthrough-transform.pipe");
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
	if (name == "BeginPlay") {
		frameInfo = std::vector<FrameInfo>(GameModule::GetSwapCount());
		auto mainCommands = GameModule::CreateSecondaryCommandContexts();
		auto preRenderCommands = GameModule::CreateSecondaryCommandContexts();
		for (size_t i = 0; i < frameInfo.size(); ++i) {
			frameInfo[i].mainCommandContext = std::move(mainCommands[i]);
			frameInfo[i].preRenderCommandContext = std::move(preRenderCommands[i]);
		}
	} else if (name == "PreRenderPass") {
		if (hasCreatedLocalResources && sprite.image->GetImage()) {
			auto camera = (SubmittedCamera *)args["camera"].asPointer;
			frameInfo[GameModule::GetCurrFrame()].pvm = camera->projection * camera->view * entity->transform.GetLocalToWorld();
			auto ctx = frameInfo[GameModule::GetCurrFrame()].preRenderCommandContext;
			GameModule::BeginSecondaryCommandContext(ctx);
			ctx->CmdUpdateBuffer(uvs, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(frameInfo[GameModule::GetCurrFrame()].pvm));
			ctx->EndRecording();
			GameModule::SubmitCommandBuffer(ctx);
		}
	} else if (name == "MainRenderPass") {
		if (hasCreatedLocalResources && sprite.image->GetImage()) {
			auto img = sprite.image->GetImage();
			auto camera = (SubmittedCamera *)args["camera"].asPointer;
			auto ctx = frameInfo[GameModule::GetCurrFrame()].mainCommandContext;
			auto res = GameModule::GetResolution();
			GameModule::BeginSecondaryCommandContext(ctx);
			CommandBuffer::Viewport viewport = {
				0.f,
				0.f,
				res.x,
				res.y,
				0.f,
				1.f
			};
			ctx->CmdSetViewport(0, 1, &viewport);

			CommandBuffer::Rect2D scissor = {
				{
					0,
					0
				},
				{
					res.x,
					res.y
				}
			};
			ctx->CmdSetScissor(0, 1, &scissor);

			ctx->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS, s_resources.pipeline);

			ctx->CmdBindIndexBuffer(s_resources.ebo, 0, CommandBuffer::IndexType::UINT32);
			ctx->CmdBindVertexBuffer(s_resources.vbo, 0, 0, 8 * sizeof(float));

			ctx->CmdBindDescriptorSet(descriptorSet);
			ctx->CmdDrawIndexed(6, 1, 0, 0);

			ctx->EndRecording();
			GameModule::SubmitCommandBuffer(ctx);
		}
	}
}
