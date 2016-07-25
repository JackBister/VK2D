#define KEYCODE_IMPL
#include "input.h"

#include <cstring>

#include "json.hpp"

#include "input_cfuncs.h"

using nlohmann::json;
using namespace std;

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
			downKeys[static_cast<Keycode>(e.key.keysym.sym)] = true;
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

int Input::GetButton_Lua(lua_State * L)
{
	INPUT_GET(GetButton, BUTTON)
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

int Input::GetButtonDown_Lua(lua_State * L)
{
	INPUT_GET(GetButtonDown, BUTTON)
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

int Input::GetButtonUp_Lua(lua_State * L)
{
	INPUT_GET(GetButtonUp, BUTTON)
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


PUSH_TO_LUA(Input);

int Input::LuaIndex(lua_State * L)
{
	void ** inputPtr = static_cast<void **>(lua_touserdata(L, 1));
	Input * ptr = static_cast<Input *>(*inputPtr);
	const char * idx = lua_tostring(L, 2);
	if (ptr == nullptr || idx == nullptr) {
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(idx, "GetKey") == 0) {
		lua_pushcfunction(L, Input::GetKey_Lua);
	} else if (strcmp(idx, "GetKeyDown") == 0) {
		lua_pushcfunction(L, Input::GetKeyDown_Lua);
	} else if (strcmp(idx, "GetKeyUp") == 0) {
		lua_pushcfunction(L, Input::GetKeyUp_Lua);
	} else if (strcmp(idx, "GetButton") == 0) {
		lua_pushcfunction(L, Input::GetButton_Lua);
	} else if (strcmp(idx, "GetButtonDown") == 0) {
		lua_pushcfunction(L, Input::GetButtonDown_Lua);
	} else if (strcmp(idx, "GetButtonUp") == 0) {
		lua_pushcfunction(L, Input::GetButtonUp_Lua);
	} else if (strcmp(idx, "AddKeybind") == 0) {
		lua_pushcfunction(L, Input::AddKeybind_Lua);
	} else if (strcmp(idx, "RemoveKeybind") == 0) {
		lua_pushcfunction(L, Input::RemoveKeybind_Lua);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

int Input::LuaNewIndex(lua_State *)
{
	return 0;
}

Input * Input::Deserialize(string s)
{
	Input * ret = new Input();
	json j = json::parse(s);
	for (auto& js : j["keybinds"]) {
		string name = js["name"];
		for (auto& kjs : js["keys"]) {
			ret->AddKeybind(name, strToKeycode[kjs]);
		}
	}

	return ret;
}
