#pragma once

#include <string>

#include <ThirdParty/SDL2/include/SDL.h>

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
    Gamepad(Gamepad && pad) : gameController(pad.gameController), joystickId(pad.joystickId)
    {
        pad.gameController = nullptr;
        pad.joystickId = 0;
    }
    Gamepad(SDL_GameController * gameController, SDL_JoystickID joystickId)
        : gameController(gameController), joystickId(joystickId)
    {
    }

    ~Gamepad();

    Gamepad & operator=(Gamepad && pad)
    {
        this->gameController = pad.gameController;
        this->joystickId = pad.joystickId;
        pad.gameController = nullptr;
        pad.joystickId = 0;
        return *this;
    }

    float GetAxis(GamepadAxis axis) const;
    float GetAxisRaw(GamepadAxis axis) const;
    bool GetButton(GamepadButton button) const;
    std::string GetName() const;

    SDL_JoystickID GetId() const;

private:
    SDL_GameController * gameController;
    SDL_JoystickID joystickId;
};
