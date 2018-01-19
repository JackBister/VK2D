#define KEYCODE_IMPL
#include "Core/Input.h"

#include <algorithm>
#include <SDL2/SDL.h>
#include <set>
#include <unordered_map>

namespace Input
{
	std::unordered_map<HashedString, std::set<Keycode>> buttonMap;

	std::unordered_map<Keycode, bool, std::hash<int>> downKeys;
	std::unordered_map<Keycode, bool, std::hash<int>> heldKeys;
	std::unordered_map<Keycode, bool, std::hash<int>> upKeys;

	void EAPI AddKeybind(HashedString s, Keycode kc)
	{
		buttonMap[s].emplace(kc);
	}

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

	void EAPI RemoveKeybind(HashedString s, Keycode keycode)
	{
		if (buttonMap.find(s) != buttonMap.end()) {
			buttonMap[s].erase(keycode);
		}
	}
};
