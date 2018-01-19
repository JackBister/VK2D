#pragma once

#include "Core/DllExport.h"
#include "Core/Queue.h"

class CameraComponent;
class CommandBuffer;
class PhysicsWorld;
class ResourceCreationContext;
class ResourceManager;
class Renderer;

namespace GameModule {
	enum class FrameStage
	{
		INPUT,
		TIME,
		FENCE_WAIT,
		PHYSICS,
		TICK,
		PRE_RENDERPASS,
		MAIN_RENDERPASS,
	};

	void BeginPlay();
	void BeginSecondaryCommandContext(CommandBuffer * ctx);
	void CreateResources(std::function<void(ResourceCreationContext&)> fun);
	std::vector<CommandBuffer *> CreateSecondaryCommandContexts();
	void DeserializePhysics(std::string const&);
	void DestroySecondaryCommandContexts(std::vector<CommandBuffer *>);
	//TODO:
	PhysicsWorld * GetPhysicsWorld();
	size_t GetSwapCount();
	size_t GetCurrFrame();
	void Init(ResourceManager * resMan, Renderer * renderer);
	void LoadDLL(std::string const&);
	void LoadScene(std::string const&);
	std::string SerializePhysics();
	void SubmitCamera(CameraComponent *);
	void SubmitCommandBuffer(CommandBuffer *);
	void Tick();
	void UnloadDLL(std::string const&);
};
