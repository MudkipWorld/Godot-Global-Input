#pragma once
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/core/class_db.hpp"
#include <memory>
#include <unordered_map>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifdef __linux__
#include "unix_keys.h"
#endif

using namespace godot;

class KeyMaps{
    public:
    KeyMaps(){};
    virtual ~KeyMaps() {}
    virtual void init_key_map(std::unordered_map<int, int>& key_map){};
    void get_platform_key_mapping(std::unordered_map<int, int>& key_map);

};

class WindowsKeyMap : public KeyMaps {
public:
    WindowsKeyMap(){};

    void init_key_map(std::unordered_map<int, int>& key_map) override{
        #ifdef _WIN32
        key_map.clear();
        // Fn key_map
        key_map[VK_F1] = KEY_F1;
        key_map[VK_F2] = KEY_F2;
        key_map[VK_F3] = KEY_F3;
        key_map[VK_F4] = KEY_F4;
        key_map[VK_F5] = KEY_F5;
        key_map[VK_F6] = KEY_F6;
        key_map[VK_F7] = KEY_F7;
        key_map[VK_F8] = KEY_F8;
        key_map[VK_F9] = KEY_F9;
        key_map[VK_F10] = KEY_F10;
        key_map[VK_F11] = KEY_F11;
        key_map[VK_F12] = KEY_F12;
        key_map[VK_F13] = KEY_F13;
        key_map[VK_F14] = KEY_F14;
        key_map[VK_F15] = KEY_F15;
        key_map[VK_F16] = KEY_F16;
        key_map[VK_F17] = KEY_F17;
        key_map[VK_F18] = KEY_F18;
        key_map[VK_F19] = KEY_F19;
        key_map[VK_F20] = KEY_F20;
        key_map[VK_F21] = KEY_F21;
        key_map[VK_F22] = KEY_F22;
        key_map[VK_F23] = KEY_F23;
        key_map[VK_F24] = KEY_F24;

        key_map[VK_CONTROL] = KEY_CTRL;
        key_map[VK_SHIFT] = KEY_SHIFT;
        key_map[VK_MENU] = KEY_ALT;
        key_map[VK_TAB] = KEY_TAB;
        key_map[VK_SPACE] = KEY_SPACE;
        key_map[VK_BACK] = KEY_BACKSPACE;
        key_map[VK_INSERT] = KEY_INSERT;
        key_map[VK_DELETE] = KEY_DELETE;
        key_map[VK_HOME] = KEY_HOME;
        key_map[VK_END] = KEY_END;
        key_map[VK_PRIOR] = KEY_PAGEUP;
        key_map[VK_NEXT] = KEY_PAGEDOWN;
        key_map[VK_UP] = KEY_UP;
        key_map[VK_DOWN] = KEY_DOWN;
        key_map[VK_LEFT] = KEY_LEFT;
        key_map[VK_RIGHT] = KEY_RIGHT;
        key_map[VK_NUMPAD0] = KEY_KP_0;
        key_map[VK_NUMPAD1] = KEY_KP_1;
        key_map[VK_NUMPAD2] = KEY_KP_2;
        key_map[VK_NUMPAD3] = KEY_KP_3;
        key_map[VK_NUMPAD4] = KEY_KP_4;
        key_map[VK_NUMPAD5] = KEY_KP_5;
        key_map[VK_NUMPAD6] = KEY_KP_6;
        key_map[VK_NUMPAD7] = KEY_KP_7;
        key_map[VK_NUMPAD8] = KEY_KP_8;
        key_map[VK_NUMPAD9] = KEY_KP_9;
        key_map[VK_NUMLOCK] = KEY_NUMLOCK;
        key_map[VK_ADD] = KEY_KP_ADD;
        key_map[VK_SUBTRACT] = KEY_KP_SUBTRACT;
        key_map[VK_MULTIPLY] = KEY_KP_MULTIPLY;
        key_map[VK_DIVIDE] = KEY_KP_DIVIDE;
        key_map[VK_DECIMAL] = KEY_KP_PERIOD;
        for (int i = KEY_A; i <= KEY_Z; i++) key_map[i] = i;

        for (int i = KEY_0; i <= KEY_9; i++) key_map[i] = i;
        key_map[VK_OEM_1] = KEY_SEMICOLON;
        key_map[VK_OEM_2] = KEY_SLASH;
        key_map[VK_OEM_3] = KEY_ASCIITILDE;
        key_map[VK_OEM_4] = KEY_BRACKETLEFT;
        key_map[VK_OEM_5] = KEY_BACKSLASH;
        key_map[VK_OEM_6] = KEY_BRACKETRIGHT;
        key_map[VK_OEM_7] = KEY_QUOTEDBL;
        key_map[VK_OEM_PLUS] = KEY_PLUS;
        key_map[VK_OEM_COMMA] = KEY_COMMA;
        key_map[VK_OEM_MINUS] = KEY_MINUS;
        key_map[VK_OEM_PERIOD] = KEY_PERIOD;
        key_map[VK_LBUTTON] = MOUSE_BUTTON_LEFT;
        key_map[VK_RBUTTON] = MOUSE_BUTTON_RIGHT;
        key_map[VK_MBUTTON] = MOUSE_BUTTON_MIDDLE;
        key_map[VK_XBUTTON1] = MOUSE_BUTTON_XBUTTON1;
        key_map[VK_XBUTTON2] = MOUSE_BUTTON_XBUTTON2;
        #endif
    }
    
};

class LinuxKeyMap : public KeyMaps {
public:
    LinuxKeyMap(){};

    void init_key_map(std::unordered_map<int, int>& key_map) override{
        #ifdef __linux__
        key_map.clear();

        key_map[PH_KEY_F1] = KEY_F1;
        key_map[PH_KEY_F2] = KEY_F2;
        key_map[PH_KEY_F3] = KEY_F3;
        key_map[PH_KEY_F4] = KEY_F4;
        key_map[PH_KEY_F5] = KEY_F5;
        key_map[PH_KEY_F6] = KEY_F6;
        key_map[PH_KEY_F7] = KEY_F7;
        key_map[PH_KEY_F8] = KEY_F8;
        key_map[PH_KEY_F9] = KEY_F9;
        key_map[PH_KEY_F10] = KEY_F10;
        key_map[PH_KEY_F11] = KEY_F11;
        key_map[PH_KEY_F12] = KEY_F12;
        key_map[PH_KEY_F13] = KEY_F13;
        key_map[PH_KEY_F14] = KEY_F14;
        key_map[PH_KEY_F15] = KEY_F15;
        key_map[PH_KEY_F16] = KEY_F16;
        key_map[PH_KEY_F17] = KEY_F17;
        key_map[PH_KEY_F18] = KEY_F18;
        key_map[PH_KEY_F19] = KEY_F19;
        key_map[PH_KEY_F20] = KEY_F20;
        key_map[PH_KEY_F21] = KEY_F21;
        key_map[PH_KEY_F22] = KEY_F22;
        key_map[PH_KEY_F23] = KEY_F23;
        key_map[PH_KEY_F24] = KEY_F24;

        // Control keys
        key_map[PH_KEY_LEFTCTRL] = KEY_CTRL;
        key_map[PH_KEY_RIGHTCTRL] = KEY_CTRL;
        key_map[PH_KEY_LEFTSHIFT] = KEY_SHIFT;
        key_map[PH_KEY_RIGHTSHIFT] = KEY_SHIFT;
        key_map[PH_KEY_LEFTALT] = KEY_ALT;
        key_map[PH_KEY_RIGHTALT] = KEY_ALT;
        key_map[PH_KEY_TAB] = KEY_TAB;
        key_map[PH_KEY_SPACE] = KEY_SPACE;
        key_map[PH_KEY_BACKSPACE] = KEY_BACKSPACE;
        key_map[PH_KEY_INSERT] = KEY_INSERT;
        key_map[PH_KEY_DELETE] = KEY_DELETE;
        key_map[PH_KEY_HOME] = KEY_HOME;
        key_map[PH_KEY_END] = KEY_END;
        key_map[PH_KEY_PAGEUP] = KEY_PAGEUP;
        key_map[PH_KEY_PAGEDOWN] = KEY_PAGEDOWN;

        // Arrow keys
        key_map[PH_KEY_UP] = KEY_UP;
        key_map[PH_KEY_DOWN] = KEY_DOWN;
        key_map[PH_KEY_LEFT] = KEY_LEFT;
        key_map[PH_KEY_RIGHT] = KEY_RIGHT;

        // Numpad keys
        key_map[PH_KEY_KP0] = KEY_KP_0;
        key_map[PH_KEY_KP1] = KEY_KP_1;
        key_map[PH_KEY_KP2] = KEY_KP_2;
        key_map[PH_KEY_KP3] = KEY_KP_3;
        key_map[PH_KEY_KP4] = KEY_KP_4;
        key_map[PH_KEY_KP5] = KEY_KP_5;
        key_map[PH_KEY_KP6] = KEY_KP_6;
        key_map[PH_KEY_KP7] = KEY_KP_7;
        key_map[PH_KEY_KP8] = KEY_KP_8;
        key_map[PH_KEY_KP9] = KEY_KP_9;
        key_map[PH_KEY_NUMLOCK] = KEY_NUMLOCK;
        key_map[PH_KEY_KPPLUS] = KEY_KP_ADD;
        key_map[PH_KEY_KPMINUS] = KEY_KP_SUBTRACT;
        key_map[PH_KEY_KPASTERISK] = KEY_KP_MULTIPLY;
        key_map[PH_KEY_KPSLASH] = KEY_KP_DIVIDE;
        key_map[PH_KEY_KPDOT] = KEY_KP_PERIOD;
        key_map[PH_KEY_KPENTER] = KEY_KP_ENTER;

        // Letters
        key_map[PH_KEY_Q] = KEY_Q;
        key_map[PH_KEY_W] = KEY_W;
        key_map[PH_KEY_E] = KEY_E;
        key_map[PH_KEY_R] = KEY_R;
        key_map[PH_KEY_T] = KEY_T;
        key_map[PH_KEY_Y] = KEY_Y;
        key_map[PH_KEY_U] = KEY_U;
        key_map[PH_KEY_I] = KEY_I;
        key_map[PH_KEY_O] = KEY_O;
        key_map[PH_KEY_P] = KEY_P;
        key_map[PH_KEY_A] = KEY_A;
        key_map[PH_KEY_S] = KEY_S;
        key_map[PH_KEY_D] = KEY_D;
        key_map[PH_KEY_F] = KEY_F;
        key_map[PH_KEY_G] = KEY_G;
        key_map[PH_KEY_H] = KEY_H;
        key_map[PH_KEY_J] = KEY_J;
        key_map[PH_KEY_K] = KEY_K;
        key_map[PH_KEY_L] = KEY_L;
        key_map[PH_KEY_Z] = KEY_Z;
        key_map[PH_KEY_X] = KEY_X;
        key_map[PH_KEY_C] = KEY_C;
        key_map[PH_KEY_V] = KEY_V;
        key_map[PH_KEY_B] = KEY_B;
        key_map[PH_KEY_N] = KEY_N;
        key_map[PH_KEY_M] = KEY_M;

        // Numbers
        key_map[PH_KEY_0] = KEY_0;
        for (int i = 0; i <= PH_KEY_9 - PH_KEY_1; i++) key_map[PH_KEY_1 + i] = KEY_1 + i;

        // Regional keys
        key_map[PH_KEY_SEMICOLON] = KEY_SEMICOLON;
        key_map[PH_KEY_SLASH] = KEY_SLASH;
        key_map[PH_KEY_GRAVE] = KEY_ASCIITILDE;
        key_map[PH_KEY_LEFTBRACE] = KEY_BRACKETLEFT;
        key_map[PH_KEY_BACKSLASH] = KEY_BACKSLASH;
        key_map[PH_KEY_RIGHTBRACE] = KEY_BRACKETRIGHT;
        key_map[PH_KEY_APOSTROPHE] = KEY_QUOTEDBL;
        key_map[PH_KEY_EQUAL] = KEY_PLUS;
        key_map[PH_KEY_COMMA] = KEY_COMMA;
        key_map[PH_KEY_MINUS] = KEY_MINUS;
        key_map[PH_KEY_DOT] = KEY_PERIOD;


        

        #endif
    }
};

class MacOSKeyMap : public KeyMaps {
public:
    MacOSKeyMap(){};
    void init_key_map(std::unordered_map<int, int>& key_map) override{
        #ifdef __APPLE__
        key_map.clear();

        key_map[0] = KEY_A;
        key_map[1] = KEY_S;
        key_map[2] = KEY_D;
        key_map[3] = KEY_F;
        key_map[4] = KEY_H;
        key_map[5] = KEY_G;

        key_map[6] = KEY_Z;
        key_map[7] = KEY_X;
        key_map[8] = KEY_C;
        key_map[9] = KEY_V;

        key_map[11] = KEY_B;
        key_map[12] = KEY_Q;
        key_map[13] = KEY_W;
        key_map[14] = KEY_E;
        key_map[15] = KEY_R;
        key_map[16] = KEY_Y;
        key_map[17] = KEY_T;

        key_map[18] = KEY_1;
        key_map[19] = KEY_2;
        key_map[20] = KEY_3;
        key_map[21] = KEY_4;
        key_map[22] = KEY_6;
        key_map[23] = KEY_5;
        key_map[24] = KEY_EQUAL;
        key_map[25] = KEY_9;
        key_map[26] = KEY_7;
        key_map[27] = KEY_MINUS;
        key_map[28] = KEY_8;
        key_map[29] = KEY_0;
        key_map[30] = KEY_BRACKETRIGHT;
        key_map[31] = KEY_O;
        key_map[32] = KEY_U;
        key_map[33] = KEY_BRACKETLEFT;
        key_map[34] = KEY_I;
        key_map[35] = KEY_P;

        key_map[36] = KEY_ENTER;
        key_map[37] = KEY_L;
        key_map[38] = KEY_J;
        key_map[39] = KEY_QUOTEDBL;
        key_map[40] = KEY_K;
        key_map[41] = KEY_SEMICOLON;
        key_map[42] = KEY_BACKSLASH;

        key_map[43] = KEY_COMMA;
        key_map[44] = KEY_SLASH;
        key_map[45] = KEY_N;
        key_map[46] = KEY_M;
        key_map[47] = KEY_PERIOD;

        key_map[49] = KEY_SPACE;

        key_map[50] = KEY_ASCIITILDE;
        key_map[51] = KEY_BACKSPACE;
        key_map[52] = KEY_KP_ENTER;
        key_map[53] = KEY_ESCAPE;

        key_map[55] = KEY_META;
        key_map[56] = KEY_SHIFT;
        key_map[57] = KEY_CAPSLOCK;
        key_map[58] = KEY_ALT;
        key_map[59] = KEY_CTRL;
        key_map[60] = KEY_SHIFT;
        key_map[61] = KEY_ALT;
        key_map[62] = KEY_CTRL;

        key_map[123] = KEY_LEFT;
        key_map[124] = KEY_RIGHT;
        key_map[125] = KEY_DOWN;
        key_map[126] = KEY_UP;
        #endif
    }


};

inline void KeyMaps::get_platform_key_mapping(std::unordered_map<int, int>& key_map) {
#if defined(_WIN32)
    static WindowsKeyMap win_mapping;
    win_mapping.init_key_map(key_map);
#elif defined(__linux__)
    static LinuxKeyMap linux_mapping;
    linux_mapping.init_key_map(key_map);
#elif defined(__APPLE__)
    static MacOSKeyMap macos_mapping;
    macos_mapping.init_key_map(key_map);
#endif
}
