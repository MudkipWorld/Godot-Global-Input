#include "GlobalInputsx11.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <chrono>
#include <thread>

#ifdef __linux__
#include <X11/Xlib.h>
#include <linux/input.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <X11/keysymdef.h>
#include <X11/XF86keysym.h> 
#endif

using namespace godot;

std::unordered_map<int, bool> GlobalInputX11::key_state;
std::unordered_map<int, uint64_t> GlobalInputX11::key_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputX11::key_just_released_frame;

std::unordered_map<int, bool> GlobalInputX11::mouse_state;
std::unordered_map<int, uint64_t> GlobalInputX11::mouse_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputX11::mouse_just_released_frame;

std::unordered_map<int, bool> GlobalInputX11::last_key_state;
std::unordered_map<int, bool> GlobalInputX11::last_mouse_state;

Vector2 GlobalInputX11::mouse_position;
int GlobalInputX11::wheel_delta = 0;

uint64_t GlobalInputX11::current_frame = 0;
std::atomic<bool> GlobalInputX11::running = false;
std::recursive_mutex GlobalInputX11::state_mutex;
std::thread GlobalInputX11::poll_thread;


void GlobalInputX11::start() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    if (running) return;
    running = true;

#ifdef __linux__
    display = XOpenDisplay(nullptr);
    if (!display) UtilityFunctions::push_error("GlobalInputX11: Failed to open X display.");
    root_window = DefaultRootWindow(display);
    init_key_map();

    keyboard_fd = -1;
    DIR* dir = opendir("/dev/input");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (strncmp(ent->d_name,"event",5)==0) {
                std::string path = "/dev/input/";
                path += ent->d_name;
                int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
                if (fd != -1) {
                    keyboard_fd = fd;
                    break;
                }
            }
        }
        closedir(dir);
    }
    mice_fd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
#endif

    poll_thread = std::thread(&GlobalInputX11::poll_input, this);
}

void GlobalInputX11::stop() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    running = false;
    if (poll_thread.joinable()) poll_thread.join();

#ifdef __linux__
    if (keyboard_fd != -1) close(keyboard_fd);
    if (mice_fd != -1) close(mice_fd);
    if (display) XCloseDisplay(display);
    keyboard_fd = mice_fd = -1;
    display = nullptr;
#endif
}

void GlobalInputX11::increment_frame() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

}

Vector2 GlobalInputX11::get_mouse_position() {
#ifdef __linux__
    if (!display) return mouse_position;
    ::Window root, child;
    int rx, ry, wx, wy;
    unsigned int mask;
    if (XQueryPointer(display, root_window, &root, &child, &rx, &ry, &wx, &wy, &mask)) {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        mouse_position.x = rx;
        mouse_position.y = ry;
    }
#endif
    return mouse_position;
}


bool GlobalInputX11::is_key_pressed(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_state.find(key);
    return it != key_state.end() && it->second;
}

bool GlobalInputX11::is_key_just_pressed(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_just_pressed_frame.find(key);
    return it != key_just_pressed_frame.end() &&
           (current_frame - it->second) <= JUST_BUFFER_FRAMES;
}

bool GlobalInputX11::is_key_just_released(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_just_released_frame.find(key);
    return it != key_just_released_frame.end() &&
           (current_frame - it->second) <= JUST_BUFFER_FRAMES;
}

bool GlobalInputX11::is_mouse_pressed(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_state.find(button);
    return it != mouse_state.end() && it->second;
}

bool GlobalInputX11::is_mouse_just_pressed(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_just_pressed_frame.find(button);
    return it != mouse_just_pressed_frame.end() &&
           (current_frame - it->second) <= JUST_BUFFER_FRAMES;
}

bool GlobalInputX11::is_mouse_just_released(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_just_released_frame.find(button);
    return it != mouse_just_released_frame.end() &&
           (current_frame - it->second) <= JUST_BUFFER_FRAMES;
}
bool GlobalInputX11::is_action_pressed(const String &action_name) {
    if (!InputMap::get_singleton()) return false;
    const Array events = InputMap::get_singleton()->action_get_events(action_name);
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i = 0; i < events.size(); ++i) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;

        if (ev->is_class("InputEventKey")) {
            InputEventKey *key_ev = Object::cast_to<InputEventKey>(ev.ptr());
            if (!key_ev) continue;

            int keycode = key_ev->get_keycode();
            if (!key_state[keycode]) continue;

            if (!modifiers_match(key_ev)) continue; 

            return true;
        }
        else if (ev->is_class("InputEventMouseButton")) {
            InputEventMouseButton *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr());
            if (!mouse_ev) continue;
            if (!modifiers_match(mouse_ev)) continue; 
            if (mouse_state[mouse_ev->get_button_index()]) return true;
        }
    }

    return false;
}


bool GlobalInputX11::is_action_just_pressed(const String &action_name) {
    if (!InputMap::get_singleton()) return false;
    const Array events = InputMap::get_singleton()->action_get_events(action_name);
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i = 0; i < events.size(); ++i) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;

        if (ev->is_class("InputEventKey")) {
            InputEventKey *key_ev = Object::cast_to<InputEventKey>(ev.ptr());
            if (!key_ev) continue;

            int keycode = key_ev->get_keycode();
            auto it = key_just_pressed_frame.find(keycode);
            if (it == key_just_pressed_frame.end() || (current_frame - it->second) > 1) continue;

            if (!modifiers_match(key_ev)) continue; 

            return true;
        }
        else if (ev->is_class("InputEventMouseButton")) {
            InputEventMouseButton *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr());
            if (!mouse_ev) continue;

            auto it = mouse_just_pressed_frame.find(mouse_ev->get_button_index());
            if (it != mouse_just_pressed_frame.end() && (current_frame - it->second) <= 1) {
                if (!modifiers_match(mouse_ev)) continue; 
                return true;
            }
        }
    }

    return false;
}


bool GlobalInputX11::is_action_just_released(const String &action_name) {
    if (!InputMap::get_singleton()) return false;
    const Array events = InputMap::get_singleton()->action_get_events(action_name);
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i = 0; i < events.size(); ++i) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;

        if (ev->is_class("InputEventKey")) {
            InputEventKey *key_ev = Object::cast_to<InputEventKey>(ev.ptr());
            if (!key_ev) continue;

            int keycode = key_ev->get_keycode();
            auto it = key_just_released_frame.find(keycode);
            if (it == key_just_released_frame.end() || (current_frame - it->second) > 1) continue;

            if (!modifiers_match(key_ev)) continue; 

            return true;
        }
        else if (ev->is_class("InputEventMouseButton")) {
            InputEventMouseButton *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr());
            if (!mouse_ev) continue;

            auto it = mouse_just_released_frame.find(mouse_ev->get_button_index());
            if (it != mouse_just_released_frame.end() && (current_frame - it->second) <= 1) {
                if (!modifiers_match(mouse_ev)) continue; 
                return true;
            }
        }
    }

    return false;
}



bool GlobalInputX11::is_shift_pressed()   { return is_key_pressed(KEY_SHIFT); }
bool GlobalInputX11::is_ctrl_pressed()    { return is_key_pressed(KEY_CTRL); }
bool GlobalInputX11::is_alt_pressed()     { return is_key_pressed(KEY_ALT); }
bool GlobalInputX11::is_meta_pressed()    { return is_key_pressed(KEY_META); }

bool GlobalInputX11::modifiers_match(InputEvent *ev) {
    bool ev_shift = false;
    bool ev_ctrl  = false;
    bool ev_alt   = false;
    bool ev_meta  = false;
    int  ev_keycode = 0;

    if (auto *key_ev = Object::cast_to<InputEventKey>(ev)) {
        ev_shift   = key_ev->is_shift_pressed();
        ev_ctrl    = key_ev->is_ctrl_pressed();
        ev_alt     = key_ev->is_alt_pressed();
        ev_meta    = key_ev->is_meta_pressed();
        ev_keycode = key_ev->get_keycode();
    } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev)) {
        ev_shift = mouse_ev->is_shift_pressed();
        ev_ctrl  = mouse_ev->is_ctrl_pressed();
        ev_alt   = mouse_ev->is_alt_pressed();
        ev_meta  = mouse_ev->is_meta_pressed();
    } else {
        return true;
    }

    bool shift_now = is_shift_pressed();
    bool ctrl_now  = is_ctrl_pressed();
    bool alt_now   = is_alt_pressed();
    bool meta_now  = is_meta_pressed();

    if (ev_keycode == KEY_SHIFT || ev_keycode == KEY_CTRL || ev_keycode == KEY_ALT || ev_keycode == KEY_META) {
        if (ev_keycode == KEY_SHIFT) ev_shift = shift_now;
        if (ev_keycode == KEY_CTRL)  ev_ctrl  = ctrl_now;
        if (ev_keycode == KEY_ALT)   ev_alt   = alt_now;
        if (ev_keycode == KEY_META)  ev_meta  = meta_now;
    }

    if (ev_shift != shift_now) return false;
    if (ev_ctrl  != ctrl_now)  return false;
    if (ev_alt   != alt_now)   return false;
    if (ev_meta  != meta_now)  return false;

    return true;
}

Dictionary GlobalInputX11::get_keys_pressed_detailed() {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    for (const auto &[key, down]: key_state)
        if (down) dict[key] = true;
    return dict;
}
Dictionary GlobalInputX11::get_keys_just_pressed_detailed() {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    for (const auto &[key, frame] : key_just_pressed_frame)
        if ((current_frame - frame) <= JUST_BUFFER_FRAMES)
            dict[key] = true;
    return dict;
}

Dictionary GlobalInputX11::get_keys_just_released_detailed() {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    for (const auto &[key, frame] : key_just_released_frame)
        if ((current_frame - frame) <= JUST_BUFFER_FRAMES)
            dict[key] = true;
    return dict;
}

void GlobalInputX11::poll_input() {
#ifdef __linux__
    if (!running) return;
    if (!display) return;

    // Select which events we want to listen to
    XSelectInput(display, root_window,
                 KeyPressMask | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask |
                 PointerMotionMask);

    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(state_mutex);
            if (!running) break;
        }

        // Only process events if there are any
        int max_events = 64; // cap per iteration to avoid CPU spike
        while (XPending(display) && max_events-- > 0) {
            XEvent event;
            XNextEvent(display, &event);

            std::lock_guard<std::recursive_mutex> lock(state_mutex);
            switch (event.type) {
                case KeyPress:
                case KeyRelease: {
                    XKeyEvent *key_ev = (XKeyEvent*)&event;
                    KeySym keysym = XkbKeycodeToKeysym(display, key_ev->keycode, 0, 0);
                    auto it = x11_to_godot.find(keysym);
                    if (it == x11_to_godot.end()) break;

                    int godot_key = it->second;
                    bool pressed_now = (event.type == KeyPress);
                    bool before = key_state[godot_key];

                    key_state[godot_key] = pressed_now;
                    if (pressed_now && !before) key_just_pressed_frame[godot_key] = current_frame;
                    if (!pressed_now && before) key_just_released_frame[godot_key] = current_frame;
                    break;
                }

                case ButtonPress:
                case ButtonRelease: {
                    XButtonEvent *btn_ev = (XButtonEvent*)&event;
                    bool pressed_now = (event.type == ButtonPress);
                    int button = 0;

                    if (btn_ev->button == Button1) button = MOUSE_BUTTON_LEFT;
                    else if (btn_ev->button == Button2) button = MOUSE_BUTTON_MIDDLE;
                    else if (btn_ev->button == Button3) button = MOUSE_BUTTON_RIGHT;
                    else break;

                    bool before = mouse_state[button];
                    mouse_state[button] = pressed_now;
                    if (pressed_now && !before) mouse_just_pressed_frame[button] = current_frame;
                    if (!pressed_now && before) mouse_just_released_frame[button] = current_frame;
                    break;
                }

                case MotionNotify: {
                    XMotionEvent *motion_ev = (XMotionEvent*)&event;
                    mouse_position.x = motion_ev->x_root;
                    mouse_position.y = motion_ev->y_root;
                    break;
                }

                default:
                    break;
            }
        }

        // Avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
#endif
}

#ifdef __linux__
#include <X11/keysymdef.h>

void GlobalInputX11::init_key_map() {
    x11_to_godot.clear();

    // Letters
    x11_to_godot[XK_a] = KEY_A;
    x11_to_godot[XK_b] = KEY_B;
    x11_to_godot[XK_c] = KEY_C;
    x11_to_godot[XK_d] = KEY_D;
    x11_to_godot[XK_e] = KEY_E;
    x11_to_godot[XK_f] = KEY_F;
    x11_to_godot[XK_g] = KEY_G;
    x11_to_godot[XK_h] = KEY_H;
    x11_to_godot[XK_i] = KEY_I;
    x11_to_godot[XK_j] = KEY_J;
    x11_to_godot[XK_k] = KEY_K;
    x11_to_godot[XK_l] = KEY_L;
    x11_to_godot[XK_m] = KEY_M;
    x11_to_godot[XK_n] = KEY_N;
    x11_to_godot[XK_o] = KEY_O;
    x11_to_godot[XK_p] = KEY_P;
    x11_to_godot[XK_q] = KEY_Q;
    x11_to_godot[XK_r] = KEY_R;
    x11_to_godot[XK_s] = KEY_S;
    x11_to_godot[XK_t] = KEY_T;
    x11_to_godot[XK_u] = KEY_U;
    x11_to_godot[XK_v] = KEY_V;
    x11_to_godot[XK_w] = KEY_W;
    x11_to_godot[XK_x] = KEY_X;
    x11_to_godot[XK_y] = KEY_Y;
    x11_to_godot[XK_z] = KEY_Z;

    // Number row
    x11_to_godot[XK_1] = KEY_1;
    x11_to_godot[XK_2] = KEY_2;
    x11_to_godot[XK_3] = KEY_3;
    x11_to_godot[XK_4] = KEY_4;
    x11_to_godot[XK_5] = KEY_5;
    x11_to_godot[XK_6] = KEY_6;
    x11_to_godot[XK_7] = KEY_7;
    x11_to_godot[XK_8] = KEY_8;
    x11_to_godot[XK_9] = KEY_9;
    x11_to_godot[XK_0] = KEY_0;

    // Function keys
    x11_to_godot[XK_F1]  = KEY_F1;
    x11_to_godot[XK_F2]  = KEY_F2;
    x11_to_godot[XK_F3]  = KEY_F3;
    x11_to_godot[XK_F4]  = KEY_F4;
    x11_to_godot[XK_F5]  = KEY_F5;
    x11_to_godot[XK_F6]  = KEY_F6;
    x11_to_godot[XK_F7]  = KEY_F7;
    x11_to_godot[XK_F8]  = KEY_F8;
    x11_to_godot[XK_F9]  = KEY_F9;
    x11_to_godot[XK_F10] = KEY_F10;
    x11_to_godot[XK_F11] = KEY_F11;
    x11_to_godot[XK_F12] = KEY_F12;
    x11_to_godot[XK_F13] = KEY_F13;
    x11_to_godot[XK_F14] = KEY_F14;
    x11_to_godot[XK_F15] = KEY_F15;
    x11_to_godot[XK_F16] = KEY_F16;
    x11_to_godot[XK_F17] = KEY_F17;
    x11_to_godot[XK_F18] = KEY_F18;
    x11_to_godot[XK_F19] = KEY_F19;
    x11_to_godot[XK_F20] = KEY_F20;
    x11_to_godot[XK_F21] = KEY_F21;
    x11_to_godot[XK_F22] = KEY_F22;
    x11_to_godot[XK_F23] = KEY_F23;
    x11_to_godot[XK_F24] = KEY_F24;

    // Modifiers
    x11_to_godot[XK_Control_L] = KEY_CTRL;
    x11_to_godot[XK_Control_R] = KEY_CTRL;
    x11_to_godot[XK_Shift_L]   = KEY_SHIFT;
    x11_to_godot[XK_Shift_R]   = KEY_SHIFT;
    x11_to_godot[XK_Alt_L]     = KEY_ALT;
    x11_to_godot[XK_Alt_R]     = KEY_ALT;
    x11_to_godot[XK_Super_L]   = KEY_META;
    x11_to_godot[XK_Super_R]   = KEY_META;

    // Lock keys
    x11_to_godot[XK_Caps_Lock]   = KEY_CAPSLOCK;
    x11_to_godot[XK_Num_Lock]    = KEY_NUMLOCK;
    x11_to_godot[XK_Scroll_Lock] = KEY_SCROLLLOCK;

    // Navigation keys
    x11_to_godot[XK_Tab]       = KEY_TAB;
    x11_to_godot[XK_space]     = KEY_SPACE;
    x11_to_godot[XK_BackSpace] = KEY_BACKSPACE;
    x11_to_godot[XK_Return]    = KEY_ENTER;
    x11_to_godot[XK_Escape]    = KEY_ESCAPE;
    x11_to_godot[XK_Insert]    = KEY_INSERT;
    x11_to_godot[XK_Delete]    = KEY_DELETE;
    x11_to_godot[XK_Home]      = KEY_HOME;
    x11_to_godot[XK_End]       = KEY_END;
    x11_to_godot[XK_Page_Up]   = KEY_PAGEUP;
    x11_to_godot[XK_Page_Down] = KEY_PAGEDOWN;

    // Arrow keys
    x11_to_godot[XK_Up]    = KEY_UP;
    x11_to_godot[XK_Down]  = KEY_DOWN;
    x11_to_godot[XK_Left]  = KEY_LEFT;
    x11_to_godot[XK_Right] = KEY_RIGHT;

    // Punctuation
    x11_to_godot[XK_comma]     = KEY_COMMA;
    x11_to_godot[XK_period]    = KEY_DOT;
    x11_to_godot[XK_slash]     = KEY_SLASH;
    x11_to_godot[XK_backslash] = KEY_BACKSLASH;
    x11_to_godot[XK_semicolon] = KEY_SEMICOLON;
    x11_to_godot[XK_apostrophe]= KEY_APOSTROPHE;
    x11_to_godot[XK_grave]     = KEY_GRAVE;
    x11_to_godot[XK_bracketleft]  = KEY_LEFTBRACE;
    x11_to_godot[XK_bracketright] = KEY_RIGHTBRACE;

    // Numpad
    x11_to_godot[XK_KP_0] = KEY_KP0;
    x11_to_godot[XK_KP_1] = KEY_KP1;
    x11_to_godot[XK_KP_2] = KEY_KP2;
    x11_to_godot[XK_KP_3] = KEY_KP3;
    x11_to_godot[XK_KP_4] = KEY_KP4;
    x11_to_godot[XK_KP_5] = KEY_KP5;
    x11_to_godot[XK_KP_6] = KEY_KP6;
    x11_to_godot[XK_KP_7] = KEY_KP7;
    x11_to_godot[XK_KP_8] = KEY_KP8;
    x11_to_godot[XK_KP_9] = KEY_KP9;
    x11_to_godot[XK_KP_Add]      = KEY_KPPLUS;
    x11_to_godot[XK_KP_Subtract] = KEY_KPMINUS;
    x11_to_godot[XK_KP_Multiply] = KEY_KPASTERISK;
    x11_to_godot[XK_KP_Decimal]  = KEY_KPDOT;

    // Media / extra keys
// Media / extra keys
x11_to_godot[XK_Print]   = KEY_PRINT;
x11_to_godot[XK_Pause]   = KEY_PAUSE;
x11_to_godot[XK_Menu]    = KEY_MENU;

x11_to_godot[XF86XK_AudioRaiseVolume] = KEY_VOLUMEUP;
x11_to_godot[XF86XK_AudioLowerVolume] = KEY_VOLUMEDOWN;
x11_to_godot[XF86XK_AudioMute]        = KEY_MUTE;
x11_to_godot[XF86XK_AudioPlay]        = KEY_PLAY;
x11_to_godot[XF86XK_AudioStop]        = KEY_STOP;
x11_to_godot[XF86XK_AudioNext]        = KEY_NEXT;
x11_to_godot[XF86XK_AudioPrev] = KEY_PREVIOUS;


}
#endif
