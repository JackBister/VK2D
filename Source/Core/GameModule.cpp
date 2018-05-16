#include "GameModule.h"

#include <nlohmann/json.hpp>
#include <vector>

#include "Core/Components/CameraComponent.h"
#include "Core/entity.h"
#include "Core/Input.h"
#include "Core/physicsworld.h"
#include "Core/Rendering/Renderer.h"
#include "Core/Rendering/Shaders/passthrough.frag.spv.h"
#include "Core/Rendering/Shaders/passthrough-transform.vert.spv.h"
#include "Core/Rendering/Shaders/ui.frag.spv.h"
#include "Core/Rendering/Shaders/ui.vert.spv.h"
#include "Core/scene.h"
#include "Core/UI/EditorSystem.h"
#include "Core/UI/UiSystem.h"
#include "Core/dtime.h"

namespace GameModule {
	struct FrameInfo
	{
		std::vector<CommandBuffer *> commandBuffers;

		FramebufferHandle * framebuffer;

		RenderPassHandle * mainRenderPass;
		uint32_t currentSubpass;

		CommandBuffer * mainCommandBuffer;
		CommandBuffer * preRenderPassCommandBuffer;

		FenceHandle * canStartFrame;
		SemaphoreHandle * framebufferReady;
		SemaphoreHandle * preRenderPassFinished;
		SemaphoreHandle * mainRenderPassFinished;

		CommandBufferAllocator * commandBufferAllocator;
	};

	uint32_t currFrameInfoIdx = 0;
	FrameStage currFrameStage;
	std::vector<Entity *> entities;
	std::vector<FrameInfo> frameInfo;
	Entity * mainCameraEntity;
	CameraComponent * mainCameraComponent;
	PhysicsWorld * physicsWorld;
	Renderer * renderer;
	ResourceManager * resourceManager;
	std::vector<Scene> scenes;

	void AddEntity(Entity * e)
	{
		entities.push_back(e);
	}

	void BeginPlay()
	{
		for (auto& s : scenes) {
			s.BroadcastEvent("BeginPlay");
		}
	}

	void BeginSecondaryCommandContext(CommandBuffer * ctx)
	{
		CommandBuffer::InheritanceInfo inheritanceInfo = {};
		switch (currFrameStage) {
		case GameModule::FrameStage::PRE_RENDERPASS: {
			inheritanceInfo.renderPass = nullptr;
			inheritanceInfo.subpass = 0;
			inheritanceInfo.framebuffer = nullptr;
			inheritanceInfo.commandBufferUsageFlags = 0;
			break;
		}
		case GameModule::FrameStage::MAIN_RENDERPASS: {
			inheritanceInfo.renderPass = frameInfo[currFrameInfoIdx].mainRenderPass;
			inheritanceInfo.subpass = frameInfo[currFrameInfoIdx].currentSubpass;
			inheritanceInfo.framebuffer = frameInfo[currFrameInfoIdx].framebuffer;
			inheritanceInfo.commandBufferUsageFlags = CommandBufferUsageFlagBits::RENDER_PASS_CONTINUE_BIT;
			break;
		}
		}
		ctx->BeginRecording(&inheritanceInfo);
	}

	void CreateResources(std::function<void(ResourceCreationContext&)> fun)
	{
		renderer->CreateResources(fun);
	}

	void CreatePrimitives()
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

		DescriptorSetLayoutHandle * uiPipelineDescriptorSetLayout;

		ShaderModuleHandle * ptvShader;
		ShaderModuleHandle * pfShader;

		ShaderModuleHandle * uivShader;
		ShaderModuleHandle * uifShader;

		VertexInputStateHandle * ptInputState;
		PipelineHandle * ptPipeline;

		VertexInputStateHandle * uiInputState;
		PipelineHandle * uiPipeline;

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
				Create UI shaders
			*/
			uivShader = ctx.CreateShaderModule({
				ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
				shaders_ui_vert_spv_len,
				shaders_ui_vert_spv
				});

			uifShader = ctx.CreateShaderModule({
				ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
				shaders_ui_frag_spv_len,
				shaders_ui_frag_spv
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
			for (size_t i = 0; i < frameInfo.size(); ++i) {
				frameInfo[i].mainRenderPass = ctx.CreateRenderPass(passInfo);
				frameInfo[i].commandBufferAllocator = ctx.CreateCommandBufferAllocator();
				frameInfo[i].canStartFrame = ctx.CreateFence(true);
				frameInfo[i].framebufferReady = ctx.CreateSemaphore();
				frameInfo[i].preRenderPassFinished = ctx.CreateSemaphore();
				frameInfo[i].mainRenderPassFinished = ctx.CreateSemaphore();
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

			ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo ptRasterization{
				CullMode::BACK,
				FrontFace::CLOCKWISE
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
				&ptRasterization,
				ptPipelineDescriptorSetLayout,
				frameInfo[0].mainRenderPass,
				0
				});

			/*
				Create UI shader program
			*/
			ResourceCreationContext::DescriptorSetLayoutCreateInfo::Binding uiBindings[1] = {
				{
					0,
					DescriptorType::COMBINED_IMAGE_SAMPLER,
					ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT
				}
			};

			uiPipelineDescriptorSetLayout = ctx.CreateDescriptorSetLayout({
				1,
				uiBindings
				});

			ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo uiStages[2] = {
				{
					ShaderStageFlagBits::SHADER_STAGE_VERTEX_BIT,
					uivShader,
					"UI Vertex Shader"
				},
				{
					ShaderStageFlagBits::SHADER_STAGE_FRAGMENT_BIT,
					uifShader,
					"UI Fragment Shader"
				}
			};

			ResourceCreationContext::VertexInputStateCreateInfo::VertexBindingDescription uiBinding = {
				0,
				4 * sizeof(float) + 4 * sizeof(uint8_t)
			};

			ResourceCreationContext::VertexInputStateCreateInfo::VertexAttributeDescription uiAttributes[3] = {
				{
					0,
					0,
					VertexComponentType::FLOAT,
					2,
					false,
					0
				},
				{
					0,
					1,
					VertexComponentType::FLOAT,
					2,
					false,
					2 * sizeof(float)
				},
				{
					0,
					2,
					VertexComponentType::UBYTE,
					4,
					true,
					4 * sizeof(float)
				}
			};

			ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineRasterizationStateCreateInfo uiRasterization{
				CullMode::NONE,
				FrontFace::COUNTER_CLOCKWISE
			};

			uiInputState = ctx.CreateVertexInputState({
				1,
				&uiBinding,
				3,
				uiAttributes
				});

			uiPipeline = ctx.CreateGraphicsPipeline({
				2,
				uiStages,
				uiInputState,
				&uiRasterization,	
				uiPipelineDescriptorSetLayout,
				frameInfo[0].mainRenderPass,
				0
				});

			finishedJobs++;
		});

		while (finishedJobs.load() < 1) {
			std::this_thread::sleep_for(1ms);
		}

		auto framebuffers = renderer->CreateBackbuffers(frameInfo[0].mainRenderPass);
		CommandBufferAllocator::CommandBufferCreateInfo ctxCreateInfo = {};
		ctxCreateInfo.level = CommandBufferLevel::PRIMARY;
		for (size_t i = 0; i < frameInfo.size(); ++i) {
			frameInfo[i].framebuffer = framebuffers[i];
			frameInfo[i].mainCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
			frameInfo[i].preRenderPassCommandBuffer = frameInfo[i].commandBufferAllocator->CreateBuffer(ctxCreateInfo);
		}

		resourceManager->AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVerts);
		resourceManager->AddResource("_Primitives/Buffers/QuadEBO.buffer", quadElems);

		resourceManager->AddResource("_Primitives/Shaders/passthrough-transform.vert", ptvShader);
		resourceManager->AddResource("_Primitives/Shaders/passthrough.frag", pfShader);

		resourceManager->AddResource("_Primitives/Shaders/ui.vert", uivShader);
		resourceManager->AddResource("_Primitives/Shaders/ui.frag", uifShader);

		resourceManager->AddResource("_Primitives/VertexInputStates/passthrough-transform.state", ptInputState);
		resourceManager->AddResource("_Primitives/Pipelines/passthrough-transform.pipe", ptPipeline);
		resourceManager->AddResource("_Primitives/DescriptorSetLayouts/passthrough-transform.layout", ptPipelineDescriptorSetLayout);

		resourceManager->AddResource("_Primitives/VertexInputStates/ui.state", uiInputState);
		resourceManager->AddResource("_Primitives/Pipelines/ui.pipe", uiPipeline);
		resourceManager->AddResource("_Primitives/DescriptorSetLayouts/ui.layout", uiPipelineDescriptorSetLayout);
	}

	std::vector<CommandBuffer *> CreateSecondaryCommandContexts()
	{
		CommandBufferAllocator::CommandBufferCreateInfo ci{};
		ci.level = CommandBufferLevel::SECONDARY;
		std::vector<CommandBuffer *> ret;
		for (size_t i = 0; i < frameInfo.size(); ++i) {
			ret.push_back(frameInfo[i].commandBufferAllocator->CreateBuffer(ci));
		}
		return ret;
	}

	void DeserializePhysics(std::string const& str)
	{
		if (physicsWorld == nullptr) {
			physicsWorld = (PhysicsWorld *)Deserializable::DeserializeString(resourceManager, str);
			return;
		}
		auto j = nlohmann::json::parse(str);
		physicsWorld->SetGravity(Vec3(j["gravity"]["x"], j["gravity"]["y"], j["gravity"]["z"]));
	}

	void DestroySecondaryCommandContexts(std::vector<CommandBuffer*> cbs)
	{
		assert(cbs.size() == frameInfo.size());
		for (size_t i = 0; i < frameInfo.size(); ++i) {
			frameInfo[i].commandBufferAllocator->DestroyContext(cbs[i]);
		}
	}

	Entity * GetEntityByIdx(size_t idx)
	{
		if (entities.size() <= idx) {
			return nullptr;
		}
		return entities[idx];
	}

	Entity * GetEntityByName(std::string const& name)
	{
		for (auto& s : scenes) {
			auto ret = s.GetEntityByName(name);
			if (ret != nullptr) {
				return ret;
			}
		}
		return nullptr;
	}

	size_t GetEntityCount()
	{
		return entities.size();
	}

	Entity * GetMainCamera()
	{
		return mainCameraEntity;
	}

	PhysicsWorld * GetPhysicsWorld()
	{
		return physicsWorld;
	}

	IVec2 GetResolution()
	{
		return renderer->GetResolution();
	}

	size_t GetSwapCount()
	{
		return frameInfo.size();
	}

	size_t GetCurrFrame()
	{
		return currFrameInfoIdx;
	}

	void Init(ResourceManager * resMan, Renderer * inRenderer)
	{
		//TODO: This is dumb
		resourceManager = resMan;
		renderer = inRenderer;

		CreatePrimitives();

		Input::Init();
		Time::Start();
		UiSystem::Init(resMan);
	}

	void LoadScene(std::string const& filename)
	{
		scenes.emplace_back(filename, resourceManager, filename);
	}

	void RemoveEntity(Entity * entity)
	{
		for (auto it = entities.begin(); it != entities.end(); ++it) {
			if (*it == entity) {
				delete *it;
				entities.erase(it);
			}
		}
	}

	std::string SerializePhysics()
	{
		return physicsWorld->Serialize();
	}

	void SubmitCommandBuffer(CommandBuffer * ctx)
	{
		frameInfo[currFrameInfoIdx].commandBuffers.push_back(ctx);
	}

	void TakeCameraFocus(Entity * camera)
	{
		mainCameraEntity = camera;
		mainCameraComponent = (CameraComponent *) camera->GetComponent("CameraComponent");
	}

	void Tick()
	{
		currFrameStage = FrameStage::INPUT;
		Input::Frame();
		currFrameStage = FrameStage::TIME;
		Time::Frame();

		//TODO: remove these (hardcoded serialize + dump keybinds)
#ifdef _DEBUG
		if (Input::GetKeyDown(KC_F8)) {
			scenes[0].SerializeToFile("_dump.scene");
		}
		if (Input::GetKeyDown(KC_F9)) {
			scenes[0].LoadFile("main.scene");
			scenes[0].BroadcastEvent("BeginPlay");
		}
		if (Input::GetKeyDown(KC_F10)) {
			for (auto& fi : frameInfo) {
				fi.canStartFrame->Wait(std::numeric_limits<uint64_t>::max());
				fi.preRenderPassCommandBuffer->Reset();
				fi.preRenderPassCommandBuffer->BeginRecording(nullptr);
				fi.preRenderPassCommandBuffer->EndRecording();
				renderer->ExecuteCommandBuffer(fi.preRenderPassCommandBuffer, {}, {}, fi.canStartFrame);
			}
			scenes[0].Unload();
		}
#endif

		currFrameStage = FrameStage::FENCE_WAIT;
		//We use the previous frame's framebufferReady semaphore because in theory we can't know what frame we end up on after calling AcquireNext
		auto prevFrameInfo = currFrameInfoIdx;
		currFrameInfoIdx = renderer->AcquireNextFrameIndex(frameInfo[prevFrameInfo].framebufferReady, nullptr);
		auto& currFrame = frameInfo[currFrameInfoIdx];
		currFrame.canStartFrame->Wait(std::numeric_limits<uint64_t>::max());
		currFrame.commandBufferAllocator->Reset();

		currFrame.commandBuffers.clear();

		currFrameStage = FrameStage::PHYSICS;
		//TODO: substeps
		physicsWorld->world->stepSimulation(Time::GetDeltaTime());
		currFrameStage = FrameStage::TICK;
		for (auto& s : scenes) {
			s.BroadcastEvent("Tick", {{"deltaTime", Time::GetDeltaTime()}});
		}

		currFrameStage = FrameStage::PRE_RENDERPASS;
		SubmittedCamera submittedCamera;
		submittedCamera.projection = mainCameraComponent->GetProjection();
		submittedCamera.view = mainCameraComponent->GetView();
		for (auto& s : scenes) {
			s.BroadcastEvent("PreRenderPass", {{"camera", &submittedCamera}});
		}

		currFrame.preRenderPassCommandBuffer->Reset();
		currFrame.preRenderPassCommandBuffer->BeginRecording(nullptr);

		//TODO: Assumes single thread
		if (currFrame.commandBuffers.size() > 0) {
			currFrame.preRenderPassCommandBuffer->CmdExecuteCommands(std::move(currFrame.commandBuffers));
			currFrame.commandBuffers = std::vector<CommandBuffer *>();
		}

		currFrame.preRenderPassCommandBuffer->EndRecording();

		renderer->ExecuteCommandBuffer(currFrame.preRenderPassCommandBuffer,
			std::vector<SemaphoreHandle *>(),
			{currFrame.preRenderPassFinished});

		CommandBuffer::ClearValue clearValues[] = {
			{
				CommandBuffer::ClearValue::Type::COLOR,
		{
			0.f, 0.f, 1.f, 1.f
		}
			}
		};
		auto res = GetResolution();
		CommandBuffer::RenderPassBeginInfo beginInfo = {
			currFrame.mainRenderPass,
			currFrame.framebuffer,
		{
			{
				0,
				0
			},
			{
				res.x,
				res.y
			}
		},
			1,
			clearValues
		};
		currFrame.mainCommandBuffer->BeginRecording(nullptr);
		currFrameStage = FrameStage::MAIN_RENDERPASS;
		currFrame.mainCommandBuffer->CmdBeginRenderPass(&beginInfo, CommandBuffer::SubpassContents::SECONDARY_COMMAND_BUFFERS);
		currFrame.currentSubpass = 0;
		for (auto& s : scenes) {
			s.BroadcastEvent("MainRenderPass", {{"camera", &submittedCamera}});
		}

		UiSystem::BeginFrame();
		EditorSystem::OnGui();
		UiSystem::EndFrame();

		//TODO: Assumes single thread
		if (currFrame.commandBuffers.size() > 0) {
			currFrame.mainCommandBuffer->CmdExecuteCommands(std::move(currFrame.commandBuffers));
			currFrame.commandBuffers = std::vector<CommandBuffer *>();
		}

		currFrame.mainCommandBuffer->CmdEndRenderPass();
		currFrame.mainCommandBuffer->EndRecording();

		renderer->ExecuteCommandBuffer(currFrame.mainCommandBuffer,
			{frameInfo[prevFrameInfo].framebufferReady, currFrame.preRenderPassFinished},
			{currFrame.mainRenderPassFinished},
			currFrame.canStartFrame);

		renderer->SwapWindow(currFrameInfoIdx, currFrame.mainRenderPassFinished);
	}
};
