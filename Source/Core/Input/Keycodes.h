#pragma once
#include <string>
#include <unordered_map>
#include <vector>

typedef enum {
    KC_0,                  // 0
    KC_1,                  // 1
    KC_2,                  // 2
    KC_3,                  // 3
    KC_4,                  // 4
    KC_5,                  // 5
    KC_6,                  // 6
    KC_7,                  // 7
    KC_8,                  // 8
    KC_9,                  // 9
    KC_a,                  // A
    KC_ACBACK,             // AC Back
    KC_ACBOOKMARKS,        // AC Bookmarks
    KC_ACFORWARD,          // AC Forward
    KC_ACHOME,             // AC Home
    KC_ACREFRESH,          // AC Refresh
    KC_ACSEARCH,           // AC Search
    KC_ACSTOP,             // AC Stop
    KC_AGAIN,              // Again
    KC_ALTERASE,           // AltErase
    KC_QUOTE,              // '
    KC_APPLICATION,        // Application
    KC_AUDIOMUTE,          // AudioMute
    KC_AUDIONEXT,          // AudioNext
    KC_AUDIOPLAY,          // AudioPlay
    KC_AUDIOPREV,          // AudioPrev
    KC_AUDIOSTOP,          // AudioStop
    KC_b,                  // B
    KC_BACKSLASH,          // Backslash
    KC_BACKSPACE,          // Backspace
    KC_BRIGHTNESSDOWN,     // BrightnessDown
    KC_BRIGHTNESSUP,       // BrightnessUp
    KC_c,                  // C
    KC_CALCULATOR,         // Calculator
    KC_CANCEL,             // Cancel
    KC_CAPSLOCK,           // CapsLock
    KC_CLEAR,              // Clear
    KC_CLEARAGAIN,         // Clear / Again
    KC_COMMA,              // ,
    KC_COMPUTER,           // Computer
    KC_COPY,               // Copy
    KC_CRSEL,              // CrSel
    KC_CURRENCYSUBUNIT,    // CurrencySubUnit
    KC_CURRENCYUNIT,       // CurrencyUnit
    KC_CUT,                // Cut
    KC_d,                  // D
    KC_DECIMALSEPARATOR,   // DecimalSeparator
    KC_DELETE,             // Delete
    KC_DISPLAYSWITCH,      // DisplaySwitch
    KC_DOWN,               // Down
    KC_e,                  // E
    KC_EJECT,              // Eject
    KC_END,                // End
    KC_EQUALS,             // =
    KC_ESCAPE,             // Escape
    KC_EXECUTE,            // Execute
    KC_EXSEL,              // ExSel
    KC_f,                  // F
    KC_F1,                 // F1
    KC_F10,                // F10
    KC_F11,                // F11
    KC_F12,                // F12
    KC_F13,                // F13
    KC_F14,                // F14
    KC_F15,                // F15
    KC_F16,                // F16
    KC_F17,                // F17
    KC_F18,                // F18
    KC_F19,                // F19
    KC_F2,                 // F2
    KC_F20,                // F20
    KC_F21,                // F21
    KC_F22,                // F22
    KC_F23,                // F23
    KC_F24,                // F24
    KC_F3,                 // F3
    KC_F4,                 // F4
    KC_F5,                 // F5
    KC_F6,                 // F6
    KC_F7,                 // F7
    KC_F8,                 // F8
    KC_F9,                 // F9
    KC_FIND,               // Find
    KC_g,                  // G
    KC_BACKQUOTE,          // `
    KC_h,                  // H
    KC_HELP,               // Help
    KC_HOME,               // Home
    KC_i,                  // I
    KC_INSERT,             // Insert
    KC_j,                  // J
    KC_k,                  // K
    KC_KBDILLUMDOWN,       // KBDIllumDown
    KC_KBDILLUMTOGGLE,     // KBDIllumToggle
    KC_KBDILLUMUP,         // KBDIllumUp
    KC_KP0,                // Keypad 0
    KC_KP00,               // Keypad 00
    KC_KP000,              // Keypad 000
    KC_KP1,                // Keypad 1
    KC_KP2,                // Keypad 2
    KC_KP3,                // Keypad 3
    KC_KP4,                // Keypad 4
    KC_KP5,                // Keypad 5
    KC_KP6,                // Keypad 6
    KC_KP7,                // Keypad 7
    KC_KP8,                // Keypad 8
    KC_KP9,                // Keypad 9
    KC_KPA,                // Keypad A
    KC_KPAMPERSAND,        // Keypad &
    KC_KPAT,               // Keypad @
    KC_KPB,                // Keypad B
    KC_KPBACKSPACE,        // Keypad Backspace
    KC_KPBINARY,           // Keypad Binary
    KC_KPC,                // Keypad C
    KC_KPCLEAR,            // Keypad Clear
    KC_KPCLEARENTRY,       // Keypad ClearEntry
    KC_KPCOLON,            // Keypad :
    KC_KPCOMMA,            // Keypad ,
    KC_KPD,                // Keypad D
    KC_KPDBLAMPERSAND,     // Keypad &&
    KC_KPDBLVERTICALBAR,   // Keypad ||
    KC_KPDECIMAL,          // Keypad Decimal
    KC_KPDIVIDE,           // Keypad /
    KC_KPE,                // Keypad E
    KC_KPENTER,            // Keypad Enter
    KC_KPEQUALS,           // Keypad =
    KC_KPEQUALSAS400,      // Keypad = (AS400)
    KC_KPEXCLAM,           // Keypad !
    KC_KPF,                // Keypad F
    KC_KPGREATER,          // Keypad >
    KC_KPHASH,             // Keypad #
    KC_KPHEXADECIMAL,      // Keypad Hexadecimal
    KC_KPLEFTBRACE,        // Keypad {
    KC_KPLEFTPAREN,        // Keypad (
    KC_KPLESS,             // Keypad <
    KC_KPMEMADD,           // Keypad MemAdd
    KC_KPMEMCLEAR,         // Keypad MemClear
    KC_KPMEMDIVIDE,        // Keypad MemDivide
    KC_KPMEMMULTIPLY,      // Keypad MemMultiply
    KC_KPMEMRECALL,        // Keypad MemRecall
    KC_KPMEMSTORE,         // Keypad MemStore
    KC_KPMEMSUBTRACT,      // Keypad MemSubtract
    KC_KPMINUS,            // Keypad -
    KC_KPMULTIPLY,         // Keypad *
    KC_KPOCTAL,            // Keypad Octal
    KC_KPPERCENT,          // Keypad %
    KC_KPPERIOD,           // Keypad .
    KC_KPPLUS,             // Keypad +
    KC_KPPLUSMINUS,        // Keypad +/-
    KC_KPPOWER,            // Keypad ^
    KC_KPRIGHTBRACE,       // Keypad }
    KC_KPRIGHTPAREN,       // Keypad )
    KC_KPSPACE,            // Keypad Space
    KC_KPTAB,              // Keypad Tab
    KC_KPVERTICALBAR,      // Keypad |
    KC_KPXOR,              // Keypad XOR
    KC_l,                  // L
    KC_LALT,               // Left Alt
    KC_LCTRL,              // Left Ctrl
    KC_LEFT,               // Left
    KC_LEFTBRACKET,        // [
    KC_LGUI,               // Left GUI
    KC_LSHIFT,             // Left Shift
    KC_m,                  // M
    KC_MAIL,               // Mail
    KC_MEDIASELECT,        // MediaSelect
    KC_MENU,               // Menu
    KC_MINUS,              // -
    KC_MODE,               // ModeSwitch
    KC_MUTE,               // Mute
    KC_n,                  // N
    KC_NUMLOCKCLEAR,       // Numlock
    KC_o,                  // O
    KC_OPER,               // Oper
    KC_OUT,                // Out
    KC_p,                  // P
    KC_PAGEDOWN,           // PageDown
    KC_PAGEUP,             // PageUp
    KC_PASTE,              // Paste
    KC_PAUSE,              // Pause
    KC_PERIOD,             // .
    KC_POWER,              // Power
    KC_PRINTSCREEN,        // PrintScreen
    KC_PRIOR,              // Prior
    KC_q,                  // Q
    KC_r,                  // R
    KC_RALT,               // Right Alt
    KC_RCTRL,              // Right Ctrl
    KC_RETURN,             // Return
    KC_RETURN2,            // Return
    KC_RGUI,               // Right GUI
    KC_RIGHT,              // Right
    KC_RIGHTBRACKET,       // ]
    KC_RSHIFT,             // Right Shift
    KC_s,                  // S
    KC_SCROLLLOCK,         // ScrollLock
    KC_SELECT,             // Select
    KC_SEMICOLON,          // ;
    KC_SEPARATOR,          // Separator
    KC_SLASH,              // /
    KC_SLEEP,              // Sleep
    KC_SPACE,              // Space
    KC_STOP,               // Stop
    KC_SYSREQ,             // SysReq
    KC_t,                  // T
    KC_TAB,                // Tab
    KC_THOUSANDSSEPARATOR, // ThousandsSeparator
    KC_u,                  // U
    KC_UNDO,               // Undo
    KC_UP,                 // Up
    KC_v,                  // V
    KC_VOLUMEDOWN,         // VolumeDown
    KC_VOLUMEUP,           // VolumeUp
    KC_w,                  // W
    KC_WWW,                // WWW
    KC_x,                  // X
    KC_y,                  // Y
    KC_z,                  // Z

    KC_MOUSE_LEFT,
    KC_MOUSE_MIDDLE,
    KC_MOUSE_RIGHT,
    // For mice with more buttons than 3. Kinda dislike the name.
    KC_MOUSE_X1,
    KC_MOUSE_X2,
} Keycode;

extern std::unordered_map<std::string, Keycode> strToKeycode;
extern std::unordered_map<Keycode, std::string, std::hash<int>> keycodeToStr;

std::vector<std::string> const & GetAllKeycodeStrings();
