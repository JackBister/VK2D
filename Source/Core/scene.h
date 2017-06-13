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
#include "Core/Rendering/Accessor.h"
#include "Core/Rendering/Material.h"
#include "Core/Rendering/Mesh.h"
#include "Core/Rendering/Program.h"
#include "Core/Rendering/RenderCommand.h"
#include "Core/Resource.h"

#include "Tools/HeaderGenerator/headergenerator.h"

//TODO: Allocate all entities/components from same block for cache

struct CameraComponent;
struct Entity;
struct PhysicsWorld;
struct ResourceManager;

PROPERTY(LuaIndex, LuaNewIndex)
struct Scene : LuaSerializable, Resource
{
	ResourceManager * resourceManager;
	PROPERTY(LuaRead)
	Input input;
	PhysicsWorld * physicsWorld;
	Time time;
	std::vector<Entity *> entities;

	Scene(const std::string&, ResourceManager *, Queue<SDL_Event>::Reader&&, Queue<RenderCommand>::Writer&&, const std::string&) noexcept;

	void EndFrame() noexcept;
	void PushRenderCommand(RenderCommand&&) noexcept;
	void SubmitCamera(CameraComponent *) noexcept;
	void SubmitCommandBuffer(std::unique_ptr<RenderCommandContext>&&);
	void Tick() noexcept;

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	/*
		Returns the entity with the given name, or nullptr if an entity with that name does not exist.
	*/
	PROPERTY(LuaRead)
	Entity * GetEntityByName(std::string);
	static int GetEntityByName_Lua(lua_State *);

	/*
		Sends an event to all entities.
	*/
	PROPERTY(LuaRead)
	void BroadcastEvent(std::string ename, EventArgs eas = {});
	static int BroadcastEvent_Lua(lua_State *);

private:
	Queue<RenderCommand>::Writer renderQueue;
	std::vector<SubmittedCamera> camerasToSubmit;
	void CreatePrimitives();

	std::vector<std::unique_ptr<RenderCommandContext>> command_buffers_;
	RenderPassHandle * main_renderpass_;
};
