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

	//RenderCommandContext * oneTimeContext;

	frameInfo = std::vector<FrameInfo>(renderer->GetSwapCount());
	//frameInfo = std::vector<FrameInfo>(1);

	renderer->CreateResources([&](ResourceCreationContext& ctx) {
		//resource_manager_->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([&](ResourceCreationContext& ctx) {
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
			renderer->GetBackbufferFormat(),
			RenderPassHandle::AttachmentDescription::LoadOp::CLEAR,
			RenderPassHandle::AttachmentDescription::StoreOp::STORE,
			RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
			RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
			ImageLayout::UNDEFINED,
			ImageLayout::PRESENT_SRC_KHR
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

		ResourceCreationContext::RenderPassCreateInfo passInfo{
			1,
			&attachment,
			1,
			&subpass,
			0,
			nullptr
		};

		for (size_t i = 0; i < this->frameInfo.size(); ++i) {
			frameInfo[i].main_renderpass_ = ctx.CreateRenderPass(passInfo);
		}

		for (size_t i = 0; i < this->frameInfo.size(); ++i) {
			frameInfo[i].commandContextAllocator = ctx.CreateCommandContextAllocator();
			frameInfo[i].can_start_frame_ = ctx.CreateFence(true);
			frameInfo[i].framebufferReady = ctx.CreateSemaphore();
			frameInfo[i].pre_renderpass_finished_ = ctx.CreateSemaphore();
			frameInfo[i].main_renderpass_finished_ = ctx.CreateSemaphore();
		}

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
			frameInfo[0].main_renderpass_,
			0
		});

		finishedJobs++;
		//})));
	});

	while (finishedJobs.load() < 1) {
		std::this_thread::sleep_for(1ms);
	}

	auto framebuffers = renderer->CreateBackbuffers(frameInfo[0].main_renderpass_);
	CommandContextAllocator::RenderCommandContextCreateInfo ctxCreateInfo = {};
	ctxCreateInfo.level = RenderCommandContextLevel::PRIMARY;
	for (size_t i = 0; i < frameInfo.size(); ++i) {
		frameInfo[i].framebuffer = framebuffers[i];
		frameInfo[i].main_command_context_ = frameInfo[i].commandContextAllocator->CreateContext(&ctxCreateInfo);
		frameInfo[i].pre_renderpass_context_ = frameInfo[i].commandContextAllocator->CreateContext(&ctxCreateInfo);
	}

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
	: resource_manager_(resMan), input_(std::move(inputQueue)), render_queue_(std::move(writer)), renderer(renderer)
	//,get_free_command_buffers_(free_command_buffers_.GetReader()), put_free_command_buffers_(free_command_buffers_.GetWriter())
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

void Scene::BeginSecondaryCommandContext(RenderCommandContext * ctx)
{
	RenderCommandContext::InheritanceInfo inheritanceInfo = {};
	switch (currFrameStage) {
	case FrameStage::PRE_RENDERPASS: {
		inheritanceInfo.renderPass = nullptr;
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = nullptr;
		inheritanceInfo.commandContextUsageFlags = 0;
		break;
	}
	case FrameStage::MAIN_RENDERPASS: {
		inheritanceInfo.renderPass = frameInfo[currFrameInfo].main_renderpass_;
		inheritanceInfo.subpass = frameInfo[currFrameInfo].current_subpass_;
		inheritanceInfo.framebuffer = frameInfo[currFrameInfo].framebuffer;
		inheritanceInfo.commandContextUsageFlags = CommandContextUsageFlagBits::RENDER_PASS_CONTINUE_BIT;
		break;
	}
	}
	ctx->BeginRecording(&inheritanceInfo);
}

std::vector<RenderCommandContext *> Scene::CreateSecondaryCommandContexts()
{
	CommandContextAllocator::RenderCommandContextCreateInfo ci {};
	ci.level = RenderCommandContextLevel::SECONDARY;
	std::vector<RenderCommandContext *> ret;
	for (size_t i = 0; i < frameInfo.size(); ++i) {
		ret.push_back(frameInfo[i].commandContextAllocator->CreateContext(&ci));
	}
	return ret;
}

size_t Scene::GetSwapCount()
{
	return frameInfo.size();
}

size_t Scene::GetCurrFrame()
{
	return currFrameInfo;
}

#if 0
RenderCommandContext * Scene::GetSecondaryCommandContext(bool inMainRenderPass)
{
	CommandContextAllocator::RenderCommandContextCreateInfo ci {};
	ci.level = RenderCommandContextLevel::SECONDARY;
	auto ret = frameInfo[currFrameInfo].commandContextAllocator->CreateContext(&ci);
	//auto ret = get_free_command_buffers_.Wait();
	RenderCommandContext::InheritanceInfo inheritanceInfo = {};
	if (inMainRenderPass) {
		inheritanceInfo.renderPass = frameInfo[currFrameInfo].main_renderpass_;
		inheritanceInfo.subpass = frameInfo[currFrameInfo].current_subpass_;
		inheritanceInfo.framebuffer = &Renderer::Backbuffer;
	} else {
		inheritanceInfo.renderPass = nullptr;
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = nullptr;
	}
	ret->BeginRecording(&inheritanceInfo);
	return ret;
}
#endif

void Scene::PushRenderCommand(RenderCommand&& rc) noexcept
{
	render_queue_.Push(std::move(rc));
}

void Scene::SubmitCamera(CameraComponent * cam) noexcept
{
	frameInfo[currFrameInfo].cameras_to_submit_.emplace_back(cam->view(), cam->projection());
}

void Scene::SubmitCommandBuffer(RenderCommandContext * ctx)
{
	frameInfo[currFrameInfo].command_buffers_.push_back(ctx);
}

void Scene::Tick() noexcept
{
	currFrameStage = FrameStage::INPUT;
	input_.Frame();
	currFrameStage = FrameStage::TIME;
	time_.Frame();
	//printf("%f\n", time.GetDeltaTime());

	currFrameStage = FrameStage::FENCE_WAIT;
	//frameInfo[(currFrameInfo + 1) % frameInfo.size()].can_start_frame_->Wait(std::numeric_limits<uint64_t>::max());
	//frameInfo[currFrameInfo].can_start_frame_->Wait(std::numeric_limits<uint64_t>::max());
	frameInfo[currFrameInfo].can_start_frame_->Wait(std::numeric_limits<uint64_t>::max());
	currFrameInfo = renderer->AcquireNextFrameIndex(frameInfo[currFrameInfo].framebufferReady, nullptr);
	frameInfo[currFrameInfo].commandContextAllocator->Reset();

	frameInfo[currFrameInfo].cameras_to_submit_.clear();
	frameInfo[currFrameInfo].command_buffers_.clear();

	currFrameStage = FrameStage::PHYSICS;
	//TODO: substeps
	physics_world_->world_->stepSimulation(time_.get_delta_time());
	currFrameStage = FrameStage::TICK;
	BroadcastEvent("Tick", { { "deltaTime", time_.get_delta_time() } });

	currFrameStage = FrameStage::PRE_RENDERPASS;
	for (auto& cc : frameInfo[currFrameInfo].cameras_to_submit_) {
		BroadcastEvent("PreRenderPass", { { "camera", &cc } });
	}

	frameInfo[currFrameInfo].pre_renderpass_context_->Reset();
	frameInfo[currFrameInfo].pre_renderpass_context_->BeginRecording(nullptr);

	//TODO: Assumes single thread
	if (frameInfo[currFrameInfo].command_buffers_.size() > 0) {
		//ctx->CmdExecuteCommands(command_buffers_.size(), &command_buffers_[0]);
		frameInfo[currFrameInfo].pre_renderpass_context_->CmdExecuteCommands(std::move(frameInfo[currFrameInfo].command_buffers_));
		frameInfo[currFrameInfo].command_buffers_ = std::vector<RenderCommandContext *>();
	}

	frameInfo[currFrameInfo].pre_renderpass_context_->EndRecording();

	renderer->ExecuteCommandContext(frameInfo[currFrameInfo].pre_renderpass_context_,
		std::vector<SemaphoreHandle *>(),
		{frameInfo[currFrameInfo].pre_renderpass_finished_});

#if 0
	render_queue_.Push(RenderCommand(RenderCommand::ExecuteCommandContextParams(
		frameInfo[currFrameInfo].pre_renderpass_context_,
		std::vector<SemaphoreHandle *>(),
		{frameInfo[currFrameInfo].pre_renderpass_finished_})));
#endif

	RenderCommandContext::ClearValue clearValues[] = {
		{
			RenderCommandContext::ClearValue::Type::COLOR,
			{
				0.f, 0.f, 1.f, 1.f
			}
		}
	};
	RenderCommandContext::RenderPassBeginInfo beginInfo = {
		frameInfo[currFrameInfo].main_renderpass_,
		frameInfo[currFrameInfo].framebuffer,
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
	frameInfo[currFrameInfo].main_command_context_->BeginRecording(nullptr);
	currFrameStage = FrameStage::MAIN_RENDERPASS;
	frameInfo[currFrameInfo].main_command_context_->CmdBeginRenderPass(&beginInfo, RenderCommandContext::SubpassContents::SECONDARY_COMMAND_BUFFERS);
	frameInfo[currFrameInfo].current_subpass_ = 0;
	for (auto& cc : frameInfo[currFrameInfo].cameras_to_submit_) {
		BroadcastEvent("MainRenderPass", { { "camera", &cc } });
	}

	//TODO: Assumes single thread
	if (frameInfo[currFrameInfo].command_buffers_.size() > 0) {
		//ctx->CmdExecuteCommands(command_buffers_.size(), &command_buffers_[0]);
		frameInfo[currFrameInfo].main_command_context_->CmdExecuteCommands(std::move(frameInfo[currFrameInfo].command_buffers_));
		frameInfo[currFrameInfo].command_buffers_ = std::vector<RenderCommandContext *>();
	}

	frameInfo[currFrameInfo].main_command_context_->CmdEndRenderPass();
	frameInfo[currFrameInfo].main_command_context_->EndRecording();

	renderer->ExecuteCommandContext(frameInfo[currFrameInfo].main_command_context_,
	{frameInfo[currFrameInfo].framebufferReady, frameInfo[currFrameInfo].pre_renderpass_finished_},
	{frameInfo[currFrameInfo].main_renderpass_finished_});

#if 0
	render_queue_.Push(RenderCommand(RenderCommand::ExecuteCommandContextParams(
		frameInfo[currFrameInfo].main_command_context_,
		{frameInfo[currFrameInfo].framebufferReady, frameInfo[currFrameInfo].pre_renderpass_finished_},
		{frameInfo[currFrameInfo].main_renderpass_finished_})));
#endif
	//TODO:
	//render_queue_.Push(RenderCommand(RenderCommand::SwapWindowParams(currFrameInfo, frameInfo[currFrameInfo].main_renderpass_finished_, frameInfo[currFrameInfo].can_start_frame_)));
	renderer->Swap(currFrameInfo, frameInfo[currFrameInfo].main_renderpass_finished_);


	currFrameInfo = (currFrameInfo + 1) % frameInfo.size();
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
