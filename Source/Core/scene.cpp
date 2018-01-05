#include "Core/scene.h"

#include <atomic>
#include <cinttypes>
#include <fstream>
#include <sstream>

#include "json.hpp"
#include "rttr/registration.h"

#include "Core/Components/CameraComponent.h"
#include "Core/entity.h"
#include "Core/input.h"
#include "Core/physicsworld.h"
#include "Core/Rendering/Renderer.h"
#include "Core/Rendering/Shaders/passthrough.frag.spv.h"
#include "Core/Rendering/Shaders/passthrough-transform.vert.spv.h"
#include "Core/sprite.h"
#include "Core/transform.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Scene>("Scene")
	//.method("GetEntityByName", &Scene::GetEntityByName)
	.method("GetEntityByName", static_cast<Entity*(Scene::*)(std::string)>(&Scene::GetEntityByName))
	//.method("GetEntityByName", static_cast<Entity*(Scene::*)(char const *)>(&Scene::GetEntityByName))
	.method("BroadcastEvent", &Scene::BroadcastEvent);
}

void Scene::CreatePrimitives()
{
	using namespace std::chrono_literals;
	std::vector<float> const plainQuadVerts{
		//vec3 pos, vec3 color, vec2 texcoord
		-1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top left
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom right
		-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom left
	};

	std::vector<uint32_t> const plainQuadElems = {
		0, 1, 2,
		2, 3, 0
	};

	std::atomic_int finishedJobs = 0;
	BufferHandle * quadElems;
	BufferHandle * quadVerts;

	DescriptorSetLayoutHandle * ptPipelineDescriptorSetLayout;

	ShaderModuleHandle * ptvShader;
	ShaderModuleHandle * pfShader;

	VertexInputStateHandle * ptInputState;
	PipelineHandle * ptPipeline;

	std::unique_ptr<RenderCommandContext> oneTimeContext;

	resource_manager_->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([&](ResourceCreationContext& ctx) {
		/*
			Create quad resources
		*/
		quadElems = ctx.CreateBuffer({
			6 * sizeof(uint32_t),
			BufferUsageFlags::INDEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
			DEVICE_LOCAL_BIT
		});
		quadVerts = ctx.CreateBuffer({
			//4 verts, 8 floats (vec3 pos, vec3 color, vec2 texcoord)
			8 * 4 * sizeof(float),
			BufferUsageFlags::VERTEX_BUFFER_BIT | BufferUsageFlags::TRANSFER_DST_BIT,
			DEVICE_LOCAL_BIT
		});
		ctx.BufferSubData(quadElems, (uint8_t *)&plainQuadElems[0], 0, plainQuadElems.size() * sizeof(uint32_t));

		ctx.BufferSubData(quadVerts, (uint8_t *)&plainQuadVerts[0], 0, plainQuadVerts.size() * sizeof(float));

		/*
			Create passthrough shaders
		*/
		ptvShader = ctx.CreateShaderModule({
			ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
			shaders_passthrough_transform_vert_spv_len,
			shaders_passthrough_transform_vert_spv
		});

		pfShader = ctx.CreateShaderModule({
			ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
			shaders_passthrough_frag_spv_len,
			shaders_passthrough_frag_spv
		});

		/*
			Create main renderpass
		*/
		RenderPassHandle::AttachmentDescription attachment = {
			0,
			Renderer::Backbuffer.format,
			RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
			RenderPassHandle::AttachmentDescription::StoreOp::STORE,
			RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
			RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
			ImageLayout::UNDEFINED,
			ImageLayout::TRANSFER_SRC_OPTIMAL
			//ImageLayout::PRESENT_SRC_KHR
		};

		RenderPassHandle::AttachmentReference reference = {
			0,
			ImageLayout::COLOR_ATTACHMENT_OPTIMAL
		};

		RenderPassHandle::SubpassDescription subpass = {
			RenderPassHandle::PipelineBindPoint::GRAPHICS,
			0,
			nullptr,
			1,
			&reference,
			nullptr,
			nullptr,
			0,
			nullptr
		};

		this->main_renderpass_ = ctx.CreateRenderPass({
			1,
			&attachment,
			1,
			&subpass,
			0,
			nullptr
		});

		ResourceCreationContext::RenderCommandContextCreateInfo ctxCreateInfo = {};
		ctxCreateInfo.level = RenderCommandContextLevel::PRIMARY;
		main_command_context_ = ctx.CreateCommandContext(&ctxCreateInfo);
		pre_renderpass_context_ = ctx.CreateCommandContext(&ctxCreateInfo);
		oneTimeContext = ctx.CreateCommandContext(&ctxCreateInfo);

		main_renderpass_finished_ = ctx.CreateSemaphore();
		pre_renderpass_finished_ = ctx.CreateSemaphore();
		swap_finished_ = ctx.CreateSemaphore();

		/*
			Create passthrough shader program
		*/

		ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding uniformBindings[2] = {
			{
				0,
				DescriptorType::UNIFORM_BUFFER,
				ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT | ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
			},
			{
				1,
				DescriptorType::COMBINED_IMAGE_SAMPLER,
				ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
			}
		};

		//DescriptorSetLayout
		ptPipelineDescriptorSetLayout = ctx.CreateDescriptorSetLayout({
			2,
			uniformBindings
		});

		ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo stages[2] = {
			{
				ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
				ptvShader,
				"Passthrough Vertex Shader"
			},
			{
				ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
				pfShader,
				"Passthrough Fragment Shader"
			}
		};

		ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription binding = {
			0,
			8 * sizeof(float)
		};

		ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription attributes[3] = {
			{
				0,
				0,
				VertexComponentType::FLOAT,
				3,
				false,
				0
			},
			{
				0,
				1,
				VertexComponentType::FLOAT,
				3,
				false,
				3 * sizeof(float)
			},
			{
				0,
				2,
				VertexComponentType::FLOAT,
				2,
				false,
				6 * sizeof(float)
			}
		};

		ptInputState = ctx.CreateVertexInputState({
			1,
			&binding,
			3,
			attributes
		});

		ptPipeline = ctx.CreateGraphicsPipeline({
			2,
			stages,
			ptInputState,
			ptPipelineDescriptorSetLayout,
			this->main_renderpass_,
			0
		});

		finishedJobs++;
	})));

	while (finishedJobs.load() < 1) {
		std::this_thread::sleep_for(1ms);
	}

	oneTimeContext->BeginRecording(nullptr);
	oneTimeContext->EndRecording();
	render_queue_.Push(RenderCommand(RenderCommand::ExecuteCommandContextParams(oneTimeContext.get(), nullptr, swap_finished_)));

	resource_manager_->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([&](ResourceCreationContext& ctx) {
		//TODO: Destroy oneTimeContext
	})));

	resource_manager_->AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVerts);
	resource_manager_->AddResource("_Primitives/Buffers/QuadEBO.buffer", quadElems);

	resource_manager_->AddResource("_Primitives/Shaders/passthrough-transform.vert", ptvShader);
	resource_manager_->AddResource("_Primitives/Shaders/passthrough.frag", pfShader);

	resource_manager_->AddResource("_Primitives/VertexInputStates/passthrough-transform.state", ptInputState);
	resource_manager_->AddResource("_Primitives/Pipelines/passthrough-transform.pipe", ptPipeline);
	resource_manager_->AddResource("_Primitives/DescriptorSetLayouts/passthrough-transform.layout", ptPipelineDescriptorSetLayout);
}

Scene::Scene(std::string const& name, ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue,
			 Queue<RenderCommand>::Writer&& writer, std::string const& serializedScene, Renderer * renderer) noexcept
	: resource_manager_(resMan), input_(std::move(inputQueue)), render_queue_(std::move(writer)), renderer_(renderer),
	get_free_command_buffers_(free_command_buffers_.GetReader()), put_free_command_buffers_(free_command_buffers_.GetWriter())
{
	this->name = name;

	CreatePrimitives();

	auto const j = nlohmann::json::parse(serializedScene);

	input_.DeserializeInPlace(j["input"].dump());

	this->physics_world_ = static_cast<PhysicsWorld *>(Deserializable::DeserializeString(resource_manager_, j["physics"].dump()));

	for (auto& je : j["entities"]) {
		Entity * tmp = static_cast<Entity *>(Deserializable::DeserializeString(resource_manager_, je.dump()));
		tmp->scene_ = this;
		this->entities_.push_back(tmp);
	}
}

void Scene::EndFrame() noexcept
{
}

std::unique_ptr<RenderCommandContext> Scene::GetSecondaryCommandContext(bool inMainRenderPass)
{
	auto ret = get_free_command_buffers_.Wait();
	RenderCommandContext::InheritanceInfo inheritanceInfo = {};
	if (inMainRenderPass) {
		inheritanceInfo.renderPass = main_renderpass_;
		inheritanceInfo.subpass = current_subpass_;
		inheritanceInfo.framebuffer = &Renderer::Backbuffer;
	} else {
		inheritanceInfo.renderPass = nullptr;
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = nullptr;
	}
	ret->BeginRecording(&inheritanceInfo);
	return ret;
}

void Scene::PushRenderCommand(RenderCommand&& rc) noexcept
{
	render_queue_.Push(std::move(rc));
}

void Scene::SubmitCamera(CameraComponent * cam) noexcept
{
	cameras_to_submit_.emplace_back(cam->view(), cam->projection());
}

void Scene::SubmitCommandBuffer(std::unique_ptr<RenderCommandContext>&& ctx)
{
	command_buffers_.push_back(std::move(ctx));
}

void Scene::Tick() noexcept
{
	input_.Frame();
	time_.Frame();
	//printf("%f\n", time.GetDeltaTime());

	cameras_to_submit_.clear();
	command_buffers_.clear();

	resource_manager_->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([&](ResourceCreationContext& ctx) {
		//TODO:
		for (int i = 0; i < 6; ++i) {
			ResourceCreationContext::RenderCommandContextCreateInfo ci = {};
			ci.level = RenderCommandContextLevel::SECONDARY;
			this->put_free_command_buffers_.Push(ctx.CreateCommandContext(&ci));
		}
	})));

	//TODO: substeps
	physics_world_->world_->stepSimulation(time_.get_delta_time());
	BroadcastEvent("Tick", { { "deltaTime", time_.get_delta_time() } });

	for (auto& cc : cameras_to_submit_) {
		BroadcastEvent("PreRenderPass", { { "camera", &cc } });
	}

	pre_renderpass_context_->Reset();
	pre_renderpass_context_->BeginRecording(nullptr);

	//TODO: Assumes single thread
	if (command_buffers_.size() > 0) {
		//ctx->CmdExecuteCommands(command_buffers_.size(), &command_buffers_[0]);
		pre_renderpass_context_->CmdExecuteCommands(std::move(command_buffers_));
		command_buffers_ = std::vector<std::unique_ptr<RenderCommandContext>>();
	}

	pre_renderpass_context_->EndRecording();

	render_queue_.Push(RenderCommand(RenderCommand::ExecuteCommandContextParams(pre_renderpass_context_.get(), swap_finished_, pre_renderpass_finished_)));

	RenderCommandContext::ClearValue clearValues[] = {
		{
			RenderCommandContext::ClearValue::Type::COLOR,
			{
				0.f, 0.f, 1.f, 1.f
			}
		}
	};
	RenderCommandContext::RenderPassBeginInfo beginInfo = {
		main_renderpass_,
		&Renderer::Backbuffer,
		{
			{
				0,
				0
			},
			{
				//TODO:
				800,
				600
			}
		},
		1,
		clearValues
	};
	main_command_context_->BeginRecording(nullptr);
	main_command_context_->CmdBeginRenderPass(&beginInfo, RenderCommandContext::SubpassContents::SECONDARY_COMMAND_BUFFERS);
	current_subpass_ = 0;
	for (auto& cc : cameras_to_submit_) {
		BroadcastEvent("MainRenderPass", { { "camera", &cc } });
	}

	//TODO: Assumes single thread
	if (command_buffers_.size() > 0) {
		//ctx->CmdExecuteCommands(command_buffers_.size(), &command_buffers_[0]);
		main_command_context_->CmdExecuteCommands(std::move(command_buffers_));
		command_buffers_ = std::vector<std::unique_ptr<RenderCommandContext>>();
	}

	main_command_context_->CmdSwapWindow();
	main_command_context_->CmdEndRenderPass();
	main_command_context_->EndRecording();

	render_queue_.Push(RenderCommand(RenderCommand::ExecuteCommandContextParams(main_command_context_.get(), pre_renderpass_finished_, main_renderpass_finished_)));
	render_queue_.Push(RenderCommand(RenderCommand::SwapWindowParams(main_renderpass_finished_, swap_finished_)));
}

Entity * Scene::GetEntityByName(std::string name)
{
	for (auto& ep : entities_) {
		if (ep->name_ == name) {
			return ep;
		}
	}
	return nullptr;
}

#if 0
Entity * Scene::GetEntityByName(char const * c)
{
	return GetEntityByName(std::string(c));
}
#endif

void Scene::BroadcastEvent(std::string ename, EventArgs eas)
{
	for (auto& ep : entities_) {
		ep->FireEvent(ename, eas);
	}
}
