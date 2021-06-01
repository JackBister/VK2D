#pragma once

#include <string>
#include <vector>

#include "Keycodes.h"

class Keybinding
{
public:
    Keybinding(std::string const & name, std::vector<Keycode> const & keyCodes) : name(name), keyCodes(keyCodes) {}

    std::string const & GetName() const { return name; }
    std::vector<Keycode> const & GetKeyCodes() const { return keyCodes; }

private:
    std::string name;
    std::vector<Keycode> keyCodes;
};