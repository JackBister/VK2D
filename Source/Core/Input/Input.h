#pragma once

#include <glm/fwd.hpp>

#include "Core/DllExport.h"
#include "Core/HashedString.h"
#include "Gamepad.h"
#include "Keycodes.h"

namespace Input
{
void Frame();
void Init();
bool EAPI GetKey(Keycode);
bool EAPI GetKeyDown(Keycode);
bool EAPI GetKeyUp(Keycode);

bool EAPI GetButton(HashedString);
bool EAPI GetButtonDown(HashedString);
bool EAPI GetButtonUp(HashedString);

glm::ivec2 EAPI GetMousePosition();

// Returns null if idx >= GetGamepadCount() OR if the controller at the index has been unplugged
Gamepad const * GetGamepad(size_t idx);
// Returns the number of unique controllers that have been connected since the engine was started.
// That is, if two controllers were connected and one was disconnected, this would return 2. If the controller was then
// connected this would still return 2.
size_t GetGamepadCount();

void EAPI AddKeybind(HashedString, Keycode);
void EAPI RemoveKeybind(HashedString, Keycode);
};
