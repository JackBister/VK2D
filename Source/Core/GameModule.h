#pragma once

#include "Core/DllExport.h"
#include "Core/Queue.h"

class CameraComponent;
class CommandBuffer;
class Input;
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
	void DeserializeInput(std::string const&);
	void DeserializePhysics(std::string const&);
	void DestroySecondaryCommandContexts(std::vector<CommandBuffer *>);
	//TODO:
	EAPI Input * GetInput();
	PhysicsWorld * GetPhysicsWorld();
	size_t GetSwapCount();
	size_t GetCurrFrame();
	void Init(ResourceManager * resMan, Queue<SDL_Event>::Reader&& inputQueue, Renderer * renderer);
	void LoadDLL(std::string const&);
	void LoadScene(std::string const&);
	std::string SerializeInput();
	std::string SerializePhysics();
	void SubmitCamera(CameraComponent *);
	void SubmitCommandBuffer(CommandBuffer *);
	void Tick();
	void UnloadDLL(std::string const&);
};
