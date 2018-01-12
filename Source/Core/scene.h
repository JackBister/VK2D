#pragma once
#include <memory>
#include <string>
#include <vector>

#include "SDL2/SDL.h"

#include "Core/dtime.h"
#include "Core/eventarg.h"
#include "Core/input.h"
#include "Core/Lua/luaserializable.h"
#include "Core/Queue.h"
#include "Core/Rendering/RenderCommand.h"
#include "Core/Rendering/Context/RenderContext.h"
#include "Core/Resource.h"

//TODO: Allocate all entities/components from same block for cache

class CameraComponent;
class Entity;
class PhysicsWorld;
class ResourceManager;

class Scene : public LuaSerializable, public Resource
{
	RTTR_ENABLE(LuaSerializable)
public:
	Scene(std::string const&, ResourceManager *, Queue<SDL_Event>::Reader&&, std::string const&, Renderer * renderer) noexcept;

	void BeginSecondaryCommandContext(CommandBuffer *);
	void CreateResources(std::function<void(ResourceCreationContext&)> fun);
	std::vector<CommandBuffer *> CreateSecondaryCommandContexts();
	size_t GetSwapCount();
	size_t GetCurrFrame();
	void SubmitCamera(CameraComponent *) noexcept;
	void SubmitCommandBuffer(CommandBuffer *);
	void Tick() noexcept;

	/*
		Sends an event to all entities.
	*/
	void BroadcastEvent(std::string ename, EventArgs eas = {});

	/*
		Returns the entity with the given name, or nullptr if an entity with that name does not exist.
	*/
	Entity * GetEntityByName(std::string);


	Input input;
	PhysicsWorld * physicsWorld;
	Time time;
private:
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

	struct FrameInfo
	{
		std::vector<SubmittedCamera> cameras_to_submit_;
		std::vector<CommandBuffer *> command_buffers_;

		FramebufferHandle * framebuffer;

		RenderPassHandle * main_renderpass_;
		uint32_t current_subpass_;

		CommandBuffer * main_command_context_;
		CommandBuffer * pre_renderpass_context_;

		FenceHandle * can_start_frame_;
		SemaphoreHandle * framebufferReady;
		SemaphoreHandle * pre_renderpass_finished_;
		SemaphoreHandle * main_renderpass_finished_;

		CommandContextAllocator * commandContextAllocator;
	};

	void CreatePrimitives();

	Renderer * renderer;

	uint32_t currFrameInfoIdx = 0;
	std::vector<FrameInfo> frameInfo;
	FrameStage currFrameStage;
	ResourceManager * resourceManager;

	std::vector<Entity *> entities;
};
