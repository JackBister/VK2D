#pragma once
#include <memory>
#include <string>
#include <vector>

#include "SDL2/SDL.h"

#include "Core/dtime.h"
#include "Core/eventarg.h"
#include "Core/HashedString.h"
#include "Core/GameModule.h"
#include "Core/Input.h"
#include "Core/Queue.h"

//TODO: Allocate all entities/components from same block for cache

class Entity;

class Scene
{
public:
	Scene(std::string const&);

	void LoadFile(std::string const&);
	void SerializeToFile(std::string const& filename);
	void Unload();

	/*
		Sends an event to all entities.
	*/
	void BroadcastEvent(HashedString ename, EventArgs eas = {});

	/*
		Returns the entity with the given name, or nullptr if an entity with that name does not exist.
	*/
	Entity * GetEntityByName(std::string);

private:
	std::vector<std::string> dllFileNames;

	std::vector<Entity *> entities;
};
