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

class Input : public LuaSerializable
{
	RTTR_ENABLE(LuaSerializable)
public:
	Input(Queue<SDL_Event>::Reader&&) noexcept;
	//Call every frame
	//TODO: This could/should be static and push events to all Input instances - question is if that's meaningful in any way.
	void Frame() noexcept;

	void AddKeybind(std::string const&, Keycode);
	static int AddKeybind_Lua(lua_State *);

	void DeserializeInPlace(std::string const&) noexcept;

	//TODO: Make Keycode a class with implicit std::string constructor for ez Lua interaction
	//TODO: Modifier keys?
	bool GetKey(Keycode);
	static int GetKey_Lua(lua_State *);
	bool GetKeyDown(Keycode);
	static int GetKeyDown_Lua(lua_State *);
	bool GetKeyUp(Keycode);
	static int GetKeyUp_Lua(lua_State *);

	PROPERTY(LuaRead)
	bool GetButton(std::string const&);
	PROPERTY(LuaRead)
	bool GetButtonDown(std::string const&);
	PROPERTY(LuaRead)
	bool GetButtonUp(std::string const&);

	void RemoveKeybind(std::string const&, Keycode);
	static int RemoveKeybind_Lua(lua_State *);

private:
	Queue<SDL_Event>::Reader input_queue_;
	//TODO: vector means you can have infinite keys bound
	std::unordered_map<std::string, std::vector<Keycode>> button_map_;
	Keymap down_keys_;
	Keymap held_keys_;
	Keymap up_keys_;
};
