#define KEYCODE_IMPL
#include "Core/input.h"

#include "json.hpp"

#include "Core/Lua/input_cfuncs.h"
#include "Core/Maybe.h"

#include "Core/input.h.generated.h"

using nlohmann::json;
using namespace std;

Input::Input(Queue<SDL_Event>::Reader&& reader) noexcept
	: inputQueue(std::move(reader))
{
}

void Input::Frame() noexcept
{
	for (auto& kbp : downKeys) {
		//If key was down last frame, it's held this frame
		if (kbp.second) {
			heldKeys[kbp.first] = true;
		}
	}
	downKeys.clear();
	upKeys.clear();

	Maybe<SDL_Event> evt;
	do {
		evt = inputQueue.Pop();
		if (evt.index() != 0) {
			const SDL_Event& e = std::get<SDL_Event>(evt);
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
	} while (evt.index() != 0);
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

void Input::DeserializeInPlace(const std::string& serializedInput) noexcept
{
	json j = json::parse(serializedInput);
	for (auto& js : j["keybinds"]) {
		string name = js["name"];
		for (auto& kjs : js["keys"]) {
			this->AddKeybind(name, strToKeycode[kjs]);
		}
	}
}
