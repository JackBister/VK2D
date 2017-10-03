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

	Scene(std::string const&, ResourceManager *, Queue<SDL_Event>::Reader&&, Queue<RenderCommand>::Writer&&, std::string const&) noexcept;

	void EndFrame() noexcept;

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
	void CreatePrimitives();

	Queue<RenderCommand>::Writer render_queue_;
	std::vector<SubmittedCamera> cameras_to_submit_;

	std::vector<std::unique_ptr<RenderCommandContext>> command_buffers_;
	RenderPassHandle * main_renderpass_;
};
