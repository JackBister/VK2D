#pragma once

#include <functional>
#include <glm/fwd.hpp>

#include "Core/DllExport.h"
#include "Core/Queue.h"

class CameraComponent;
class CommandBuffer;
class Entity;
class PhysicsWorld;
class ResourceCreationContext;
class ResourceManager;
class RenderSystem;
struct SubmittedSprite;

namespace GameModule {
	enum class FrameStage
	{
		INPUT,
		TIME,
		FENCE_WAIT,
		PHYSICS,
		TICK,
		RENDER,
	};

	void AddEntity(Entity *);
	void BeginPlay();
    void CreateResources(std::function<void(ResourceCreationContext &)> fun);
	void DeserializePhysics(std::string const&);
	Entity * GetEntityByIdx(size_t idx);
	Entity * GetEntityByName(std::string const&);
	size_t GetEntityCount();
	Entity * GetMainCamera();
	//TODO:
	PhysicsWorld * GetPhysicsWorld();
	void Init(ResourceManager * resMan, RenderSystem * renderSystem);
	void LoadDLL(std::string const&);
	void LoadScene(std::string const&);
	void RemoveEntity(Entity *);
	std::string SerializePhysics();
    void SubmitSprite(SubmittedSprite const & sprite);
	void TakeCameraFocus(Entity *);
	void Tick();
	void UnloadDLL(std::string const&);
};
