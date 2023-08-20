#pragma once

#include <cassert>

#include <ThirdParty/SDL2/include/SDL.h>

#include "Keycodes.h"

#define MOUSE_MASK (1 << 28)
#define MOUSE_TO_KEYCODE(X) (X | MOUSE_MASK)

SDL_Keycode KeycodeToSdlKeycode(Keycode kc)
{
    switch (kc) {
    case Keycode::KC_0:
        return SDLK_0; // 0
    case Keycode::KC_1:
        return SDLK_1; // 1
    case Keycode::KC_2:
        return SDLK_2; // 2
    case Keycode::KC_3:
        return SDLK_3; // 3
    case Keycode::KC_4:
        return SDLK_4; // 4
    case Keycode::KC_5:
        return SDLK_5; // 5
    case Keycode::KC_6:
        return SDLK_6; // 6
    case Keycode::KC_7:
        return SDLK_7; // 7
    case Keycode::KC_8:
        return SDLK_8; // 8
    case Keycode::KC_9:
        return SDLK_9; // 9
    case Keycode::KC_a:
        return SDLK_a; // A
    case Keycode::KC_ACBACK:
        return SDLK_AC_BACK; // AC Back
    case Keycode::KC_ACBOOKMARKS:
        return SDLK_AC_BOOKMARKS; // AC Bookmarks
    case Keycode::KC_ACFORWARD:
        return SDLK_AC_FORWARD; // AC Forward
    case Keycode::KC_ACHOME:
        return SDLK_AC_HOME; // AC Home
    case Keycode::KC_ACREFRESH:
        return SDLK_AC_REFRESH; // AC Refresh
    case Keycode::KC_ACSEARCH:
        return SDLK_AC_SEARCH; // AC Search
    case Keycode::KC_ACSTOP:
        return SDLK_AC_STOP; // AC Stop
    case Keycode::KC_AGAIN:
        return SDLK_AGAIN; // Again
    case Keycode::KC_ALTERASE:
        return SDLK_ALTERASE; // AltErase
    case Keycode::KC_QUOTE:
        return SDLK_QUOTE; // '
    case Keycode::KC_APPLICATION:
        return SDLK_APPLICATION; // Application
    case Keycode::KC_AUDIOMUTE:
        return SDLK_AUDIOMUTE; // AudioMute
    case Keycode::KC_AUDIONEXT:
        return SDLK_AUDIONEXT; // AudioNext
    case Keycode::KC_AUDIOPLAY:
        return SDLK_AUDIOPLAY; // AudioPlay
    case Keycode::KC_AUDIOPREV:
        return SDLK_AUDIOPREV; // AudioPrev
    case Keycode::KC_AUDIOSTOP:
        return SDLK_AUDIOSTOP; // AudioStop
    case Keycode::KC_b:
        return SDLK_b; // B
    case Keycode::KC_BACKSLASH:
        return SDLK_BACKSLASH; // Backslash
    case Keycode::KC_BACKSPACE:
        return SDLK_BACKSPACE; // Backspace
    case Keycode::KC_BRIGHTNESSDOWN:
        return SDLK_BRIGHTNESSDOWN; // BrightnessDown
    case Keycode::KC_BRIGHTNESSUP:
        return SDLK_BRIGHTNESSUP; // BrightnessUp
    case Keycode::KC_c:
        return SDLK_c; // C
    case Keycode::KC_CALCULATOR:
        return SDLK_CALCULATOR; // Calculator
    case Keycode::KC_CANCEL:
        return SDLK_CANCEL; // Cancel
    case Keycode::KC_CAPSLOCK:
        return SDLK_CAPSLOCK; // CapsLock
    case Keycode::KC_CLEAR:
        return SDLK_CLEAR; // Clear
    case Keycode::KC_CLEARAGAIN:
        return SDLK_CLEARAGAIN; // Clear / Again
    case Keycode::KC_COMMA:
        return SDLK_COMMA; // ,
    case Keycode::KC_COMPUTER:
        return SDLK_COMPUTER; // Computer
    case Keycode::KC_COPY:
        return SDLK_COPY; // Copy
    case Keycode::KC_CRSEL:
        return SDLK_CRSEL; // CrSel
    case Keycode::KC_CURRENCYSUBUNIT:
        return SDLK_CURRENCYSUBUNIT; // CurrencySubUnit
    case Keycode::KC_CURRENCYUNIT:
        return SDLK_CURRENCYUNIT; // CurrencyUnit
    case Keycode::KC_CUT:
        return SDLK_CUT; // Cut
    case Keycode::KC_d:
        return SDLK_d; // D
    case Keycode::KC_DECIMALSEPARATOR:
        return SDLK_DECIMALSEPARATOR; // DecimalSeparator
    case Keycode::KC_DELETE:
        return SDLK_DELETE; // Delete
    case Keycode::KC_DISPLAYSWITCH:
        return SDLK_DISPLAYSWITCH; // DisplaySwitch
    case Keycode::KC_DOWN:
        return SDLK_DOWN; // Down
    case Keycode::KC_e:
        return SDLK_e; // E
    case Keycode::KC_EJECT:
        return SDLK_EJECT; // Eject
    case Keycode::KC_END:
        return SDLK_END; // End
    case Keycode::KC_EQUALS:
        return SDLK_EQUALS; // =
    case Keycode::KC_ESCAPE:
        return SDLK_ESCAPE; // Escape
    case Keycode::KC_EXECUTE:
        return SDLK_EXECUTE; // Execute
    case Keycode::KC_EXSEL:
        return SDLK_EXSEL; // ExSel
    case Keycode::KC_f:
        return SDLK_f; // F
    case Keycode::KC_F1:
        return SDLK_F1; // F1
    case Keycode::KC_F10:
        return SDLK_F10; // F10
    case Keycode::KC_F11:
        return SDLK_F11; // F11
    case Keycode::KC_F12:
        return SDLK_F12; // F12
    case Keycode::KC_F13:
        return SDLK_F13; // F13
    case Keycode::KC_F14:
        return SDLK_F14; // F14
    case Keycode::KC_F15:
        return SDLK_F15; // F15
    case Keycode::KC_F16:
        return SDLK_F16; // F16
    case Keycode::KC_F17:
        return SDLK_F17; // F17
    case Keycode::KC_F18:
        return SDLK_F18; // F18
    case Keycode::KC_F19:
        return SDLK_F19; // F19
    case Keycode::KC_F2:
        return SDLK_F2; // F2
    case Keycode::KC_F20:
        return SDLK_F20; // F20
    case Keycode::KC_F21:
        return SDLK_F21; // F21
    case Keycode::KC_F22:
        return SDLK_F22; // F22
    case Keycode::KC_F23:
        return SDLK_F23; // F23
    case Keycode::KC_F24:
        return SDLK_F24; // F24
    case Keycode::KC_F3:
        return SDLK_F3; // F3
    case Keycode::KC_F4:
        return SDLK_F4; // F4
    case Keycode::KC_F5:
        return SDLK_F5; // F5
    case Keycode::KC_F6:
        return SDLK_F6; // F6
    case Keycode::KC_F7:
        return SDLK_F7; // F7
    case Keycode::KC_F8:
        return SDLK_F8; // F8
    case Keycode::KC_F9:
        return SDLK_F9; // F9
    case Keycode::KC_FIND:
        return SDLK_FIND; // Find
    case Keycode::KC_g:
        return SDLK_g; // G
    case Keycode::KC_BACKQUOTE:
        return SDLK_BACKQUOTE; // `
    case Keycode::KC_h:
        return SDLK_h; // H
    case Keycode::KC_HELP:
        return SDLK_HELP; // Help
    case Keycode::KC_HOME:
        return SDLK_HOME; // Home
    case Keycode::KC_i:
        return SDLK_i; // I
    case Keycode::KC_INSERT:
        return SDLK_INSERT; // Insert
    case Keycode::KC_j:
        return SDLK_j; // J
    case Keycode::KC_k:
        return SDLK_k; // K
    case Keycode::KC_KBDILLUMDOWN:
        return SDLK_KBDILLUMDOWN; // KBDIllumDown
    case Keycode::KC_KBDILLUMTOGGLE:
        return SDLK_KBDILLUMTOGGLE; // KBDIllumToggle
    case Keycode::KC_KBDILLUMUP:
        return SDLK_KBDILLUMUP; // KBDIllumUp
    case Keycode::KC_KP0:
        return SDLK_KP_0; // Keypad 0
    case Keycode::KC_KP00:
        return SDLK_KP_00; // Keypad 00
    case Keycode::KC_KP000:
        return SDLK_KP_000; // Keypad 000
    case Keycode::KC_KP1:
        return SDLK_KP_1; // Keypad 1
    case Keycode::KC_KP2:
        return SDLK_KP_2; // Keypad 2
    case Keycode::KC_KP3:
        return SDLK_KP_3; // Keypad 3
    case Keycode::KC_KP4:
        return SDLK_KP_4; // Keypad 4
    case Keycode::KC_KP5:
        return SDLK_KP_5; // Keypad 5
    case Keycode::KC_KP6:
        return SDLK_KP_6; // Keypad 6
    case Keycode::KC_KP7:
        return SDLK_KP_7; // Keypad 7
    case Keycode::KC_KP8:
        return SDLK_KP_8; // Keypad 8
    case Keycode::KC_KP9:
        return SDLK_KP_9; // Keypad 9
    case Keycode::KC_KPA:
        return SDLK_KP_A; // Keypad A
    case Keycode::KC_KPAMPERSAND:
        return SDLK_KP_AMPERSAND; // Keypad &
    case Keycode::KC_KPAT:
        return SDLK_KP_AT; // Keypad @
    case Keycode::KC_KPB:
        return SDLK_KP_B; // Keypad B
    case Keycode::KC_KPBACKSPACE:
        return SDLK_KP_BACKSPACE; // Keypad Backspace
    case Keycode::KC_KPBINARY:
        return SDLK_KP_BINARY; // Keypad Binary
    case Keycode::KC_KPC:
        return SDLK_KP_C; // Keypad C
    case Keycode::KC_KPCLEAR:
        return SDLK_KP_CLEAR; // Keypad Clear
    case Keycode::KC_KPCLEARENTRY:
        return SDLK_KP_CLEARENTRY; // Keypad ClearEntry
    case Keycode::KC_KPCOLON:
        return SDLK_KP_COLON; // Keypad :
    case Keycode::KC_KPCOMMA:
        return SDLK_KP_COMMA; // Keypad ,
    case Keycode::KC_KPD:
        return SDLK_KP_D; // Keypad D
    case Keycode::KC_KPDBLAMPERSAND:
        return SDLK_KP_DBLAMPERSAND; // Keypad &&
    case Keycode::KC_KPDBLVERTICALBAR:
        return SDLK_KP_DBLVERTICALBAR; // Keypad ||
    case Keycode::KC_KPDECIMAL:
        return SDLK_KP_DECIMAL; // Keypad Decimal
    case Keycode::KC_KPDIVIDE:
        return SDLK_KP_DIVIDE; // Keypad /
    case Keycode::KC_KPE:
        return SDLK_KP_E; // Keypad E
    case Keycode::KC_KPENTER:
        return SDLK_KP_ENTER; // Keypad Enter
    case Keycode::KC_KPEQUALS:
        return SDLK_KP_EQUALS; // Keypad =
    case Keycode::KC_KPEQUALSAS400:
        return SDLK_KP_EQUALSAS400; // Keypad = (AS400)
    case Keycode::KC_KPEXCLAM:
        return SDLK_KP_EXCLAM; // Keypad !
    case Keycode::KC_KPF:
        return SDLK_KP_F; // Keypad F
    case Keycode::KC_KPGREATER:
        return SDLK_KP_GREATER; // Keypad >
    case Keycode::KC_KPHASH:
        return SDLK_KP_HASH; // Keypad #
    case Keycode::KC_KPHEXADECIMAL:
        return SDLK_KP_HEXADECIMAL; // Keypad Hexadecimal
    case Keycode::KC_KPLEFTBRACE:
        return SDLK_KP_LEFTBRACE; // Keypad {
    case Keycode::KC_KPLEFTPAREN:
        return SDLK_KP_LEFTPAREN; // Keypad (
    case Keycode::KC_KPLESS:
        return SDLK_KP_LESS; // Keypad <
    case Keycode::KC_KPMEMADD:
        return SDLK_KP_MEMADD; // Keypad MemAdd
    case Keycode::KC_KPMEMCLEAR:
        return SDLK_KP_MEMCLEAR; // Keypad MemClear
    case Keycode::KC_KPMEMDIVIDE:
        return SDLK_KP_MEMDIVIDE; // Keypad MemDivide
    case Keycode::KC_KPMEMMULTIPLY:
        return SDLK_KP_MEMMULTIPLY; // Keypad MemMultiply
    case Keycode::KC_KPMEMRECALL:
        return SDLK_KP_MEMRECALL; // Keypad MemRecall
    case Keycode::KC_KPMEMSTORE:
        return SDLK_KP_MEMSTORE; // Keypad MemStore
    case Keycode::KC_KPMEMSUBTRACT:
        return SDLK_KP_MEMSUBTRACT; // Keypad MemSubtract
    case Keycode::KC_KPMINUS:
        return SDLK_KP_MINUS; // Keypad -
    case Keycode::KC_KPMULTIPLY:
        return SDLK_KP_MULTIPLY; // Keypad *
    case Keycode::KC_KPOCTAL:
        return SDLK_KP_OCTAL; // Keypad Octal
    case Keycode::KC_KPPERCENT:
        return SDLK_KP_PERCENT; // Keypad %
    case Keycode::KC_KPPERIOD:
        return SDLK_KP_PERIOD; // Keypad .
    case Keycode::KC_KPPLUS:
        return SDLK_KP_PLUS; // Keypad +
    case Keycode::KC_KPPLUSMINUS:
        return SDLK_KP_PLUSMINUS; // Keypad +/-
    case Keycode::KC_KPPOWER:
        return SDLK_KP_POWER; // Keypad ^
    case Keycode::KC_KPRIGHTBRACE:
        return SDLK_KP_RIGHTBRACE; // Keypad }
    case Keycode::KC_KPRIGHTPAREN:
        return SDLK_KP_RIGHTPAREN; // Keypad )
    case Keycode::KC_KPSPACE:
        return SDLK_KP_SPACE; // Keypad Space
    case Keycode::KC_KPTAB:
        return SDLK_KP_TAB; // Keypad Tab
    case Keycode::KC_KPVERTICALBAR:
        return SDLK_KP_VERTICALBAR; // Keypad |
    case Keycode::KC_KPXOR:
        return SDLK_KP_XOR; // Keypad XOR
    case Keycode::KC_l:
        return SDLK_l; // L
    case Keycode::KC_LALT:
        return SDLK_LALT; // Left Alt
    case Keycode::KC_LCTRL:
        return SDLK_LCTRL; // Left Ctrl
    case Keycode::KC_LEFT:
        return SDLK_LEFT; // Left
    case Keycode::KC_LEFTBRACKET:
        return SDLK_LEFTBRACKET; // [
    case Keycode::KC_LGUI:
        return SDLK_LGUI; // Left GUI
    case Keycode::KC_LSHIFT:
        return SDLK_LSHIFT; // Left Shift
    case Keycode::KC_m:
        return SDLK_m; // M
    case Keycode::KC_MAIL:
        return SDLK_MAIL; // Mail
    case Keycode::KC_MEDIASELECT:
        return SDLK_MEDIASELECT; // MediaSelect
    case Keycode::KC_MENU:
        return SDLK_MENU; // Menu
    case Keycode::KC_MINUS:
        return SDLK_MINUS; // -
    case Keycode::KC_MODE:
        return SDLK_MODE; // ModeSwitch
    case Keycode::KC_MUTE:
        return SDLK_MUTE; // Mute
    case Keycode::KC_n:
        return SDLK_n; // N
    case Keycode::KC_NUMLOCKCLEAR:
        return SDLK_NUMLOCKCLEAR; // Numlock
    case Keycode::KC_o:
        return SDLK_o; // O
    case Keycode::KC_OPER:
        return SDLK_OPER; // Oper
    case Keycode::KC_OUT:
        return SDLK_OUT; // Out
    case Keycode::KC_p:
        return SDLK_p; // P
    case Keycode::KC_PAGEDOWN:
        return SDLK_PAGEDOWN; // PageDown
    case Keycode::KC_PAGEUP:
        return SDLK_PAGEUP; // PageUp
    case Keycode::KC_PASTE:
        return SDLK_PASTE; // Paste
    case Keycode::KC_PAUSE:
        return SDLK_PAUSE; // Pause
    case Keycode::KC_PERIOD:
        return SDLK_PERIOD; // .
    case Keycode::KC_POWER:
        return SDLK_POWER; // Power
    case Keycode::KC_PRINTSCREEN:
        return SDLK_PRINTSCREEN; // PrintScreen
    case Keycode::KC_PRIOR:
        return SDLK_PRIOR; // Prior
    case Keycode::KC_q:
        return SDLK_q; // Q
    case Keycode::KC_r:
        return SDLK_r; // R
    case Keycode::KC_RALT:
        return SDLK_RALT; // Right Alt
    case Keycode::KC_RCTRL:
        return SDLK_RCTRL; // Right Ctrl
    case Keycode::KC_RETURN:
        return SDLK_RETURN; // Return
    case Keycode::KC_RETURN2:
        return SDLK_RETURN2; // Return
    case Keycode::KC_RGUI:
        return SDLK_RGUI; // Right GUI
    case Keycode::KC_RIGHT:
        return SDLK_RIGHT; // Right
    case Keycode::KC_RIGHTBRACKET:
        return SDLK_RIGHTBRACKET; // ]
    case Keycode::KC_RSHIFT:
        return SDLK_RSHIFT; // Right Shift
    case Keycode::KC_s:
        return SDLK_s; // S
    case Keycode::KC_SCROLLLOCK:
        return SDLK_SCROLLLOCK; // ScrollLock
    case Keycode::KC_SELECT:
        return SDLK_SELECT; // Select
    case Keycode::KC_SEMICOLON:
        return SDLK_SEMICOLON; // ;
    case Keycode::KC_SEPARATOR:
        return SDLK_SEPARATOR; // Separator
    case Keycode::KC_SLASH:
        return SDLK_SLASH; // /
    case Keycode::KC_SLEEP:
        return SDLK_SLEEP; // Sleep
    case Keycode::KC_SPACE:
        return SDLK_SPACE; // Space
    case Keycode::KC_STOP:
        return SDLK_STOP; // Stop
    case Keycode::KC_SYSREQ:
        return SDLK_SYSREQ; // SysReq
    case Keycode::KC_t:
        return SDLK_t; // T
    case Keycode::KC_TAB:
        return SDLK_TAB; // Tab
    case Keycode::KC_THOUSANDSSEPARATOR:
        return SDLK_THOUSANDSSEPARATOR; // ThousandsSeparator
    case Keycode::KC_u:
        return SDLK_u; // U
    case Keycode::KC_UNDO:
        return SDLK_UNDO; // Undo
    case Keycode::KC_UP:
        return SDLK_UP; // Up
    case Keycode::KC_v:
        return SDLK_v; // V
    case Keycode::KC_VOLUMEDOWN:
        return SDLK_VOLUMEDOWN; // VolumeDown
    case Keycode::KC_VOLUMEUP:
        return SDLK_VOLUMEUP; // VolumeUp
    case Keycode::KC_w:
        return SDLK_w; // W
    case Keycode::KC_WWW:
        return SDLK_WWW; // WWW
    case Keycode::KC_x:
        return SDLK_x; // X
    case Keycode::KC_y:
        return SDLK_y; // Y
    case Keycode::KC_z:
        return SDLK_z; // Z

    case Keycode::KC_MOUSE_LEFT:
        return MOUSE_TO_KEYCODE(SDL_BUTTON_LEFT);
    case Keycode::KC_MOUSE_MIDDLE:
        return MOUSE_TO_KEYCODE(SDL_BUTTON_MIDDLE);
    case Keycode::KC_MOUSE_RIGHT:
        return MOUSE_TO_KEYCODE(SDL_BUTTON_RIGHT);
    // For mice with more buttons than 3. Kinda dislike the name.
    case Keycode::KC_MOUSE_X1:
        return MOUSE_TO_KEYCODE(SDL_BUTTON_X1);
    case Keycode::KC_MOUSE_X2:
        return MOUSE_TO_KEYCODE(SDL_BUTTON_X2);
    default:
        assert(false);
        return SDLK_RETURN;
    }
}

Keycode SdlKeycodeToKeycode(SDL_Keycode kc)
{
    switch (kc) {
    case SDLK_0:
        return Keycode::KC_0; // 0
    case SDLK_1:
        return Keycode::KC_1; // 1
    case SDLK_2:
        return Keycode::KC_2; // 2
    case SDLK_3:
        return Keycode::KC_3; // 3
    case SDLK_4:
        return Keycode::KC_4; // 4
    case SDLK_5:
        return Keycode::KC_5; // 5
    case SDLK_6:
        return Keycode::KC_6; // 6
    case SDLK_7:
        return Keycode::KC_7; // 7
    case SDLK_8:
        return Keycode::KC_8; // 8
    case SDLK_9:
        return Keycode::KC_9; // 9
    case SDLK_a:
        return Keycode::KC_a; // A
    case SDLK_AC_BACK:
        return Keycode::KC_ACBACK; // AC Back
    case SDLK_AC_BOOKMARKS:
        return Keycode::KC_ACBOOKMARKS; // AC Bookmarks
    case SDLK_AC_FORWARD:
        return Keycode::KC_ACFORWARD; // AC Forward
    case SDLK_AC_HOME:
        return Keycode::KC_ACHOME; // AC Home
    case SDLK_AC_REFRESH:
        return Keycode::KC_ACREFRESH; // AC Refresh
    case SDLK_AC_SEARCH:
        return Keycode::KC_ACSEARCH; // AC Search
    case SDLK_AC_STOP:
        return Keycode::KC_ACSTOP; // AC Stop
    case SDLK_AGAIN:
        return Keycode::KC_AGAIN; // Again
    case SDLK_ALTERASE:
        return Keycode::KC_ALTERASE; // AltErase
    case SDLK_QUOTE:
        return Keycode::KC_QUOTE; // '
    case SDLK_APPLICATION:
        return Keycode::KC_APPLICATION; // Application
    case SDLK_AUDIOMUTE:
        return Keycode::KC_AUDIOMUTE; // AudioMute
    case SDLK_AUDIONEXT:
        return Keycode::KC_AUDIONEXT; // AudioNext
    case SDLK_AUDIOPLAY:
        return Keycode::KC_AUDIOPLAY; // AudioPlay
    case SDLK_AUDIOPREV:
        return Keycode::KC_AUDIOPREV; // AudioPrev
    case SDLK_AUDIOSTOP:
        return Keycode::KC_AUDIOSTOP; // AudioStop
    case SDLK_b:
        return Keycode::KC_b; // B
    case SDLK_BACKSLASH:
        return Keycode::KC_BACKSLASH; // Backslash
    case SDLK_BACKSPACE:
        return Keycode::KC_BACKSPACE; // Backspace
    case SDLK_BRIGHTNESSDOWN:
        return Keycode::KC_BRIGHTNESSDOWN; // BrightnessDown
    case SDLK_BRIGHTNESSUP:
        return Keycode::KC_BRIGHTNESSUP; // BrightnessUp
    case SDLK_c:
        return Keycode::KC_c; // C
    case SDLK_CALCULATOR:
        return Keycode::KC_CALCULATOR; // Calculator
    case SDLK_CANCEL:
        return Keycode::KC_CANCEL; // Cancel
    case SDLK_CAPSLOCK:
        return Keycode::KC_CAPSLOCK; // CapsLock
    case SDLK_CLEAR:
        return Keycode::KC_CLEAR; // Clear
    case SDLK_CLEARAGAIN:
        return Keycode::KC_CLEARAGAIN; // Clear / Again
    case SDLK_COMMA:
        return Keycode::KC_COMMA; // ,
    case SDLK_COMPUTER:
        return Keycode::KC_COMPUTER; // Computer
    case SDLK_COPY:
        return Keycode::KC_COPY; // Copy
    case SDLK_CRSEL:
        return Keycode::KC_CRSEL; // CrSel
    case SDLK_CURRENCYSUBUNIT:
        return Keycode::KC_CURRENCYSUBUNIT; // CurrencySubUnit
    case SDLK_CURRENCYUNIT:
        return Keycode::KC_CURRENCYUNIT; // CurrencyUnit
    case SDLK_CUT:
        return Keycode::KC_CUT; // Cut
    case SDLK_d:
        return Keycode::KC_d; // D
    case SDLK_DECIMALSEPARATOR:
        return Keycode::KC_DECIMALSEPARATOR; // DecimalSeparator
    case SDLK_DELETE:
        return Keycode::KC_DELETE; // Delete
    case SDLK_DISPLAYSWITCH:
        return Keycode::KC_DISPLAYSWITCH; // DisplaySwitch
    case SDLK_DOWN:
        return Keycode::KC_DOWN; // Down
    case SDLK_e:
        return Keycode::KC_e; // E
    case SDLK_EJECT:
        return Keycode::KC_EJECT; // Eject
    case SDLK_END:
        return Keycode::KC_END; // End
    case SDLK_EQUALS:
        return Keycode::KC_EQUALS; // =
    case SDLK_ESCAPE:
        return Keycode::KC_ESCAPE; // Escape
    case SDLK_EXECUTE:
        return Keycode::KC_EXECUTE; // Execute
    case SDLK_EXSEL:
        return Keycode::KC_EXSEL; // ExSel
    case SDLK_f:
        return Keycode::KC_f; // F
    case SDLK_F1:
        return Keycode::KC_F1; // F1
    case SDLK_F10:
        return Keycode::KC_F10; // F10
    case SDLK_F11:
        return Keycode::KC_F11; // F11
    case SDLK_F12:
        return Keycode::KC_F12; // F12
    case SDLK_F13:
        return Keycode::KC_F13; // F13
    case SDLK_F14:
        return Keycode::KC_F14; // F14
    case SDLK_F15:
        return Keycode::KC_F15; // F15
    case SDLK_F16:
        return Keycode::KC_F16; // F16
    case SDLK_F17:
        return Keycode::KC_F17; // F17
    case SDLK_F18:
        return Keycode::KC_F18; // F18
    case SDLK_F19:
        return Keycode::KC_F19; // F19
    case SDLK_F2:
        return Keycode::KC_F2; // F2
    case SDLK_F20:
        return Keycode::KC_F20; // F20
    case SDLK_F21:
        return Keycode::KC_F21; // F21
    case SDLK_F22:
        return Keycode::KC_F22; // F22
    case SDLK_F23:
        return Keycode::KC_F23; // F23
    case SDLK_F24:
        return Keycode::KC_F24; // F24
    case SDLK_F3:
        return Keycode::KC_F3; // F3
    case SDLK_F4:
        return Keycode::KC_F4; // F4
    case SDLK_F5:
        return Keycode::KC_F5; // F5
    case SDLK_F6:
        return Keycode::KC_F6; // F6
    case SDLK_F7:
        return Keycode::KC_F7; // F7
    case SDLK_F8:
        return Keycode::KC_F8; // F8
    case SDLK_F9:
        return Keycode::KC_F9; // F9
    case SDLK_FIND:
        return Keycode::KC_FIND; // Find
    case SDLK_g:
        return Keycode::KC_g; // G
    case SDLK_BACKQUOTE:
        return Keycode::KC_BACKQUOTE; // `
    case SDLK_h:
        return Keycode::KC_h; // H
    case SDLK_HELP:
        return Keycode::KC_HELP; // Help
    case SDLK_HOME:
        return Keycode::KC_HOME; // Home
    case SDLK_i:
        return Keycode::KC_i; // I
    case SDLK_INSERT:
        return Keycode::KC_INSERT; // Insert
    case SDLK_j:
        return Keycode::KC_j; // J
    case SDLK_k:
        return Keycode::KC_k; // K
    case SDLK_KBDILLUMDOWN:
        return Keycode::KC_KBDILLUMDOWN; // KBDIllumDown
    case SDLK_KBDILLUMTOGGLE:
        return Keycode::KC_KBDILLUMTOGGLE; // KBDIllumToggle
    case SDLK_KBDILLUMUP:
        return Keycode::KC_KBDILLUMUP; // KBDIllumUp
    case SDLK_KP_0:
        return Keycode::KC_KP0; // Keypad 0
    case SDLK_KP_00:
        return Keycode::KC_KP00; // Keypad 00
    case SDLK_KP_000:
        return Keycode::KC_KP000; // Keypad 000
    case SDLK_KP_1:
        return Keycode::KC_KP1; // Keypad 1
    case SDLK_KP_2:
        return Keycode::KC_KP2; // Keypad 2
    case SDLK_KP_3:
        return Keycode::KC_KP3; // Keypad 3
    case SDLK_KP_4:
        return Keycode::KC_KP4; // Keypad 4
    case SDLK_KP_5:
        return Keycode::KC_KP5; // Keypad 5
    case SDLK_KP_6:
        return Keycode::KC_KP6; // Keypad 6
    case SDLK_KP_7:
        return Keycode::KC_KP7; // Keypad 7
    case SDLK_KP_8:
        return Keycode::KC_KP8; // Keypad 8
    case SDLK_KP_9:
        return Keycode::KC_KP9; // Keypad 9
    case SDLK_KP_A:
        return Keycode::KC_KPA; // Keypad A
    case SDLK_KP_AMPERSAND:
        return Keycode::KC_KPAMPERSAND; // Keypad &
    case SDLK_KP_AT:
        return Keycode::KC_KPAT; // Keypad @
    case SDLK_KP_B:
        return Keycode::KC_KPB; // Keypad B
    case SDLK_KP_BACKSPACE:
        return Keycode::KC_KPBACKSPACE; // Keypad Backspace
    case SDLK_KP_BINARY:
        return Keycode::KC_KPBINARY; // Keypad Binary
    case SDLK_KP_C:
        return Keycode::KC_KPC; // Keypad C
    case SDLK_KP_CLEAR:
        return Keycode::KC_KPCLEAR; // Keypad Clear
    case SDLK_KP_CLEARENTRY:
        return Keycode::KC_KPCLEARENTRY; // Keypad ClearEntry
    case SDLK_KP_COLON:
        return Keycode::KC_KPCOLON; // Keypad :
    case SDLK_KP_COMMA:
        return Keycode::KC_KPCOMMA; // Keypad ,
    case SDLK_KP_D:
        return Keycode::KC_KPD; // Keypad D
    case SDLK_KP_DBLAMPERSAND:
        return Keycode::KC_KPDBLAMPERSAND; // Keypad &&
    case SDLK_KP_DBLVERTICALBAR:
        return Keycode::KC_KPDBLVERTICALBAR; // Keypad ||
    case SDLK_KP_DECIMAL:
        return Keycode::KC_KPDECIMAL; // Keypad Decimal
    case SDLK_KP_DIVIDE:
        return Keycode::KC_KPDIVIDE; // Keypad /
    case SDLK_KP_E:
        return Keycode::KC_KPE; // Keypad E
    case SDLK_KP_ENTER:
        return Keycode::KC_KPENTER; // Keypad Enter
    case SDLK_KP_EQUALS:
        return Keycode::KC_KPEQUALS; // Keypad =
    case SDLK_KP_EQUALSAS400:
        return Keycode::KC_KPEQUALSAS400; // Keypad = (AS400)
    case SDLK_KP_EXCLAM:
        return Keycode::KC_KPEXCLAM; // Keypad !
    case SDLK_KP_F:
        return Keycode::KC_KPF; // Keypad F
    case SDLK_KP_GREATER:
        return Keycode::KC_KPGREATER; // Keypad >
    case SDLK_KP_HASH:
        return Keycode::KC_KPHASH; // Keypad #
    case SDLK_KP_HEXADECIMAL:
        return Keycode::KC_KPHEXADECIMAL; // Keypad Hexadecimal
    case SDLK_KP_LEFTBRACE:
        return Keycode::KC_KPLEFTBRACE; // Keypad {
    case SDLK_KP_LEFTPAREN:
        return Keycode::KC_KPLEFTPAREN; // Keypad (
    case SDLK_KP_LESS:
        return Keycode::KC_KPLESS; // Keypad <
    case SDLK_KP_MEMADD:
        return Keycode::KC_KPMEMADD; // Keypad MemAdd
    case SDLK_KP_MEMCLEAR:
        return Keycode::KC_KPMEMCLEAR; // Keypad MemClear
    case SDLK_KP_MEMDIVIDE:
        return Keycode::KC_KPMEMDIVIDE; // Keypad MemDivide
    case SDLK_KP_MEMMULTIPLY:
        return Keycode::KC_KPMEMMULTIPLY; // Keypad MemMultiply
    case SDLK_KP_MEMRECALL:
        return Keycode::KC_KPMEMRECALL; // Keypad MemRecall
    case SDLK_KP_MEMSTORE:
        return Keycode::KC_KPMEMSTORE; // Keypad MemStore
    case SDLK_KP_MEMSUBTRACT:
        return Keycode::KC_KPMEMSUBTRACT; // Keypad MemSubtract
    case SDLK_KP_MINUS:
        return Keycode::KC_KPMINUS; // Keypad -
    case SDLK_KP_MULTIPLY:
        return Keycode::KC_KPMULTIPLY; // Keypad *
    case SDLK_KP_OCTAL:
        return Keycode::KC_KPOCTAL; // Keypad Octal
    case SDLK_KP_PERCENT:
        return Keycode::KC_KPPERCENT; // Keypad %
    case SDLK_KP_PERIOD:
        return Keycode::KC_KPPERIOD; // Keypad .
    case SDLK_KP_PLUS:
        return Keycode::KC_KPPLUS; // Keypad +
    case SDLK_KP_PLUSMINUS:
        return Keycode::KC_KPPLUSMINUS; // Keypad +/-
    case SDLK_KP_POWER:
        return Keycode::KC_KPPOWER; // Keypad ^
    case SDLK_KP_RIGHTBRACE:
        return Keycode::KC_KPRIGHTBRACE; // Keypad }
    case SDLK_KP_RIGHTPAREN:
        return Keycode::KC_KPRIGHTPAREN; // Keypad )
    case SDLK_KP_SPACE:
        return Keycode::KC_KPSPACE; // Keypad Space
    case SDLK_KP_TAB:
        return Keycode::KC_KPTAB; // Keypad Tab
    case SDLK_KP_VERTICALBAR:
        return Keycode::KC_KPVERTICALBAR; // Keypad |
    case SDLK_KP_XOR:
        return Keycode::KC_KPXOR; // Keypad XOR
    case SDLK_l:
        return Keycode::KC_l; // L
    case SDLK_LALT:
        return Keycode::KC_LALT; // Left Alt
    case SDLK_LCTRL:
        return Keycode::KC_LCTRL; // Left Ctrl
    case SDLK_LEFT:
        return Keycode::KC_LEFT; // Left
    case SDLK_LEFTBRACKET:
        return Keycode::KC_LEFTBRACKET; // [
    case SDLK_LGUI:
        return Keycode::KC_LGUI; // Left GUI
    case SDLK_LSHIFT:
        return Keycode::KC_LSHIFT; // Left Shift
    case SDLK_m:
        return Keycode::KC_m; // M
    case SDLK_MAIL:
        return Keycode::KC_MAIL; // Mail
    case SDLK_MEDIASELECT:
        return Keycode::KC_MEDIASELECT; // MediaSelect
    case SDLK_MENU:
        return Keycode::KC_MENU; // Menu
    case SDLK_MINUS:
        return Keycode::KC_MINUS; // -
    case SDLK_MODE:
        return Keycode::KC_MODE; // ModeSwitch
    case SDLK_MUTE:
        return Keycode::KC_MUTE; // Mute
    case SDLK_n:
        return Keycode::KC_n; // N
    case SDLK_NUMLOCKCLEAR:
        return Keycode::KC_NUMLOCKCLEAR; // Numlock
    case SDLK_o:
        return Keycode::KC_o; // O
    case SDLK_OPER:
        return Keycode::KC_OPER; // Oper
    case SDLK_OUT:
        return Keycode::KC_OUT; // Out
    case SDLK_p:
        return Keycode::KC_p; // P
    case SDLK_PAGEDOWN:
        return Keycode::KC_PAGEDOWN; // PageDown
    case SDLK_PAGEUP:
        return Keycode::KC_PAGEUP; // PageUp
    case SDLK_PASTE:
        return Keycode::KC_PASTE; // Paste
    case SDLK_PAUSE:
        return Keycode::KC_PAUSE; // Pause
    case SDLK_PERIOD:
        return Keycode::KC_PERIOD; // .
    case SDLK_POWER:
        return Keycode::KC_POWER; // Power
    case SDLK_PRINTSCREEN:
        return Keycode::KC_PRINTSCREEN; // PrintScreen
    case SDLK_PRIOR:
        return Keycode::KC_PRIOR; // Prior
    case SDLK_q:
        return Keycode::KC_q; // Q
    case SDLK_r:
        return Keycode::KC_r; // R
    case SDLK_RALT:
        return Keycode::KC_RALT; // Right Alt
    case SDLK_RCTRL:
        return Keycode::KC_RCTRL; // Right Ctrl
    case SDLK_RETURN:
        return Keycode::KC_RETURN; // Return
    case SDLK_RETURN2:
        return Keycode::KC_RETURN2; // Return
    case SDLK_RGUI:
        return Keycode::KC_RGUI; // Right GUI
    case SDLK_RIGHT:
        return Keycode::KC_RIGHT; // Right
    case SDLK_RIGHTBRACKET:
        return Keycode::KC_RIGHTBRACKET; // ]
    case SDLK_RSHIFT:
        return Keycode::KC_RSHIFT; // Right Shift
    case SDLK_s:
        return Keycode::KC_s; // S
    case SDLK_SCROLLLOCK:
        return Keycode::KC_SCROLLLOCK; // ScrollLock
    case SDLK_SELECT:
        return Keycode::KC_SELECT; // Select
    case SDLK_SEMICOLON:
        return Keycode::KC_SEMICOLON; // ;
    case SDLK_SEPARATOR:
        return Keycode::KC_SEPARATOR; // Separator
    case SDLK_SLASH:
        return Keycode::KC_SLASH; // /
    case SDLK_SLEEP:
        return Keycode::KC_SLEEP; // Sleep
    case SDLK_SPACE:
        return Keycode::KC_SPACE; // Space
    case SDLK_STOP:
        return Keycode::KC_STOP; // Stop
    case SDLK_SYSREQ:
        return Keycode::KC_SYSREQ; // SysReq
    case SDLK_t:
        return Keycode::KC_t; // T
    case SDLK_TAB:
        return Keycode::KC_TAB; // Tab
    case SDLK_THOUSANDSSEPARATOR:
        return Keycode::KC_THOUSANDSSEPARATOR; // ThousandsSeparator
    case SDLK_u:
        return Keycode::KC_u; // U
    case SDLK_UNDO:
        return Keycode::KC_UNDO; // Undo
    case SDLK_UP:
        return Keycode::KC_UP; // Up
    case SDLK_v:
        return Keycode::KC_v; // V
    case SDLK_VOLUMEDOWN:
        return Keycode::KC_VOLUMEDOWN; // VolumeDown
    case SDLK_VOLUMEUP:
        return Keycode::KC_VOLUMEUP; // VolumeUp
    case SDLK_w:
        return Keycode::KC_w; // W
    case SDLK_WWW:
        return Keycode::KC_WWW; // WWW
    case SDLK_x:
        return Keycode::KC_x; // X
    case SDLK_y:
        return Keycode::KC_y; // Y
    case SDLK_z:
        return Keycode::KC_z; // Z

    case MOUSE_TO_KEYCODE(SDL_BUTTON_LEFT):
        return Keycode::KC_MOUSE_LEFT;
    case MOUSE_TO_KEYCODE(SDL_BUTTON_MIDDLE):
        return Keycode::KC_MOUSE_MIDDLE;
    case MOUSE_TO_KEYCODE(SDL_BUTTON_RIGHT):
        return Keycode::KC_MOUSE_RIGHT;
    // For mice with more buttons than 3. Kinda dislike the name.
    case MOUSE_TO_KEYCODE(SDL_BUTTON_X1):
        return Keycode::KC_MOUSE_X1;
    case MOUSE_TO_KEYCODE(SDL_BUTTON_X2):
        return Keycode::KC_MOUSE_X2;
    default:
        return Keycode::KC_UNKNOWN;
    }
}