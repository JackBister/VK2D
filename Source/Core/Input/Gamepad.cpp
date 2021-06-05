#include "Gamepad.h"

#include <algorithm>

#include "Core/Config/Config.h"
#include "Logging/Logger.h"

static auto const logger = Logger::Create("Gamepad");

// These are just based on experimenting with my own gamepad, my triggers always were at 0 when neutral, but the sticks
// rested between 0 and 0.15. Based on that I chose to give the triggers a smaller deadzone by default so you can have
// better precision on them
static auto const deadzone = Config::AddFloat("gamepad_axis_deadzone", 0.2f);
static auto const deadzoneTriggers = Config::AddFloat("gamepad_axis_deadzone_triggers", 0.05f);

SDL_GameControllerAxis ConvertAxis(GamepadAxis axis)
{
    switch (axis) {
    case GamepadAxis::AXIS_LEFTX:
        return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX;
    case GamepadAxis::AXIS_LEFTY:
        return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY;
    case GamepadAxis::AXIS_RIGHTX:
        return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTX;
    case GamepadAxis::AXIS_RIGHTY:
        return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_RIGHTY;
    case GamepadAxis::AXIS_TRIGGERLEFT:
        return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    case GamepadAxis::AXIS_TRIGGERRIGHT:
        return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    default:
        logger.Error("Unhandled GamepadAxis={}, defaulting to LEFTX", axis);
        return SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX;
    }
}

SDL_GameControllerButton ConvertButton(GamepadButton button)
{
    switch (button) {
    case GamepadButton::BUTTON_A:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A;
    case GamepadButton::BUTTON_B:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B;
    case GamepadButton::BUTTON_X:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X;
    case GamepadButton::BUTTON_Y:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y;
    case GamepadButton::BUTTON_BACK:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_BACK;
    case GamepadButton::BUTTON_GUIDE:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_GUIDE;
    case GamepadButton::BUTTON_START:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START;
    case GamepadButton::BUTTON_LEFTSTICK:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSTICK;
    case GamepadButton::BUTTON_RIGHTSTICK:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    case GamepadButton::BUTTON_LEFTSHOULDER:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    case GamepadButton::BUTTON_RIGHTSHOULDER:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    case GamepadButton::BUTTON_DPAD_UP:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP;
    case GamepadButton::BUTTON_DPAD_DOWN:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    case GamepadButton::BUTTON_DPAD_LEFT:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    case GamepadButton::BUTTON_DPAD_RIGHT:
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    default:
        logger.Error("Unhandled GamepadButton={}, defaulting to BUTTON_A", button);
        return SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A;
    }
}

Gamepad::~Gamepad()
{
    if (gameController) {
        SDL_GameControllerClose(gameController);
    }
}

float Gamepad::GetAxis(GamepadAxis axis) const
{
    auto raw = GetAxisRaw(axis);
    if (axis == GamepadAxis::AXIS_TRIGGERLEFT || axis == GamepadAxis::AXIS_TRIGGERRIGHT) {
        return std::abs(raw) < deadzoneTriggers.Get() ? 0.f : raw;
    } else {
        return std::abs(raw) < deadzone.Get() ? 0.f : raw;
    }
}

float Gamepad::GetAxisRaw(GamepadAxis axis) const
{
    auto raw = SDL_GameControllerGetAxis(gameController, ConvertAxis(axis));

    // Normalizes the value to be between -1 and 1
    return std::max(raw / (double)INT16_MAX, -1.0);
}

bool Gamepad::GetButton(GamepadButton button) const
{
    auto btn = SDL_GameControllerGetButton(gameController, ConvertButton(button));
    return btn != 0;
}

std::string Gamepad::GetName() const
{
    return SDL_GameControllerName(gameController);
}

SDL_JoystickID Gamepad::GetId() const
{
    return joystickId;
}
