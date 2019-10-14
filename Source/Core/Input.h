#pragma once

#include <glm/fwd.hpp>
#include "Core/DllExport.h"
#include "Core/HashedString.h"
#include "Core/keycodes.h"

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

	void EAPI AddKeybind(HashedString, Keycode);
	void EAPI RemoveKeybind(HashedString, Keycode);
};
