#pragma once

#include "Core/DllExport.h"
#include "Core/HashedString.h"
#include "Core/keycodes.h"

namespace Input
{
	void EAPI AddKeybind(HashedString, Keycode);
	void Frame();
	bool EAPI GetKey(Keycode);
	bool EAPI GetKeyDown(Keycode);
	bool EAPI GetKeyUp(Keycode);

	bool EAPI GetButton(HashedString);
	bool EAPI GetButtonDown(HashedString);
	bool EAPI GetButtonUp(HashedString);

	void EAPI RemoveKeybind(HashedString, Keycode);
};
