#pragma once
#include <memory>
#include <string>
#include <vector>

#include "SDL2/SDL.h"

#include "Core/dtime.h"
#include "Core/eventarg.h"
#include "Core/GameModule.h"
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
	//RTTR_ENABLE(LuaSerializable)
public:
	Scene(std::string const&, ResourceManager *, std::string const&);

	void LoadFile(std::string const&);
	void SerializeToFile(std::string const& filename);
	void Unload();

	/*
		Sends an event to all entities.
	*/
	void BroadcastEvent(std::string ename, EventArgs eas = {});

	/*
		Returns the entity with the given name, or nullptr if an entity with that name does not exist.
	*/
	Entity * GetEntityByName(std::string);

private:
	Renderer * renderer;
	ResourceManager * resourceManager;

	std::vector<std::string> dllFileNames;
	std::vector<Entity *> entities;
};
