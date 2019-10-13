#pragma once

#include <functional>

#include "Core/DllExport.h"
#include "Core/Math/mathtypes.h"
#include "Core/Queue.h"

class CameraComponent;
class CommandBuffer;
class Entity;
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

	void AddEntity(Entity *);
	void BeginPlay();
	void BeginSecondaryCommandContext(CommandBuffer * ctx);
	void CreateResources(std::function<void(ResourceCreationContext&)> fun);
	std::vector<CommandBuffer *> CreateSecondaryCommandContexts();
	void DeserializePhysics(std::string const&);
	void DestroySecondaryCommandContexts(std::vector<CommandBuffer *>);
	Entity * GetEntityByIdx(size_t idx);
	Entity * GetEntityByName(std::string const&);
	size_t GetEntityCount();
	Entity * GetMainCamera();
	//TODO:
	PhysicsWorld * GetPhysicsWorld();
	IVec2 GetResolution();
	size_t GetSwapCount();
	size_t GetCurrFrame();
	void Init(ResourceManager * resMan, Renderer * renderer);
	void LoadDLL(std::string const&);
	void LoadScene(std::string const&);
	void RemoveEntity(Entity *);
	std::string SerializePhysics();
	void SubmitCommandBuffer(CommandBuffer *);
	void TakeCameraFocus(Entity *);
	void Tick();
	void UnloadDLL(std::string const&);
};
