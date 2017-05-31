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
#include "Core/Rendering/Framebuffer.h"
#include "Core/Rendering/Mesh.h"
#include "Core/sprite.h"
#include "Core/transform.h"

#include "Core/scene.h.generated.h"

static void CreatePrimitives(ResourceManager * resMan)
{
	using namespace std::chrono_literals;
	const float plainQuadVerts[] = {
		//vec3 pos, vec3 color, vec2 texcoord
		-1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, // Top left
		1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // Top right
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, // Bottom right
		-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // Bottom left
	};

	const uint32_t plainQuadElems[] = {
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

	RenderCommand rc(RenderCommand::CreateResourceParams([&](ResourceCreationContext& ctx) {
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
				3
			},
			{
				0,
				2,
				VertexComponentType::FLOAT,
				2,
				false,
				6
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

		finishedJobs++;
	}));

	resMan->PushRenderCommand(rc);

	while (finishedJobs.load() < 1) {
		std::this_thread::sleep_for(1ms);
	}

	resMan->AddResource("_Primitives/Buffers/QuadVBO.buffer", quadVerts);
	resMan->AddResource("_Primitives/Buffers/QuadEBO.buffer", quadElems);

	resMan->AddResource("_Primitives/Shaders/passthrough-transform.vert", ptvShader);
	resMan->AddResource("_Primitives/Shaders/passthrough.frag", pfShader);

	resMan->AddResource("_Primitives/VertexInputStates/passthrough-transform.state", ptInputState);
	resMan->AddResource("_Primitives/Pipelines/passthrough-transform.pipe", ptPipeline);

}

Scene::Scene(const std::string& name, ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue,
			 Queue<RenderCommand>::Writer&& writer, Queue<ViewDef *>::Reader&& viewDefQueue, const std::string& serializedScene) noexcept
	: resourceManager(resMan), input(std::move(inputQueue)), renderQueue(std::move(writer)), viewDefQueue(std::move(viewDefQueue))
{
	using nlohmann::json;
	this->name = name;

	CreatePrimitives(resMan);

	json j = json::parse(serializedScene);

	input.DeserializeInPlace(j["input"].dump());

	this->physicsWorld = static_cast<PhysicsWorld *>(Deserializable::DeserializeString(resourceManager, j["physics"].dump()));

	for (auto& je : j["entities"]) {
		Entity * tmp = static_cast<Entity *>(Deserializable::DeserializeString(resourceManager, je.dump()));
		tmp->scene = this;
		this->entities.push_back(tmp);
	}

	//TODO: parametrize
	viewDefs = std::vector<ViewDef>(2);
	for (auto& vd : viewDefs) {
		PushRenderCommand(RenderCommand(RenderCommand::DrawViewParams(&vd)));
	}
}

void Scene::EndFrame() noexcept
{
	Maybe<ViewDef *> nextViewDef;
	//TODO: We can either wait for view def to become available or skip
	//Skipping can starve GPU thread, waiting can stall CPU thread
	//Optimally want to just fire DRAW_VIEWs and let GPU thread work out which one to use.
	/*
	if (nextViewDef.index() == 0) {
		camerasToSubmit.clear();
		spritesToSubmit.clear();
		return;
	}
	*/
	do {
		nextViewDef = viewDefQueue.Pop();
	} while (nextViewDef.index() == 0);

	ViewDef * vd = std::get<ViewDef *>(nextViewDef);
	std::swap(vd->camera, camerasToSubmit);
	std::swap(vd->sprites, spritesToSubmit);
	PushRenderCommand(RenderCommand(RenderCommand::DrawViewParams(vd)));
	camerasToSubmit.clear();
	spritesToSubmit.clear();
}

void Scene::PushRenderCommand(const RenderCommand& rc) noexcept
{
	renderQueue.Push(rc);
}

void Scene::SubmitCamera(CameraComponent * cam) noexcept
{
	camerasToSubmit.emplace_back(cam->GetViewMatrix(), cam->GetProjectionMatrix(), cam->GetRenderTarget()->GetRendererData());
}

void Scene::SubmitSprite(Sprite * sprite) noexcept
{
	spritesToSubmit.emplace_back(sprite->transform->GetLocalToWorldSpace(), sprite->minUV, sprite->sizeUV, sprite->image->GetImageHandle());
}

void Scene::Tick() noexcept
{
	input.Frame();
	time.Frame();
	//TODO: substeps
	physicsWorld->world->stepSimulation(time.GetDeltaTime());
	BroadcastEvent("Tick", { { "deltaTime", time.GetDeltaTime() } });

	for (auto& cc : camerasToSubmit) {
		BroadcastEvent("GatherRenderCommands", { { "camera", &cc } });
	}
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
