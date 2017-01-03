#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include <SDL/SDL.h>

#include "Core/Deserializable.h"
#include "Core/keycodes.h"
#include "Core/Lua/luaserializable.h"
#include "Core/Queue.h"

#include "Tools/HeaderGenerator/headergenerator.h"

typedef std::unordered_map<Keycode, bool, std::hash<int>> Keymap;

struct Input : LuaSerializable
{

	Input(Queue<SDL_Event>::Reader&&) noexcept;
	//Call every frame
	//TODO: This could/should be static and push events to all Input instances - question is if that's meaningful in any way.
	void Frame() noexcept;

	//TODO: Make Keycode a class with implicit std::string constructor for ez Lua interaction
	//TODO: Modifier keys?
	bool GetKey(Keycode);
	static int GetKey_Lua(lua_State *);
	bool GetKeyDown(Keycode);
	static int GetKeyDown_Lua(lua_State *);
	bool GetKeyUp(Keycode);
	static int GetKeyUp_Lua(lua_State *);

	PROPERTY(LuaRead)
	bool GetButton(std::string);
	PROPERTY(LuaRead)
	bool GetButtonDown(std::string);
	PROPERTY(LuaRead)
	bool GetButtonUp(std::string);

	void AddKeybind(std::string, Keycode);
	static int AddKeybind_Lua(lua_State *);
	void RemoveKeybind(std::string, Keycode);
	static int RemoveKeybind_Lua(lua_State *);

	int LuaIndex(lua_State *) override;
	int LuaNewIndex(lua_State *) override;

	void DeserializeInPlace(const std::string&) noexcept;

private:
	Queue<SDL_Event>::Reader inputQueue;
	//TODO: vector means you can have infinite keys bound
	std::unordered_map<std::string, std::vector<Keycode>> buttonMap;
	Keymap downKeys;
	Keymap heldKeys;
	Keymap upKeys;
};
