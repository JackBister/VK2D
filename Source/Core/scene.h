#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Core/dtime.h"
#include "Core/eventarg.h"
#include "Core/Lua/luaserializable.h"
#include "Core/Resource.h"

#include "Tools/HeaderGenerator/headergenerator.h"

//TODO: Allocate all entities/components from same block for cache

struct Entity;
struct Input;
struct PhysicsWorld;
struct ResourceManager;

struct Scene : LuaSerializable, Resource
{
	PROPERTY(LuaRead)
	Input * input;
	PhysicsWorld * physicsWorld;
	ResourceManager * resourceManager;
	Time time;
	std::vector<Entity *> entities;

	Scene(ResourceManager *, const std::string&) noexcept;
	Scene(ResourceManager *, const std::string&, const std::vector<char>&) noexcept;

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
};

RESOURCE_HEADER(Scene)
