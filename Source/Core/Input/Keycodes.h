#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "SDL2/SDL.h"

#define MOUSE_MASK (1 << 28)
#define MOUSE_TO_KEYCODE(X) (X | MOUSE_MASK)

typedef enum {
    KC_0 = SDLK_0,                                   // 0
    KC_1 = SDLK_1,                                   // 1
    KC_2 = SDLK_2,                                   // 2
    KC_3 = SDLK_3,                                   // 3
    KC_4 = SDLK_4,                                   // 4
    KC_5 = SDLK_5,                                   // 5
    KC_6 = SDLK_6,                                   // 6
    KC_7 = SDLK_7,                                   // 7
    KC_8 = SDLK_8,                                   // 8
    KC_9 = SDLK_9,                                   // 9
    KC_a = SDLK_a,                                   // A
    KC_ACBACK = SDLK_AC_BACK,                        // AC Back
    KC_ACBOOKMARKS = SDLK_AC_BOOKMARKS,              // AC Bookmarks
    KC_ACFORWARD = SDLK_AC_FORWARD,                  // AC Forward
    KC_ACHOME = SDLK_AC_HOME,                        // AC Home
    KC_ACREFRESH = SDLK_AC_REFRESH,                  // AC Refresh
    KC_ACSEARCH = SDLK_AC_SEARCH,                    // AC Search
    KC_ACSTOP = SDLK_AC_STOP,                        // AC Stop
    KC_AGAIN = SDLK_AGAIN,                           // Again
    KC_ALTERASE = SDLK_ALTERASE,                     // AltErase
    KC_QUOTE = SDLK_QUOTE,                           // '
    KC_APPLICATION = SDLK_APPLICATION,               // Application
    KC_AUDIOMUTE = SDLK_AUDIOMUTE,                   // AudioMute
    KC_AUDIONEXT = SDLK_AUDIONEXT,                   // AudioNext
    KC_AUDIOPLAY = SDLK_AUDIOPLAY,                   // AudioPlay
    KC_AUDIOPREV = SDLK_AUDIOPREV,                   // AudioPrev
    KC_AUDIOSTOP = SDLK_AUDIOSTOP,                   // AudioStop
    KC_b = SDLK_b,                                   // B
    KC_BACKSLASH = SDLK_BACKSLASH,                   // Backslash
    KC_BACKSPACE = SDLK_BACKSPACE,                   // Backspace
    KC_BRIGHTNESSDOWN = SDLK_BRIGHTNESSDOWN,         // BrightnessDown
    KC_BRIGHTNESSUP = SDLK_BRIGHTNESSUP,             // BrightnessUp
    KC_c = SDLK_c,                                   // C
    KC_CALCULATOR = SDLK_CALCULATOR,                 // Calculator
    KC_CANCEL = SDLK_CANCEL,                         // Cancel
    KC_CAPSLOCK = SDLK_CAPSLOCK,                     // CapsLock
    KC_CLEAR = SDLK_CLEAR,                           // Clear
    KC_CLEARAGAIN = SDLK_CLEARAGAIN,                 // Clear / Again
    KC_COMMA = SDLK_COMMA,                           // ,
    KC_COMPUTER = SDLK_COMPUTER,                     // Computer
    KC_COPY = SDLK_COPY,                             // Copy
    KC_CRSEL = SDLK_CRSEL,                           // CrSel
    KC_CURRENCYSUBUNIT = SDLK_CURRENCYSUBUNIT,       // CurrencySubUnit
    KC_CURRENCYUNIT = SDLK_CURRENCYUNIT,             // CurrencyUnit
    KC_CUT = SDLK_CUT,                               // Cut
    KC_d = SDLK_d,                                   // D
    KC_DECIMALSEPARATOR = SDLK_DECIMALSEPARATOR,     // DecimalSeparator
    KC_DELETE = SDLK_DELETE,                         // Delete
    KC_DISPLAYSWITCH = SDLK_DISPLAYSWITCH,           // DisplaySwitch
    KC_DOWN = SDLK_DOWN,                             // Down
    KC_e = SDLK_e,                                   // E
    KC_EJECT = SDLK_EJECT,                           // Eject
    KC_END = SDLK_END,                               // End
    KC_EQUALS = SDLK_EQUALS,                         // =
    KC_ESCAPE = SDLK_ESCAPE,                         // Escape
    KC_EXECUTE = SDLK_EXECUTE,                       // Execute
    KC_EXSEL = SDLK_EXSEL,                           // ExSel
    KC_f = SDLK_f,                                   // F
    KC_F1 = SDLK_F1,                                 // F1
    KC_F10 = SDLK_F10,                               // F10
    KC_F11 = SDLK_F11,                               // F11
    KC_F12 = SDLK_F12,                               // F12
    KC_F13 = SDLK_F13,                               // F13
    KC_F14 = SDLK_F14,                               // F14
    KC_F15 = SDLK_F15,                               // F15
    KC_F16 = SDLK_F16,                               // F16
    KC_F17 = SDLK_F17,                               // F17
    KC_F18 = SDLK_F18,                               // F18
    KC_F19 = SDLK_F19,                               // F19
    KC_F2 = SDLK_F2,                                 // F2
    KC_F20 = SDLK_F20,                               // F20
    KC_F21 = SDLK_F21,                               // F21
    KC_F22 = SDLK_F22,                               // F22
    KC_F23 = SDLK_F23,                               // F23
    KC_F24 = SDLK_F24,                               // F24
    KC_F3 = SDLK_F3,                                 // F3
    KC_F4 = SDLK_F4,                                 // F4
    KC_F5 = SDLK_F5,                                 // F5
    KC_F6 = SDLK_F6,                                 // F6
    KC_F7 = SDLK_F7,                                 // F7
    KC_F8 = SDLK_F8,                                 // F8
    KC_F9 = SDLK_F9,                                 // F9
    KC_FIND = SDLK_FIND,                             // Find
    KC_g = SDLK_g,                                   // G
    KC_BACKQUOTE = SDLK_BACKQUOTE,                   // `
    KC_h = SDLK_h,                                   // H
    KC_HELP = SDLK_HELP,                             // Help
    KC_HOME = SDLK_HOME,                             // Home
    KC_i = SDLK_i,                                   // I
    KC_INSERT = SDLK_INSERT,                         // Insert
    KC_j = SDLK_j,                                   // J
    KC_k = SDLK_k,                                   // K
    KC_KBDILLUMDOWN = SDLK_KBDILLUMDOWN,             // KBDIllumDown
    KC_KBDILLUMTOGGLE = SDLK_KBDILLUMTOGGLE,         // KBDIllumToggle
    KC_KBDILLUMUP = SDLK_KBDILLUMUP,                 // KBDIllumUp
    KC_KP0 = SDLK_KP_0,                              // Keypad 0
    KC_KP00 = SDLK_KP_00,                            // Keypad 00
    KC_KP000 = SDLK_KP_000,                          // Keypad 000
    KC_KP1 = SDLK_KP_1,                              // Keypad 1
    KC_KP2 = SDLK_KP_2,                              // Keypad 2
    KC_KP3 = SDLK_KP_3,                              // Keypad 3
    KC_KP4 = SDLK_KP_4,                              // Keypad 4
    KC_KP5 = SDLK_KP_5,                              // Keypad 5
    KC_KP6 = SDLK_KP_6,                              // Keypad 6
    KC_KP7 = SDLK_KP_7,                              // Keypad 7
    KC_KP8 = SDLK_KP_8,                              // Keypad 8
    KC_KP9 = SDLK_KP_9,                              // Keypad 9
    KC_KPA = SDLK_KP_A,                              // Keypad A
    KC_KPAMPERSAND = SDLK_KP_AMPERSAND,              // Keypad &
    KC_KPAT = SDLK_KP_AT,                            // Keypad @
    KC_KPB = SDLK_KP_B,                              // Keypad B
    KC_KPBACKSPACE = SDLK_KP_BACKSPACE,              // Keypad Backspace
    KC_KPBINARY = SDLK_KP_BINARY,                    // Keypad Binary
    KC_KPC = SDLK_KP_C,                              // Keypad C
    KC_KPCLEAR = SDLK_KP_CLEAR,                      // Keypad Clear
    KC_KPCLEARENTRY = SDLK_KP_CLEARENTRY,            // Keypad ClearEntry
    KC_KPCOLON = SDLK_KP_COLON,                      // Keypad :
    KC_KPCOMMA = SDLK_KP_COMMA,                      // Keypad ,
    KC_KPD = SDLK_KP_D,                              // Keypad D
    KC_KPDBLAMPERSAND = SDLK_KP_DBLAMPERSAND,        // Keypad &&
    KC_KPDBLVERTICALBAR = SDLK_KP_DBLVERTICALBAR,    // Keypad ||
    KC_KPDECIMAL = SDLK_KP_DECIMAL,                  // Keypad Decimal
    KC_KPDIVIDE = SDLK_KP_DIVIDE,                    // Keypad /
    KC_KPE = SDLK_KP_E,                              // Keypad E
    KC_KPENTER = SDLK_KP_ENTER,                      // Keypad Enter
    KC_KPEQUALS = SDLK_KP_EQUALS,                    // Keypad =
    KC_KPEQUALSAS400 = SDLK_KP_EQUALSAS400,          // Keypad = (AS400)
    KC_KPEXCLAM = SDLK_KP_EXCLAM,                    // Keypad !
    KC_KPF = SDLK_KP_F,                              // Keypad F
    KC_KPGREATER = SDLK_KP_GREATER,                  // Keypad >
    KC_KPHASH = SDLK_KP_HASH,                        // Keypad #
    KC_KPHEXADECIMAL = SDLK_KP_HEXADECIMAL,          // Keypad Hexadecimal
    KC_KPLEFTBRACE = SDLK_KP_LEFTBRACE,              // Keypad {
    KC_KPLEFTPAREN = SDLK_KP_LEFTPAREN,              // Keypad (
    KC_KPLESS = SDLK_KP_LESS,                        // Keypad <
    KC_KPMEMADD = SDLK_KP_MEMADD,                    // Keypad MemAdd
    KC_KPMEMCLEAR = SDLK_KP_MEMCLEAR,                // Keypad MemClear
    KC_KPMEMDIVIDE = SDLK_KP_MEMDIVIDE,              // Keypad MemDivide
    KC_KPMEMMULTIPLY = SDLK_KP_MEMMULTIPLY,          // Keypad MemMultiply
    KC_KPMEMRECALL = SDLK_KP_MEMRECALL,              // Keypad MemRecall
    KC_KPMEMSTORE = SDLK_KP_MEMSTORE,                // Keypad MemStore
    KC_KPMEMSUBTRACT = SDLK_KP_MEMSUBTRACT,          // Keypad MemSubtract
    KC_KPMINUS = SDLK_KP_MINUS,                      // Keypad -
    KC_KPMULTIPLY = SDLK_KP_MULTIPLY,                // Keypad *
    KC_KPOCTAL = SDLK_KP_OCTAL,                      // Keypad Octal
    KC_KPPERCENT = SDLK_KP_PERCENT,                  // Keypad %
    KC_KPPERIOD = SDLK_KP_PERIOD,                    // Keypad .
    KC_KPPLUS = SDLK_KP_PLUS,                        // Keypad +
    KC_KPPLUSMINUS = SDLK_KP_PLUSMINUS,              // Keypad +/-
    KC_KPPOWER = SDLK_KP_POWER,                      // Keypad ^
    KC_KPRIGHTBRACE = SDLK_KP_RIGHTBRACE,            // Keypad }
    KC_KPRIGHTPAREN = SDLK_KP_RIGHTPAREN,            // Keypad )
    KC_KPSPACE = SDLK_KP_SPACE,                      // Keypad Space
    KC_KPTAB = SDLK_KP_TAB,                          // Keypad Tab
    KC_KPVERTICALBAR = SDLK_KP_VERTICALBAR,          // Keypad |
    KC_KPXOR = SDLK_KP_XOR,                          // Keypad XOR
    KC_l = SDLK_l,                                   // L
    KC_LALT = SDLK_LALT,                             // Left Alt
    KC_LCTRL = SDLK_LCTRL,                           // Left Ctrl
    KC_LEFT = SDLK_LEFT,                             // Left
    KC_LEFTBRACKET = SDLK_LEFTBRACKET,               // [
    KC_LGUI = SDLK_LGUI,                             // Left GUI
    KC_LSHIFT = SDLK_LSHIFT,                         // Left Shift
    KC_m = SDLK_m,                                   // M
    KC_MAIL = SDLK_MAIL,                             // Mail
    KC_MEDIASELECT = SDLK_MEDIASELECT,               // MediaSelect
    KC_MENU = SDLK_MENU,                             // Menu
    KC_MINUS = SDLK_MINUS,                           // -
    KC_MODE = SDLK_MODE,                             // ModeSwitch
    KC_MUTE = SDLK_MUTE,                             // Mute
    KC_n = SDLK_n,                                   // N
    KC_NUMLOCKCLEAR = SDLK_NUMLOCKCLEAR,             // Numlock
    KC_o = SDLK_o,                                   // O
    KC_OPER = SDLK_OPER,                             // Oper
    KC_OUT = SDLK_OUT,                               // Out
    KC_p = SDLK_p,                                   // P
    KC_PAGEDOWN = SDLK_PAGEDOWN,                     // PageDown
    KC_PAGEUP = SDLK_PAGEUP,                         // PageUp
    KC_PASTE = SDLK_PASTE,                           // Paste
    KC_PAUSE = SDLK_PAUSE,                           // Pause
    KC_PERIOD = SDLK_PERIOD,                         // .
    KC_POWER = SDLK_POWER,                           // Power
    KC_PRINTSCREEN = SDLK_PRINTSCREEN,               // PrintScreen
    KC_PRIOR = SDLK_PRIOR,                           // Prior
    KC_q = SDLK_q,                                   // Q
    KC_r = SDLK_r,                                   // R
    KC_RALT = SDLK_RALT,                             // Right Alt
    KC_RCTRL = SDLK_RCTRL,                           // Right Ctrl
    KC_RETURN = SDLK_RETURN,                         // Return
    KC_RETURN2 = SDLK_RETURN2,                       // Return
    KC_RGUI = SDLK_RGUI,                             // Right GUI
    KC_RIGHT = SDLK_RIGHT,                           // Right
    KC_RIGHTBRACKET = SDLK_RIGHTBRACKET,             // ]
    KC_RSHIFT = SDLK_RSHIFT,                         // Right Shift
    KC_s = SDLK_s,                                   // S
    KC_SCROLLLOCK = SDLK_SCROLLLOCK,                 // ScrollLock
    KC_SELECT = SDLK_SELECT,                         // Select
    KC_SEMICOLON = SDLK_SEMICOLON,                   // ;
    KC_SEPARATOR = SDLK_SEPARATOR,                   // Separator
    KC_SLASH = SDLK_SLASH,                           // /
    KC_SLEEP = SDLK_SLEEP,                           // Sleep
    KC_SPACE = SDLK_SPACE,                           // Space
    KC_STOP = SDLK_STOP,                             // Stop
    KC_SYSREQ = SDLK_SYSREQ,                         // SysReq
    KC_t = SDLK_t,                                   // T
    KC_TAB = SDLK_TAB,                               // Tab
    KC_THOUSANDSSEPARATOR = SDLK_THOUSANDSSEPARATOR, // ThousandsSeparator
    KC_u = SDLK_u,                                   // U
    KC_UNDO = SDLK_UNDO,                             // Undo
    KC_UP = SDLK_UP,                                 // Up
    KC_v = SDLK_v,                                   // V
    KC_VOLUMEDOWN = SDLK_VOLUMEDOWN,                 // VolumeDown
    KC_VOLUMEUP = SDLK_VOLUMEUP,                     // VolumeUp
    KC_w = SDLK_w,                                   // W
    KC_WWW = SDLK_WWW,                               // WWW
    KC_x = SDLK_x,                                   // X
    KC_y = SDLK_y,                                   // Y
    KC_z = SDLK_z,                                   // Z

    KC_MOUSE_LEFT = MOUSE_TO_KEYCODE(SDL_BUTTON_LEFT),
    KC_MOUSE_MIDDLE = MOUSE_TO_KEYCODE(SDL_BUTTON_MIDDLE),
    KC_MOUSE_RIGHT = MOUSE_TO_KEYCODE(SDL_BUTTON_RIGHT),
    // For mice with more buttons than 3. Kinda dislike the name.
    KC_MOUSE_X1 = MOUSE_TO_KEYCODE(SDL_BUTTON_X1),
    KC_MOUSE_X2 = MOUSE_TO_KEYCODE(SDL_BUTTON_X2),

} Keycode;

extern std::unordered_map<std::string, Keycode> strToKeycode;
extern std::unordered_map<Keycode, std::string, std::hash<int>> keycodeToStr;

std::vector<std::string> const & GetAllKeycodeStrings();
