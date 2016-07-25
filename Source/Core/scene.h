#pragma once
#include <string>
#include <vector>

#include "dtime.h"
#include "eventarg.h"
#include "input.h"
#include "luaserializable.h"

//TODO: Allocate all entities/components from same block for cache

struct Entity;

struct Scene : LuaSerializable
{
	Input * input;
	Time time;
	std::vector<Entity *> entities;

	void PushToLua(lua_State *) override;

	static int LuaIndex(lua_State *);
	static int LuaNewIndex(lua_State *);

	/*
		Returns the entity with the given name, or nullptr if an entity with that name does not exist.
	*/
	Entity * GetEntityByName(std::string);
	static int GetEntityByName_Lua(lua_State *);

	/*
		Sends an event to all entities.
	*/
	void BroadcastEvent(std::string ename, EventArgs eas = {});
	static int BroadcastEvent_Lua(lua_State *);

	/*
		Reads a JSON file describing a scene and returns that scene
	*/
	static Scene * FromFile(std::string fn);
};

