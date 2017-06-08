#include "Core/Components/spritecomponent.h"

#include <cstring>

#include "json.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Core/entity.h"
#include "Core/scene.h"
#include "Core/sprite.h"

using nlohmann::json;

COMPONENT_IMPL(SpriteComponent)

static const float plainQuadVerts[] = {
	//vec3 pos, vec3 color, vec2 texcoord
	-1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top left
	1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
	1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom right
	-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom left
};

static uint32_t plainQuadElems[] = {
	0, 1, 2,
	2, 3, 0
};

std::atomic<bool> SpriteComponent::hasCreatedResources(false);
std::atomic<bool> SpriteComponent::hasFinishedCreatingResources(false);
SpriteComponent::SpriteResources SpriteComponent::resources;

SpriteComponent::SpriteComponent() noexcept
{
	receiveTicks = true;
}

Deserializable * SpriteComponent::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(SpriteComponent));
	SpriteComponent * ret = new (mem) SpriteComponent();
	json j = json::parse(str);
	auto img = resourceManager->LoadResource<Image>(j["file"]);
	ret->receiveTicks = true;
	ret->sprite = Sprite(nullptr, img);
	
	{
		RenderCommand rc(RenderCommand::CreateResourceParams([ret, img](ResourceCreationContext& ctx) {
			ret->uvs = ctx.CreateBuffer({
				sizeof(glm::mat4) + 4 * sizeof(float)
			});
			float muhUVs[4] = {
				0.f, 0.f,
				1.f, 1.f
			};
			auto ubo = (float *)ctx.MapBuffer(ret->uvs, sizeof(glm::mat4), 4 * sizeof(float));
			memcpy(ubo, muhUVs, sizeof(muhUVs));
			ctx.UnmapBuffer(ret->uvs);

			ResourceCreationContext::DescriptorSetCreateInfo::BufferDescriptor uvDescriptor = {
				ret->uvs,
				0,
				sizeof(glm::mat4) + 4 * sizeof(float)
			};

			ResourceCreationContext::DescriptorSetCreateInfo::ImageDescriptor imgDescriptor = {
				img->GetImageHandle()
			};

			ResourceCreationContext::DescriptorSetCreateInfo::Descriptor descriptors[] = {
				{
					DescriptorType::UNIFORM_BUFFER,
					0,
					uvDescriptor
				},
				{
					DescriptorType::SAMPLER,
					1,
					imgDescriptor
				}
			};

			ret->descriptorSet = ctx.CreateDescriptorSet({
				2,
				descriptors
			});

			ret->hasCreatedLocalResources = true;
		}));
		resourceManager->PushRenderCommand(rc);
	}
	if (!hasCreatedResources.exchange(true)) {
		resources.vbo = resourceManager->GetResource<BufferHandle>("_Primitives/Buffers/QuadVBO.buffer");
		resources.ebo = resourceManager->GetResource<BufferHandle>("_Primitives/Buffers/QuadEBO.buffer");

		resources.vertexShader = resourceManager->GetResource<ShaderModuleHandle>("_Primitives/Shaders/passthrough-transform.vert");
		resources.fragmentShader = resourceManager->GetResource<ShaderModuleHandle>("_Primitives/Shaders/passthrough.frag");

		resources.vertexInputState = resourceManager->GetResource<VertexInputStateHandle>("_Primitives/VertexInputStates/passthrough-transform.state");
		resources.pipeline = resourceManager->GetResource<PipelineHandle>("_Primitives/Pipelines/passthrough-transform.pipe");
	}
	return ret;
}

void SpriteComponent::OnEvent(std::string name, EventArgs args)
{
	if (name == "BeginPlay") {
		sprite.transform = &(entity->transform);
	} else if (name == "GatherRenderCommands") {
		if (hasCreatedLocalResources && sprite.image->GetImageHandle()) {
			auto img = sprite.image->GetImageHandle();
			img->DEBUGUINT = 0xdeadbeef;
			auto camera = (SubmittedCamera *)args["camera"].asLuaSerializable;
			auto ctx = Renderer::CreateCommandContext();
			RenderCommandContext::Viewport viewPort = {
				0.f,
				0.f,
				800.f,
				600.f,
				0.f,
				1.f
			};
			ctx->CmdSetViewport(0, 1, &viewPort);

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

			ctx->CmdBindPipeline(RenderPassHandle::PipelineBindPoint::GRAPHICS, resources.pipeline);

			ctx->CmdBindIndexBuffer(resources.ebo, 0, RenderCommandContext::IndexType::UINT32);
			ctx->CmdBindVertexBuffer(resources.vbo, 0, 0, 8 * sizeof(float));

			//TODO: racy
			cachedMVP = camera->projection * camera->view * entity->transform.GetLocalToWorldSpace();

			ctx->CmdUpdateBuffer(uvs, 0, sizeof(glm::mat4), (uint32_t *)glm::value_ptr(cachedMVP));

			ctx->CmdBindDescriptorSet(descriptorSet);

			/*
			ctx->CmdBindUniformBuffer(0, uvs, 0, sizeof(glm::mat4) + 4 * sizeof(float));
			ctx->CmdBindUniformImage(1, sprite.image->GetImageHandle());
			*/

			ctx->CmdDrawIndexed(6, 1, 0, 0);

			entity->scene->SubmitCommandBuffer(ctx);
		}
	} else if (name == "Tick") {
		auto pos = entity->transform.GetPosition();
		//entity->transform.SetPosition(Vec3(pos.x, pos.y + args["deltaTime"].asFloat, pos.z));
	}
}

int SpriteComponent::LuaIndex(lua_State * L)
{
	void ** spriteCompPtr = static_cast<void **>(lua_touserdata(L, 1));
	SpriteComponent * ptr = static_cast<SpriteComponent *>(*spriteCompPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(idx, "entity") == 0) {
		ptr->entity->PushToLua(L);
	} else if (strcmp(idx, "type") == 0) {
		lua_pushstring(L, ptr->type.c_str());
	} else if(strcmp(idx, "isActive") == 0) {
		lua_pushboolean(L, ptr->isActive);
	} else if (strcmp(idx, "sprite") == 0) {
		ptr->sprite.PushToLua(L);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

//TODO:
int SpriteComponent::LuaNewIndex(lua_State *)
{
	return 0;
}
