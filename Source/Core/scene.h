#pragma once
#include <memory>
#include <string>
#include <vector>

#include "SDL/SDL.h"

#include "Core/dtime.h"
#include "Core/eventarg.h"
#include "Core/input.h"
#include "Core/Lua/luaserializable.h"
#include "Core/Queue.h"
#include "Core/Rendering/RenderCommand.h"
#include "Core/Rendering/Context/RenderContext.h"
#include "Core/Resource.h"

#include "Tools/HeaderGenerator/headergenerator.h"

//TODO: Allocate all entities/components from same block for cache

class CameraComponent;
class Entity;
class PhysicsWorld;
class ResourceManager;

PROPERTY(LuaIndex, LuaNewIndex)
class Scene : public LuaSerializable, public Resource
{
	RTTR_ENABLE(LuaSerializable)
public:

	Scene(std::string const&, ResourceManager *, Queue<SDL_Event>::Reader&&, Queue<RenderCommand>::Writer&&, std::string const&, Renderer * renderer) noexcept;

	void EndFrame() noexcept;

	void BeginSecondaryCommandContext(RenderCommandContext *);
	std::vector<std::unique_ptr<RenderCommandContext>> CreateSecondaryCommandContexts();
	size_t GetSwapCount();
	size_t GetCurrFrame();
	void PushRenderCommand(RenderCommand&&) noexcept;
	void SubmitCamera(CameraComponent *) noexcept;
	void SubmitCommandBuffer(std::unique_ptr<RenderCommandContext>&&);
	void Tick() noexcept;

	/*
		Returns the entity with the given name, or nullptr if an entity with that name does not exist.
	*/
	PROPERTY(LuaRead)
	Entity * GetEntityByName(std::string);
	//Entity * GetEntityByName(char const *);
	static int GetEntityByName_Lua(lua_State *);

	/*
		Sends an event to all entities.
	*/
	PROPERTY(LuaRead)
	void BroadcastEvent(std::string ename, EventArgs eas = {});
	static int BroadcastEvent_Lua(lua_State *);

	ResourceManager * resource_manager_;
	PROPERTY(LuaRead)
	Input input_;
	PhysicsWorld * physics_world_;
	Time time_;
	std::vector<Entity *> entities_;

private:
	struct FrameInfo
	{
		std::vector<SubmittedCamera> cameras_to_submit_;
		std::vector<std::unique_ptr<RenderCommandContext>> command_buffers_;

		RenderPassHandle * main_renderpass_;
		uint32_t current_subpass_;

		std::unique_ptr<RenderCommandContext> main_command_context_;
		std::unique_ptr<RenderCommandContext> pre_renderpass_context_;

		FenceHandle * can_start_frame_;
		SemaphoreHandle * pre_renderpass_finished_;
		SemaphoreHandle * main_renderpass_finished_;

		CommandContextAllocator * commandContextAllocator;
	};

	void CreatePrimitives();

	Queue<RenderCommand>::Writer render_queue_;

	/*
	Queue<std::unique_ptr<RenderCommandContext>> free_command_buffers_;
	Queue<std::unique_ptr<RenderCommandContext>>::Reader get_free_command_buffers_;
	Queue<std::unique_ptr<RenderCommandContext>>::Writer put_free_command_buffers_;
	std::vector<std::unique_ptr<RenderCommandContext>> command_buffers_;
	*/
	//uint32_t current_subpass_;
	//std::unique_ptr<RenderCommandContext> main_command_context_;
	//RenderPassHandle * main_renderpass_;
	//SemaphoreHandle * main_renderpass_finished_;
	//std::unique_ptr<RenderCommandContext> pre_renderpass_context_;
	//SemaphoreHandle * pre_renderpass_finished_;
	Renderer * renderer_;
	//SemaphoreHandle * swap_finished_;

	size_t currFrameInfo = 0;
	std::vector<FrameInfo> frameInfo;
};
