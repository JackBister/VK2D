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
	.method("GetEntityByName", static_cast<Entity*(Scene::*)(std::string)>(&Scene::GetEntityByName))
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

	frameInfo = std::vector<FrameInfo>(renderer->GetSwapCount());

	renderer->CreateResources([&](ResourceCreationContext& ctx) {
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

		//Create FrameInfo
		for (size_t i = 0; i < this->frameInfo.size(); ++i) {
			frameInfo[i].main_renderpass_ = ctx.CreateRenderPass(passInfo);
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
	});

	while (finishedJobs.load() < 1) {
		std::this_thread::sleep_for(1ms);
	}

	auto framebuffers = renderer->CreateBackbuffers(frameInfo[0].main_renderpass_);
	CommandContextAllocator::CommandBufferCreateInfo ctxCreateInfo = {};
	ctxCreateInfo.level = RenderCommandContextLevel::PRIMARY;
	for (size_t i = 0; i < frameInfo.size(); ++i) {
		frameInfo[i].framebuffer = framebuffers[i];
		frameInfo[i].main_command_context_ = frameInfo[i].commandContextAllocator->CreateContext(ctxCreateInfo);
		frameInfo[i].pre_renderpass_context_ = frameInfo[i].commandContextAllocator->CreateContext(ctxCreateInfo);
	}

	resourceManager->AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVerts);
	resourceManager->AddResource("_Primitives/Buffers/QuadEBO.buffer", quadElems);

	resourceManager->AddResource("_Primitives/Shaders/passthrough-transform.vert", ptvShader);
	resourceManager->AddResource("_Primitives/Shaders/passthrough.frag", pfShader);

	resourceManager->AddResource("_Primitives/VertexInputStates/passthrough-transform.state", ptInputState);
	resourceManager->AddResource("_Primitives/Pipelines/passthrough-transform.pipe", ptPipeline);
	resourceManager->AddResource("_Primitives/DescriptorSetLayouts/passthrough-transform.layout", ptPipelineDescriptorSetLayout);
}

Scene::Scene(std::string const& name, ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue,
	std::string const& serializedScene, Renderer * renderer) noexcept
	: resourceManager(resMan), input(std::move(inputQueue)), renderer(renderer)
{
	this->name = name;

	CreatePrimitives();

	auto const j = nlohmann::json::parse(serializedScene);

	input.DeserializeInPlace(j["input"].dump());

	this->physicsWorld = static_cast<PhysicsWorld *>(Deserializable::DeserializeString(resourceManager, j["physics"].dump()));

	for (auto& je : j["entities"]) {
		Entity * tmp = static_cast<Entity *>(Deserializable::DeserializeString(resourceManager, je.dump()));
		tmp->scene = this;
		this->entities.push_back(tmp);
	}
}

void Scene::BeginSecondaryCommandContext(CommandBuffer * ctx)
{
	CommandBuffer::InheritanceInfo inheritanceInfo = {};
	switch (currFrameStage) {
	case FrameStage::PRE_RENDERPASS: {
		inheritanceInfo.renderPass = nullptr;
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = nullptr;
		inheritanceInfo.commandContextUsageFlags = 0;
		break;
	}
	case FrameStage::MAIN_RENDERPASS: {
		inheritanceInfo.renderPass = frameInfo[currFrameInfoIdx].main_renderpass_;
		inheritanceInfo.subpass = frameInfo[currFrameInfoIdx].current_subpass_;
		inheritanceInfo.framebuffer = frameInfo[currFrameInfoIdx].framebuffer;
		inheritanceInfo.commandContextUsageFlags = CommandContextUsageFlagBits::RENDER_PASS_CONTINUE_BIT;
		break;
	}
	}
	ctx->BeginRecording(&inheritanceInfo);
}

void Scene::CreateResources(std::function<void(ResourceCreationContext&)> fun)
{
	renderer->CreateResources(fun);
}

std::vector<CommandBuffer *> Scene::CreateSecondaryCommandContexts()
{
	CommandContextAllocator::CommandBufferCreateInfo ci {};
	ci.level = RenderCommandContextLevel::SECONDARY;
	std::vector<CommandBuffer *> ret;
	for (size_t i = 0; i < frameInfo.size(); ++i) {
		ret.push_back(frameInfo[i].commandContextAllocator->CreateContext(ci));
	}
	return ret;
}

size_t Scene::GetSwapCount()
{
	return frameInfo.size();
}

size_t Scene::GetCurrFrame()
{
	return currFrameInfoIdx;
}

void Scene::SubmitCamera(CameraComponent * cam) noexcept
{
	frameInfo[currFrameInfoIdx].cameras_to_submit_.emplace_back(cam->GetView(), cam->GetProjection());
}

void Scene::SubmitCommandBuffer(CommandBuffer * ctx)
{
	frameInfo[currFrameInfoIdx].command_buffers_.push_back(ctx);
}

void Scene::Tick() noexcept
{
	currFrameStage = FrameStage::INPUT;
	input.Frame();
	currFrameStage = FrameStage::TIME;
	time.Frame();

	currFrameStage = FrameStage::FENCE_WAIT;
	//We use the previous frame's framebufferReady semaphore because in theory we can't know what frame we end up on after calling AcquireNext
	auto prevFrameInfo = currFrameInfoIdx;
	currFrameInfoIdx = renderer->AcquireNextFrameIndex(frameInfo[prevFrameInfo].framebufferReady, nullptr);
	auto& currFrame = frameInfo[currFrameInfoIdx];
	currFrame.can_start_frame_->Wait(std::numeric_limits<uint64_t>::max());
	currFrame.commandContextAllocator->Reset();

	currFrame.cameras_to_submit_.clear();
	currFrame.command_buffers_.clear();

	currFrameStage = FrameStage::PHYSICS;
	//TODO: substeps
	physicsWorld->world->stepSimulation(time.GetDeltaTime());
	currFrameStage = FrameStage::TICK;
	BroadcastEvent("Tick", { { "deltaTime", time.GetDeltaTime() } });

	currFrameStage = FrameStage::PRE_RENDERPASS;
	for (auto& cc : currFrame.cameras_to_submit_) {
		BroadcastEvent("PreRenderPass", { { "camera", &cc } });
	}

	currFrame.pre_renderpass_context_->Reset();
	currFrame.pre_renderpass_context_->BeginRecording(nullptr);

	//TODO: Assumes single thread
	if (currFrame.command_buffers_.size() > 0) {
		currFrame.pre_renderpass_context_->CmdExecuteCommands(std::move(currFrame.command_buffers_));
		currFrame.command_buffers_ = std::vector<CommandBuffer *>();
	}

	currFrame.pre_renderpass_context_->EndRecording();

	renderer->ExecuteCommandBuffer(currFrame.pre_renderpass_context_,
		std::vector<SemaphoreHandle *>(),
		{currFrame.pre_renderpass_finished_});

	CommandBuffer::ClearValue clearValues[] = {
		{
			CommandBuffer::ClearValue::Type::COLOR,
			{
				0.f, 0.f, 1.f, 1.f
			}
		}
	};
	CommandBuffer::RenderPassBeginInfo beginInfo = {
		currFrame.main_renderpass_,
		currFrame.framebuffer,
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
	currFrame.main_command_context_->BeginRecording(nullptr);
	currFrameStage = FrameStage::MAIN_RENDERPASS;
	currFrame.main_command_context_->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::SECONDARY_COMMAND_BUFFERS);
	currFrame.current_subpass_ = 0;
	for (auto& cc : currFrame.cameras_to_submit_) {
		BroadcastEvent("MainRenderPass", { { "camera", &cc } });
	}

	//TODO: Assumes single thread
	if (currFrame.command_buffers_.size() > 0) {
		currFrame.main_command_context_->CmdExecuteCommands(std::move(currFrame.command_buffers_));
		currFrame.command_buffers_ = std::vector<CommandBuffer *>();
	}

	currFrame.main_command_context_->CmdEndRenderPass();
	currFrame.main_command_context_->EndRecording();

	renderer->ExecuteCommandBuffer(currFrame.main_command_context_,
	{frameInfo[prevFrameInfo].framebufferReady, currFrame.pre_renderpass_finished_},
	{currFrame.main_renderpass_finished_},
	currFrame.can_start_frame_);

	renderer->SwapWindow(currFrameInfoIdx, currFrame.main_renderpass_finished_);
}

Entity * Scene::GetEntityByName(std::string name)
{
	for (auto& ep : entities) {
		if (ep->name == name) {
			return ep;
		}
	}
	return nullptr;
}

void Scene::BroadcastEvent(std::string ename, EventArgs eas)
{
	for (auto& ep : entities) {
		ep->FireEvent(ename, eas);
	}
}
