#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include <SDL2/SDL.h>

#include "Core/Deserializable.h"
#include "Core/DllExport.h"
#include "Core/keycodes.h"
#include "Core/Lua/luaserializable.h"
#include "Core/Queue.h"

typedef std::unordered_map<Keycode, bool, std::hash<int>> Keymap;

class EAPI Input : public LuaSerializable
{
	//RTTR_ENABLE(LuaSerializable)
public:
	Input(Queue<SDL_Event>::Reader&&);

	//Call every frame
	//TODO: This could/should be static and push events to all Input instances - question is if that's meaningful in any way.
	void Frame();

	void AddKeybind(std::string const&, Keycode);

	void DeserializeInPlace(std::string const&);
	std::string Serialize() const;

	//TODO: Make Keycode a class with implicit std::string constructor for ez Lua interaction
	//TODO: Modifier keys?
	bool GetKey(Keycode);
	bool GetKeyDown(Keycode);
	bool GetKeyUp(Keycode);

	bool GetButton(std::string const&);
	bool GetButtonDown(std::string const&);
	bool GetButtonUp(std::string const&);

	void RemoveKeybind(std::string const&, Keycode);

private:
	Queue<SDL_Event>::Reader inputQueue;
	//TODO: vector means you can have infinite keys bound
	std::unordered_map<std::string, std::vector<Keycode>> buttonMap;
	Keymap downKeys;
	Keymap heldKeys;
	Keymap upKeys;
};
