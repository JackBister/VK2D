#include "Core/scene.h"

#include <atomic>
#include <cinttypes>
#include <fstream>
#include <sstream>

#include "json.hpp"

#include "Core/Components/CameraComponent.h"
#include "Core/entity.h"
#include "Core/input.h"
#include "Core/physicsworld.h"
#include "Core/Rendering/Renderer.h"
#include "Core/sprite.h"
#include "Core/transform.h"

#include "Core/scene.h.generated.h"

void Scene::CreatePrimitives()
{
	using namespace std::chrono_literals;
	float const plainQuadVerts[] = {
		//vec3 pos, vec3 color, vec2 texcoord
		-1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top left
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom right
		-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom left
	};

	uint32_t const plainQuadElems[] = {
		0, 1, 2,
		2, 3, 0
	};

	FILE * ptvFile = fopen("shaders/passthrough-transform.vert", "rb");
	fseek(ptvFile, 0, SEEK_END);
	size_t length = ftell(ptvFile);
	fseek(ptvFile, 0, SEEK_SET);
	std::vector<uint8_t> ptvSource(length + 1);
	fread(&ptvSource[0], 1, length, ptvFile);

	FILE * pfFile = fopen("shaders/passthrough.frag", "rb");
	fseek(pfFile, 0, SEEK_END);
	length = ftell(pfFile);
	fseek(pfFile, 0, SEEK_SET);
	std::vector<uint8_t> pfSource(length + 1);
	fread(&pfSource[0], 1, length, pfFile);

	std::atomic_int finishedJobs = 0;
	BufferHandle * quadElems;
	BufferHandle * quadVerts;

	ShaderModuleHandle * ptvShader;
	ShaderModuleHandle * pfShader;

	VertexInputStateHandle * ptInputState;
	PipelineHandle * ptPipeline;

	resource_manager_->PushRenderCommand(RenderCommand(RenderCommand::CreateResourceParams([&](ResourceCreationContext& ctx) {
		/*
			Create quad resources
		*/
		quadElems = ctx.CreateBuffer({
			6 * sizeof(uint32_t)
		});
		quadVerts = ctx.CreateBuffer({
			//4 verts, 8 floats (vec3 pos, vec3 color, vec2 texcoord)
			8 * 4 * sizeof(float)
		});
		auto ebo = (uint32_t *)ctx.MapBuffer(quadElems, 0, 6 * sizeof(uint32_t));
		memcpy(ebo, plainQuadElems, sizeof(plainQuadElems));
		ctx.UnmapBuffer(quadElems);
		auto vbo = (float *)ctx.MapBuffer(quadVerts, 0, 8 * 4 * sizeof(float));
		memcpy(vbo, plainQuadVerts, sizeof(plainQuadVerts));
		ctx.UnmapBuffer(quadVerts);

		/*
			Create passthrough shaders
		*/
		ptvShader = ctx.CreateShaderModule({
			ResourceCreationContext::ShaderModuleCreateInfo::Type::VERTEX_SHADER,
			ptvSource.size() - 1,
			&ptvSource[0]
		});

		pfShader = ctx.CreateShaderModule({
			ResourceCreationContext::ShaderModuleCreateInfo::Type::FRAGMENT_SHADER,
			pfSource.size() - 1,
			&pfSource[0]
		});

		/*
			Create passthrough shader program
		*/
		ResourceCreationContext::GraphicsPipelineCreateInfo::PipelineShaderStageCreateInfo stages[2] = {
			{
				SHADER_STAGE_VERTEX_BIT,
				ptvShader,
				"Passthrough Vertex Shader"
			},
			{
				SHADER_STAGE_FRAGMENT_BIT,
				pfShader,
				"Passthrough Fragment Shader"
			}
		};


		OpenGLResourceContext::VertexInputStateCreateInfo::VertexBindingDescription binding = {
			0,
			8 * sizeof(float)
		};

		OpenGLResourceContext::VertexInputStateCreateInfo::VertexAttributeDescription attributes[3] = {
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
			ptInputState
		});

		RenderPassHandle::AttachmentDescription attachment = {
			0,
			Format::RGBA8,
			RenderPassHandle::AttachmentDescription::LoadOp::LOAD,
			RenderPassHandle::AttachmentDescription::StoreOp::STORE,
			RenderPassHandle::AttachmentDescription::LoadOp::DONT_CARE,
			RenderPassHandle::AttachmentDescription::StoreOp::DONT_CARE,
			ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
			ImageLayout::COLOR_ATTACHMENT_OPTIMAL
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

		finishedJobs++;
	})));

	while (finishedJobs.load() < 1) {
		std::this_thread::sleep_for(1ms);
	}

	resource_manager_->AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVerts);
	resource_manager_->AddResource("_Primitives/Buffers/QuadEBO.buffer", quadElems);

	resource_manager_->AddResource("_Primitives/Shaders/passthrough-transform.vert", ptvShader);
	resource_manager_->AddResource("_Primitives/Shaders/passthrough.frag", pfShader);

	resource_manager_->AddResource("_Primitives/VertexInputStates/passthrough-transform.state", ptInputState);
	resource_manager_->AddResource("_Primitives/Pipelines/passthrough-transform.pipe", ptPipeline);

}

Scene::Scene(std::string const& name, ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue,
			 Queue<RenderCommand>::Writer&& writer, std::string const& serializedScene) noexcept
	: resource_manager_(resMan), input_(std::move(inputQueue)), render_queue_(std::move(writer))
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

	//TODO: substeps
	physics_world_->world_->stepSimulation(time_.get_delta_time());
	BroadcastEvent("Tick", { { "deltaTime", time_.get_delta_time() } });

	auto ctx = Renderer::CreateCommandContext();
	RenderCommandContext::ClearValue clearValues[] = {
		{
			RenderCommandContext::ClearValue::Type::COLOR,
			{
				0.f, 0.f, 0.f, 1.f
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
	ctx->CmdBeginRenderPass(&beginInfo, RenderCommandContext::SubpassContents::SECONDARY_COMMAND_BUFFERS);

	for (auto& cc : cameras_to_submit_) {
		BroadcastEvent("GatherRenderCommands", { { "camera", &cc } });
	}

	//TODO: Assumes single thread
	if (command_buffers_.size() > 0) {
		//ctx->CmdExecuteCommands(command_buffers_.size(), &command_buffers_[0]);
		ctx->CmdExecuteCommands(std::move(command_buffers_));
		command_buffers_ = std::vector<std::unique_ptr<RenderCommandContext>>();
	}

	ctx->CmdSwapWindow();

	ctx->CmdEndRenderPass();

	render_queue_.Push(RenderCommand(RenderCommand::ExecuteCommandContextParams(std::move(ctx))));
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

void Scene::BroadcastEvent(std::string ename, EventArgs eas)
{
	for (auto& ep : entities_) {
		ep->FireEvent(ename, eas);
	}
}
