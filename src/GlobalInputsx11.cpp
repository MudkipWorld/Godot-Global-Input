#include "GlobalInputsx11.h"
#include "UnixEvents.h"
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
bool GlobalInputX11::running = false;
std::recursive_mutex GlobalInputX11::state_mutex;
std::thread GlobalInputX11::poll_thread;

#ifdef __linux__
Display *GlobalInputX11::display = nullptr;
Window GlobalInputX11::root_window = 0;
int GlobalInputX11::keyboard_fd = -1;
int GlobalInputX11::mice_fd = -1;
std::unordered_map<int,int> GlobalInputX11::x11_to_godot;
#endif

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
    current_frame++;
}

Vector2 GlobalInputX11::get_mouse_position() {
#ifdef __linux__
    if (!display) return mouse_position;
    Window root, child;
    int rx, ry, wx, wy;
    unsigned int mask;
    if (XQueryPointer(display, root_window, &root, &child, &rx, &ry, &wx, &wy, &mask)) {
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
    return it != key_just_pressed_frame.end() && (current_frame - it->second) <= 1;
}

bool GlobalInputX11::is_key_just_released(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_just_released_frame.find(key);
    return it != key_just_released_frame.end() && (current_frame - it->second) <= 1;
}

bool GlobalInputX11::is_mouse_pressed(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_state.find(button);
    return it != mouse_state.end() && it->second;
}

bool GlobalInputX11::is_mouse_just_pressed(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_just_pressed_frame.find(button);
    return it != mouse_just_pressed_frame.end() && (current_frame - it->second) <= 1;
}

bool GlobalInputX11::is_mouse_just_released(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_just_released_frame.find(button);
    return it != mouse_just_released_frame.end() && (current_frame - it->second) <= 1;
}

bool GlobalInputX11::is_action_pressed(const String &action_name) {
    if (!InputMap::get_singleton()) return false;
    const Array events = InputMap::get_singleton()->action_get_events(action_name);
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i=0;i<events.size();i++) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;
        if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
            if (!modifiers_match(key_ev)) continue;
            if (key_state[key_ev->get_keycode()]) return true;
        } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
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

    for (int i=0;i<events.size();i++) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;
        if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
            auto it = key_just_pressed_frame.find(key_ev->get_keycode());
            if (!modifiers_match(key_ev)) continue;
            if (it != key_just_pressed_frame.end() && (current_frame - it->second) <= 1) return true;
        } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
            auto it = mouse_just_pressed_frame.find(mouse_ev->get_button_index());
            if (!modifiers_match(mouse_ev)) continue;
            if (it != mouse_just_pressed_frame.end() && (current_frame - it->second) <= 1) return true;
        }
    }
    return false;
}

bool GlobalInputX11::is_action_just_released(const String &action_name) {
    if (!InputMap::get_singleton()) return false;
    const Array events = InputMap::get_singleton()->action_get_events(action_name);
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i=0;i<events.size();i++) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;
        if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
            auto it = key_just_released_frame.find(key_ev->get_keycode());
            if (!modifiers_match(key_ev)) continue;
            if (it != key_just_released_frame.end() && (current_frame - it->second) <= 1) return true;
        } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
            auto it = mouse_just_released_frame.find(mouse_ev->get_button_index());
            if (!modifiers_match(mouse_ev)) continue;
            if (it != mouse_just_released_frame.end() && (current_frame - it->second) <= 1) return true;
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
    for (const auto &[key, frame]: key_just_pressed_frame)
        if ((current_frame - frame) <= 1) dict[key] = true;
    return dict;
}
Dictionary GlobalInputX11::get_keys_just_released_detailed() {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    for (const auto &[key, frame]: key_just_released_frame)
        if ((current_frame - frame) <= 1) dict[key] = true;
    return dict;
}

void GlobalInputX11::poll_input() {
#ifdef __linux__
    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(state_mutex);
            if (!running) break;
            current_frame++;

            last_key_state = key_state;
            last_mouse_state = mouse_state;

            // -------- keyboard --------
            if (keyboard_fd != -1) {
                struct input_event ev;
                while (read(keyboard_fd,&ev,sizeof(ev))>0) {
                    if (ev.type != EV_KEY) continue;
                    int code = ev.code;
                    auto it = x11_to_godot.find(code);
                    if (it == x11_to_godot.end()) continue;
                    int godot_key = it->second;
                    bool pressed_now = ev.value == 1;
                    bool before = key_state[godot_key];
                    key_state[godot_key] = pressed_now;
                    if (pressed_now && !before) key_just_pressed_frame[godot_key] = current_frame;
                    if (!pressed_now && before) key_just_released_frame[godot_key] = current_frame;
                }
            }

            // -------- mouse --------
            if (mice_fd != -1) {
                unsigned char data[3];
                if (read(mice_fd,data,sizeof(data))>0) {
                    bool left   = data[0] & 0x1;
                    bool right  = data[0] & 0x2;
                    bool mid    = data[0] & 0x4;

                    auto handle_mouse = [&](int button, bool now) {
                        bool before = mouse_state[button];
                        mouse_state[button] = now;
                        if (now && !before) mouse_just_pressed_frame[button] = current_frame;
                        if (!now && before) mouse_just_released_frame[button] = current_frame;
                    };
                    handle_mouse(MOUSE_BUTTON_LEFT, left);
                    handle_mouse(MOUSE_BUTTON_RIGHT, right);
                    handle_mouse(MOUSE_BUTTON_MIDDLE, mid);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
#endif
}
