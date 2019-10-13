#include "Core/Input.h"

#include <algorithm>
#include <SDL2/SDL.h>
#include <set>
#include <unordered_map>

#include <imgui.h>

namespace Input
{
	std::unordered_map<HashedString, std::set<Keycode>> buttonMap;

	std::unordered_map<Keycode, bool, std::hash<int>> downKeys;
	std::unordered_map<Keycode, bool, std::hash<int>> heldKeys;
	std::unordered_map<Keycode, bool, std::hash<int>> upKeys;

	IVec2 mousePosition;

	void Frame()
	{
		for (auto const& kbp : downKeys) {
			//If key was down last frame, it's held this frame
			//TODO: Maybe there needs to be a delay of 1 or 2 frames (or at higher FPS people will always be holding)
			if (kbp.second) {
				heldKeys[kbp.first] = true;
			}
		}
		downKeys.clear();
		upKeys.clear();

		SDL_GetMouseState(&mousePosition.x, &mousePosition.y);;

		auto& io = ImGui::GetIO();

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
					io.KeysDown[e.key.keysym.scancode] = true;

					downKeys[static_cast<Keycode>(e.key.keysym.sym)] = true;
				}
				break;
			}
			case SDL_KEYUP:
			{
				io.KeysDown[e.key.keysym.scancode] = false;

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
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				if (e.wheel.y > 0) io.MouseWheel += 1;
				if (e.wheel.y < 0) io.MouseWheel -= 1;
				break;
			}
			case SDL_TEXTINPUT:
			{
				io.AddInputCharactersUTF8(e.text.text);
				break;
			}
			}
		}

		io.MousePos = ImVec2(mousePosition.x, mousePosition.y);
		io.MouseDown[0] = heldKeys[KC_MOUSE_LEFT];
		io.MouseDown[1] = heldKeys[KC_MOUSE_RIGHT];
		io.KeyAlt = heldKeys[KC_LALT] || heldKeys[KC_RALT];
		io.KeyCtrl = heldKeys[KC_LCTRL] || heldKeys[KC_RCTRL];
		io.KeyShift = heldKeys[KC_LSHIFT] || heldKeys[KC_RSHIFT];
		io.KeySuper = heldKeys[KC_LGUI] || heldKeys[KC_RGUI];
	}

	void Init()
	{
	    ImGui::CreateContext();
	 	auto& io = ImGui::GetIO();
	 	io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
		io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
		io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
		io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
		io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
		//io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
		io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
		//io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
		io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
		io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
		io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
		io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
		io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
		io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
		io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
		io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
	}

	bool EAPI GetKey(Keycode kc)
	{
		return heldKeys[kc];
	}

	bool EAPI GetKeyDown(Keycode kc)
	{
		return downKeys[kc];
	}

	bool EAPI GetKeyUp(Keycode kc)
	{
		return upKeys[kc];
	}

	bool EAPI GetButton(HashedString s)
	{
		if (buttonMap.find(s) != buttonMap.end()) {
			for (auto const kc : buttonMap[s]) {
				if (heldKeys[kc]) {
					return true;
				}
			}
		}
		return false;
	}

	bool EAPI GetButtonDown(HashedString s)
	{
		if (buttonMap.find(s) != buttonMap.end()) {
			for (auto const kc : buttonMap[s]) {
				if (downKeys[kc]) {
					return true;
				}
			}
		}
		return false;
	}

	bool EAPI GetButtonUp(HashedString s)
	{
		if (buttonMap.find(s) != buttonMap.end()) {
			for (auto const kc : buttonMap[s]) {
				if (upKeys[kc]) {
					return true;
				}
			}
		}
		return false;
	}

	IVec2 EAPI GetMousePosition()
	{
		return mousePosition;
	}

	void EAPI AddKeybind(HashedString s, Keycode kc)
	{
		buttonMap[s].emplace(kc);
	}

	void EAPI RemoveKeybind(HashedString s, Keycode keycode)
	{
		if (buttonMap.find(s) != buttonMap.end()) {
			buttonMap[s].erase(keycode);
		}
	}
};
