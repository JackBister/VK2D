#define KEYCODE_IMPL
#include "Core/input.h"

#include "nlohmann/json.hpp"
#include "rttr/registration.h"

#include "Core/Lua/input_cfuncs.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Input>("Input")
	.method("GetButton", &Input::GetButton)
	.method("GetButtonDown", &Input::GetButtonDown)
	.method("GetButtonUp", &Input::GetButtonUp);
}

Input::Input(Queue<SDL_Event>::Reader&& reader)
	: inputQueue(std::move(reader))
{
}

void Input::Frame()
{
	for (auto const& kbp : downKeys) {
		//If key was down last frame, it's held this frame
		if (kbp.second) {
			heldKeys[kbp.first] = true;
		}
	}
	downKeys.clear();
	upKeys.clear();

	std::optional<SDL_Event> evt;
	do {
		evt = inputQueue.Pop();
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
					downKeys[static_cast<Keycode>(e.key.keysym.sym)] = true;
				}
				break;
			}
			case SDL_KEYUP:
			{
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
	} while (evt.has_value());
}

bool Input::GetKey(Keycode const kc)
{
	return heldKeys[kc];
}

bool Input::GetKeyDown(Keycode const kc)
{
	return downKeys[kc];
}

bool Input::GetKeyUp(Keycode const kc)
{
	return upKeys[kc];
}

bool Input::GetButton(std::string const& b)
{
	for (auto const& kc : buttonMap[b]) {
		if (GetKey(kc)) {
			return true;
		}
	}
	return false;
}

bool Input::GetButtonDown(std::string const& b)
{
	for (auto const& kc : buttonMap[b]) {
		if (GetKeyDown(kc)) {
			return true;
		}
	}
	return false;
}

bool Input::GetButtonUp(std::string const& b)
{
	for (auto const& kc : buttonMap[b]) {
		if (GetKeyUp(kc)) {
			return true;
		}
	}
	return false;
}

void Input::AddKeybind(std::string const& s, Keycode kc)
{
	buttonMap[s].push_back(kc);
}

void Input::RemoveKeybind(std::string const& s, Keycode kc)
{
	std::vector<Keycode>& v = buttonMap[s];
	for (auto it = v.begin(); it != v.end(); ++it) {
		if (*it == kc) {
			v.erase(it);
		}
	}
}

void Input::DeserializeInPlace(std::string const& serializedInput)
{
	auto j = nlohmann::json::parse(serializedInput);
	for (auto const& js : j["keybinds"]) {
		std::string name = js["name"];
		for (auto const& kjs : js["keys"]) {
			this->AddKeybind(name, strToKeycode[kjs]);
		}
	}
}

std::string Input::Serialize() const
{
	nlohmann::json j;
	j["type"] = "Input";
	std::vector<nlohmann::json> keybinds;
	for (auto const kb : buttonMap) {
		nlohmann::json serializedKeybind;
		serializedKeybind["name"] = kb.first;
		std::vector<std::string> serializedKeycodes;
		for (auto const kc : kb.second) {
			serializedKeycodes.push_back(keycodeToStr[kc]);
		}
		serializedKeybind["keys"] = serializedKeycodes;
		keybinds.push_back(serializedKeybind);
	}
	j["keybinds"] = keybinds;
	return j.dump();
}
