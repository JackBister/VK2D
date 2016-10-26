#define KEYCODE_IMPL
#include "input.h"

#include <cstring>

#include "json.hpp"

#include "input_cfuncs.h"

#include "input.h.generated.h"

using nlohmann::json;
using namespace std;

DESERIALIZABLE_IMPL(Input)

void Input::Frame()
{
	for (auto& kbp : downKeys) {
		//If key was down last frame, it's held this frame
		if (kbp.second) {
			heldKeys[kbp.first] = true;
		}
	}
	downKeys.clear();
	upKeys.clear();
	//TODO: Put SDL polling in an event pump that sends input events here but handles some events(like quitting)
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
		{
			//TODO:
			exit(0);
		}
		case SDL_KEYDOWN:
		{
			if (!e.key.repeat) {
				downKeys[static_cast<Keycode>(e.key.keysym.sym)] = true;
			}
			break;
		}
		case SDL_KEYUP:
		{
			//if key up, it's no longer held
			heldKeys[static_cast<Keycode>(e.key.keysym.sym)] = false;
			upKeys[static_cast<Keycode>(e.key.keysym.sym)] = true;
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			downKeys[static_cast<Keycode>(MOUSE_TO_KEYCODE(e.button.button))] = true;
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			heldKeys[static_cast<Keycode>(MOUSE_TO_KEYCODE(e.button.button))] = false;
			upKeys[static_cast<Keycode>(MOUSE_TO_KEYCODE(e.button.button))] = true;
		}
		}
	}
}

bool Input::GetKey(Keycode kc)
{
	return heldKeys[kc];
}

int Input::GetKey_Lua(lua_State * L)
{
	INPUT_GET(GetKey, KEY)
}

bool Input::GetKeyDown(Keycode kc)
{
	return downKeys[kc];
}

int Input::GetKeyDown_Lua(lua_State * L)
{
	INPUT_GET(GetKeyDown, KEY)
}

bool Input::GetKeyUp(Keycode kc)
{
	return upKeys[kc];
}

int Input::GetKeyUp_Lua(lua_State * L)
{
	INPUT_GET(GetKeyUp, KEY)
}

bool Input::GetButton(string b)
{
	for (auto& kc : buttonMap[b]) {
		if (GetKey(kc)) {
			return true;
		}
	}
	return false;
}

bool Input::GetButtonDown(string b)
{
	for (auto& kc : buttonMap[b]) {
		if (GetKeyDown(kc)) {
			return true;
		}
	}
	return false;
}

bool Input::GetButtonUp(string b)
{
	for (auto& kc : buttonMap[b]) {
		if (GetKeyUp(kc)) {
			return true;
		}
	}
	return false;
}

void Input::AddKeybind(string s, Keycode kc)
{
	buttonMap[s].push_back(kc);
}

int Input::AddKeybind_Lua(lua_State *L)
{
	INPUT_KEYBIND(AddKeybind)
}

void Input::RemoveKeybind(string s, Keycode kc)
{
	vector<Keycode>& v = buttonMap[s];
	for (auto it = v.begin(); it != v.end(); ++it) {
		if (*it == kc) {
			v.erase(it);
		}
	}
}

int Input::RemoveKeybind_Lua(lua_State * L)
{
	INPUT_KEYBIND(RemoveKeybind)
}

Deserializable * Input::Deserialize(ResourceManager * resourceManager, const std::string& str, Allocator& alloc) const
{
	void * mem = alloc.Allocate(sizeof(Input));
	Input * ret = new (mem) Input();
	json j = json::parse(str);
	for (auto& js : j["keybinds"]) {
		string name = js["name"];
		for (auto& kjs : js["keys"]) {
			ret->AddKeybind(name, strToKeycode[kjs]);
		}
	}

	return ret;
}
