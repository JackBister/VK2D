#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Util/DllExport.h"

enum class GamepadAxis { AXIS_LEFTX, AXIS_LEFTY, AXIS_RIGHTX, AXIS_RIGHTY, AXIS_TRIGGERLEFT, AXIS_TRIGGERRIGHT };

enum class GamepadButton {
    BUTTON_A,
    BUTTON_B,
    BUTTON_X,
    BUTTON_Y,
    BUTTON_BACK,
    BUTTON_GUIDE,
    BUTTON_START,
    BUTTON_LEFTSTICK,
    BUTTON_RIGHTSTICK,
    BUTTON_LEFTSHOULDER,
    BUTTON_RIGHTSHOULDER,
    BUTTON_DPAD_UP,
    BUTTON_DPAD_DOWN,
    BUTTON_DPAD_LEFT,
    BUTTON_DPAD_RIGHT,
};

class EAPI Gamepad
{
public:
    Gamepad();
    Gamepad(Gamepad &&);
    Gamepad(int32_t joystickId);
    ~Gamepad();
    Gamepad & operator=(Gamepad &&);

    float GetAxis(GamepadAxis axis) const;
    float GetAxisRaw(GamepadAxis axis) const;
    bool GetButton(GamepadButton button) const;
    std::string GetName() const;

    int32_t GetId() const;

private:
    class Pimpl;

    std::unique_ptr<Pimpl> pimpl;
};
