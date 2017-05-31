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
#include "Core/Rendering/ViewDef.h"
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

	Scene(const std::string&, ResourceManager *, Queue<SDL_Event>::Reader&&, Queue<RenderCommand>::Writer&&, Queue<ViewDef *>::Reader&&, const std::string&) noexcept;

	void EndFrame() noexcept;
	void PushRenderCommand(const RenderCommand&) noexcept;
	void SubmitCamera(CameraComponent *) noexcept;
	void SubmitMesh(Mesh *) noexcept;
	void SubmitSprite(Sprite *) noexcept;
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
	Queue<ViewDef *>::Reader viewDefQueue;
	/*
		The scene allocates an array of ViewDefs (depending on if it's double buffered or triple), but may only write into one of them at a time.
		Any ViewDef that isn't the current ViewDef belongs to the renderer, so messing with it can cause crazy results.
	*/
	std::vector<ViewDef> viewDefs;
	ViewDef * currentViewDef;
	std::vector<SubmittedCamera> camerasToSubmit;
	std::vector<SubmittedMesh> meshesToSubmit;
	std::vector<SubmittedSprite> spritesToSubmit;

	//TODO:
	std::vector<std::shared_ptr<Accessor>> accessors;
	std::vector<std::shared_ptr<Material>> materials;
	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<std::shared_ptr<Program>> programs;
};
