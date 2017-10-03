#include "Core/Components/spritecomponent.h"

#include <cstring>

#include "json.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Core/entity.h"
#include "Core/Rendering/Renderer.h"
#include "Core/scene.h"
#include "Core/sprite.h"

COMPONENT_IMPL(SpriteComponent)

bool SpriteComponent::has_created_resources_ = false;
SpriteComponent::SpriteResources SpriteComponent::resources_;

SpriteComponent::SpriteComponent() noexcept
{
	receive_ticks_ = false;
}

Deserializable * SpriteComponent::Deserialize(ResourceManager * resourceManager, std::string const& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(SpriteComponent));
	SpriteComponent * ret = new (mem) SpriteComponent();
	auto j = nlohmann::json::parse(str);
	auto img = resourceManager->LoadResource<Image>(j["file"]);
	ret->sprite_ = Sprite(img);
	
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

void SpriteComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "GatherRenderCommands") {
		if (has_created_local_resources_ && sprite_.image_->get_image_handle()) {
			auto img = sprite_.image_->get_image_handle();
			auto camera = (SubmittedCamera *)args["camera"].as_LuaSerializable_;
			auto ctx = Renderer::CreateCommandContext();
			RenderCommandContext::Viewport viewport = {
				0.f,
				0.f,
				800.f,
				600.f,
				0.f,
				1.f
			};
			ctx->CmdSetViewport(0, 1, &viewport);

			RenderCommandContext::Rect2D scissor = {
				{
					0,
					0
				},
				{
					800,
					600
				}
			};
			ctx->CmdSetScissor(0, 1, &scissor);

			ctx->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS, resources_.pipeline);

			ctx->CmdBindIndexBuffer(resources_.ebo, 0, RenderCommandContext::IndexType::UINT32);
			ctx->CmdBindVertexBuffer(resources_.vbo, 0, 0, 8 * sizeof(float));

			//TODO: racy
			cached_MVP_ = camera->projection * camera->view * entity_->transform_.local_to_world();

			ctx->CmdUpdateBuffer(uvs_, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(cached_MVP_));

			ctx->CmdBindDescriptorSet(descriptor_set_);
			ctx->CmdDrawIndexed(6, 1, 0, 0);

			entity_->scene_->SubmitCommandBuffer(std::move(ctx));
		}
	}
}
