#pragma once

#include <ThirdParty/glm/glm/glm.hpp>

#include "Keycodes.h"
#include "Util/DllExport.h"
#include "Util/HashedString.h"

class Gamepad;

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
EAPI Gamepad const * GetGamepad(size_t idx);
// Returns the number of unique controllers that have been connected since the engine was started.
// That is, if two controllers were connected and one was disconnected, this would return 2. If the controller was then
// connected this would still return 2.
size_t EAPI GetGamepadCount();

void EAPI AddKeybind(HashedString, Keycode);
void EAPI RemoveKeybind(HashedString, Keycode);
void EAPI RemoveAllKeybindings();
};
