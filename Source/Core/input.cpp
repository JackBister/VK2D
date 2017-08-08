#define KEYCODE_IMPL
#include "Core/input.h"

#include "json.hpp"

#include "Core/Lua/input_cfuncs.h"
#include "Core/Maybe.h"

#include "Core/input.h.generated.h"

Input::Input(Queue<SDL_Event>::Reader&& reader) noexcept
	: input_queue_(std::move(reader))
{
}

void Input::Frame() noexcept
{
	for (auto const& kbp : down_keys_) {
		//If key was down last frame, it's held this frame
		if (kbp.second) {
			held_keys_[kbp.first] = true;
		}
	}
	down_keys_.clear();
	up_keys_.clear();

	std::optional<SDL_Event> evt;
	do {
		evt = input_queue_.Pop();
		if (evt.has_value()) {
			SDL_Event const& e = evt.value();
			switch (e.type) {
			case SDL_QUIT:
			{
				//TODO:
				exit(0);
			}
			case SDL_KEYDOWN:
			{
				if (!e.key.repeat) {
					down_keys_[static_cast<Keycode>(e.key.keysym.sym)] = true;
				}
				break;
			}
			case SDL_KEYUP:
			{
				held_keys_[static_cast<Keycode>(e.key.keysym.sym)] = false;
				up_keys_[static_cast<Keycode>(e.key.keysym.sym)] = true;
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				down_keys_[static_cast<Keycode>(MOUSE_TO_KEYCODE(e.button.button))] = true;
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				held_keys_[static_cast<Keycode>(MOUSE_TO_KEYCODE(e.button.button))] = false;
				up_keys_[static_cast<Keycode>(MOUSE_TO_KEYCODE(e.button.button))] = true;
			}
			}
		}
	} while (evt.has_value());
}

bool Input::GetKey(Keycode const kc)
{
	return held_keys_[kc];
}

int Input::GetKey_Lua(lua_State * const L)
{
	INPUT_GET(GetKey, KEY)
}

bool Input::GetKeyDown(Keycode const kc)
{
	return down_keys_[kc];
}

int Input::GetKeyDown_Lua(lua_State * const L)
{
	INPUT_GET(GetKeyDown, KEY)
}

bool Input::GetKeyUp(Keycode const kc)
{
	return up_keys_[kc];
}

int Input::GetKeyUp_Lua(lua_State * const L)
{
	INPUT_GET(GetKeyUp, KEY)
}

bool Input::GetButton(std::string const& b)
{
	for (auto const& kc : button_map_[b]) {
		if (GetKey(kc)) {
			return true;
		}
	}
	return false;
}

bool Input::GetButtonDown(std::string const& b)
{
	for (auto const& kc : button_map_[b]) {
		if (GetKeyDown(kc)) {
			return true;
		}
	}
	return false;
}

bool Input::GetButtonUp(std::string const& b)
{
	for (auto const& kc : button_map_[b]) {
		if (GetKeyUp(kc)) {
			return true;
		}
	}
	return false;
}

void Input::AddKeybind(std::string const& s, Keycode kc)
{
	button_map_[s].push_back(kc);
}

int Input::AddKeybind_Lua(lua_State * const L)
{
	INPUT_KEYBIND(AddKeybind)
}

void Input::RemoveKeybind(std::string const& s, Keycode kc)
{
	std::vector<Keycode>& v = button_map_[s];
	for (auto it = v.begin(); it != v.end(); ++it) {
		if (*it == kc) {
			v.erase(it);
		}
	}
}

int Input::RemoveKeybind_Lua(lua_State * const L)
{
	INPUT_KEYBIND(RemoveKeybind)
}

void Input::DeserializeInPlace(std::string const& serializedInput) noexcept
{
	auto j = nlohmann::json::parse(serializedInput);
	for (auto const& js : j["keybinds"]) {
		std::string name = js["name"];
		for (auto const& kjs : js["keys"]) {
			this->AddKeybind(name, strToKeycode[kjs]);
		}
	}
}
