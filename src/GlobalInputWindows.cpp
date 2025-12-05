#include "GlobalInputWindows.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif



using namespace godot;


std::unordered_map<int, bool> GlobalInputWindows::key_state;
std::unordered_map<int, uint64_t> GlobalInputWindows::key_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputWindows::key_just_released_frame;

std::unordered_map<int, bool> GlobalInputWindows::mouse_state;
std::unordered_map<int, uint64_t> GlobalInputWindows::mouse_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputWindows::mouse_just_released_frame;
std::unordered_map<int, bool> GlobalInputWindows::last_key_state;
std::unordered_map<int, bool> GlobalInputWindows::last_mouse_state;

Vector2 GlobalInputWindows::mouse_position;
int GlobalInputWindows::wheel_delta = 0;

uint64_t GlobalInputWindows::current_frame = 0;
bool GlobalInputWindows::running = false;
std::recursive_mutex GlobalInputWindows::state_mutex;
std::thread GlobalInputWindows::poll_thread;


void GlobalInputWindows::start() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    if (running) return;

    running = true;
    init_key_map();

    poll_thread = std::thread(&GlobalInputWindows::poll_input, this);
}

void GlobalInputWindows::stop() {
    {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        running = false;
    }

    if (poll_thread.joinable())
        poll_thread.join();  
}


void GlobalInputWindows::increment_frame() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    current_frame++;
}

Vector2 GlobalInputWindows::get_mouse_position() {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    return mouse_position;
}

bool GlobalInputWindows::is_key_pressed(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_state.find(key);
    return it != key_state.end() && it->second;
}

bool GlobalInputWindows::is_key_just_pressed(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_just_pressed_frame.find(key);
    return it != key_just_pressed_frame.end() && (current_frame - it->second) <= 1;
}

bool GlobalInputWindows::is_key_just_released(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_just_released_frame.find(key);
    return it != key_just_released_frame.end() && (current_frame - it->second) <= 1;
}

bool GlobalInputWindows::is_mouse_pressed(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_state.find(button);
    return it != mouse_state.end() && it->second;
}

bool GlobalInputWindows::is_mouse_just_pressed(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_just_pressed_frame.find(button);
    return it != mouse_just_pressed_frame.end() && (current_frame - it->second) <= 1;
}

bool GlobalInputWindows::is_mouse_just_released(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_just_released_frame.find(button);
    return it != mouse_just_released_frame.end() && (current_frame - it->second) <= 1;
}

bool GlobalInputWindows::is_action_pressed(const String &action_name) {
    if (!InputMap::get_singleton()) return false;
    const Array events = InputMap::get_singleton()->action_get_events(action_name);
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i = 0; i < events.size(); i++) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;

        if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
            if (!modifiers_match(key_ev)) continue; 
            if (key_state[key_ev->get_keycode()]) return true;
        } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
            if (!modifiers_match(key_ev)) continue; 
            if (mouse_state[mouse_ev->get_button_index()]) return true;
        }
    }
    return false;
}

bool GlobalInputWindows::is_action_just_pressed(const String &action_name) {
    if (!InputMap::get_singleton()) return false;
    const Array events = InputMap::get_singleton()->action_get_events(action_name);
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i = 0; i < events.size(); i++) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;

        if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
            auto it = key_just_pressed_frame.find(key_ev->get_keycode());
            if (!modifiers_match(key_ev)) continue; 
            if (it != key_just_pressed_frame.end() && (current_frame - it->second) <= 1) return true;
        } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
            auto it = mouse_just_pressed_frame.find(mouse_ev->get_button_index());
            if (!modifiers_match(key_ev)) continue; 
            if (it != mouse_just_pressed_frame.end() && (current_frame - it->second) <= 1) return true;
        }
    }
    return false;
}

bool GlobalInputWindows::is_action_just_released(const String &action_name) {
    if (!InputMap::get_singleton()) return false;
    const Array events = InputMap::get_singleton()->action_get_events(action_name);
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i = 0; i < events.size(); i++) {
        Ref<InputEvent> ev = events[i];
        if (!ev.is_valid()) continue;

        if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
            auto it = key_just_released_frame.find(key_ev->get_keycode());
            if (!modifiers_match(key_ev)) continue; 
            if (it != key_just_released_frame.end() && (current_frame - it->second) <= 1) return true;
        } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
            auto it = mouse_just_released_frame.find(mouse_ev->get_button_index());
            if (!modifiers_match(key_ev)) continue; 
            if (it != mouse_just_released_frame.end() && (current_frame - it->second) <= 1) return true;
        }
    }
    return false;
}

#ifdef _WIN32
bool GlobalInputWindows::is_shift_pressed() {
    return is_key_pressed(VK_SHIFT) || is_key_pressed(VK_LSHIFT) || is_key_pressed(VK_RSHIFT) ||  is_key_pressed(KEY_SHIFT);
}

bool GlobalInputWindows::is_ctrl_pressed() {
    return is_key_pressed(VK_CONTROL) || is_key_pressed(VK_LCONTROL) || is_key_pressed(VK_RCONTROL) || is_key_pressed(KEY_CTRL);
}

bool GlobalInputWindows::is_alt_pressed() {
    return is_key_pressed(VK_MENU) || is_key_pressed(VK_LMENU) || is_key_pressed(VK_RMENU) || is_key_pressed(KEY_ALT);
}

bool GlobalInputWindows::is_meta_pressed() {
    return is_key_pressed(VK_LWIN) || is_key_pressed(VK_RWIN) || is_key_pressed(KEY_META);
}
#endif

bool GlobalInputWindows::modifiers_match(InputEvent *ev) {
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

Dictionary GlobalInputWindows::get_keys_pressed_detailed() {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    for (const auto &[key, down] : key_state) {
        if (!down) continue;
        String name = "Unknown";
        if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
            name = OS::get_singleton()->get_keycode_string((Key)key);
        dict[name] = true;
        dict["os"] = "Windows";
    }
    return dict;
}

Dictionary GlobalInputWindows::get_keys_just_pressed_detailed() {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    for (const auto &[key, frame] : key_just_pressed_frame) {
        if ((current_frame - frame) > 1) continue;
        String name = "Unknown";
        if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
            name = OS::get_singleton()->get_keycode_string((Key)key);
        dict[name] = true;
        dict["os"] = "Windows";
    }
    return dict;
}

Dictionary GlobalInputWindows::get_keys_just_released_detailed() {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    for (const auto &[key, frame] : key_just_released_frame) {
        if ((current_frame - frame) > 1) continue;
        String name = "Unknown";
        if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
            name = OS::get_singleton()->get_keycode_string((Key)key);
        dict[name] = true;
        dict["os"] = "Windows";
    }
    return dict;
}

void GlobalInputWindows::poll_input() {
#ifdef _WIN32
    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(state_mutex);
            if (!running) break;
            current_frame++;

            last_key_state = key_state;
            last_mouse_state = mouse_state;

            // Poll keys
            for (int vk = 0; vk < 256; ++vk) {
                bool pressed = (GetAsyncKeyState(vk) & 0x8000) != 0;
                int godot_key = translate_vk_to_godot(vk);
                if (godot_key != 0) {
                    if (pressed && !key_state[godot_key]) key_just_pressed_frame[godot_key] = current_frame;
                    if (!pressed && key_state[godot_key]) key_just_released_frame[godot_key] = current_frame;
                    key_state[godot_key] = pressed;
                }
            }

            // Poll mouse
            POINT p;
            if (GetCursorPos(&p)) {
                // Find which monitor the cursor is currently on
                HMONITOR monitor = MonitorFromPoint(p, MONITOR_DEFAULTTONEAREST);

                MONITORINFO mi;
                mi.cbSize = sizeof(MONITORINFO);
                if (GetMonitorInfo(monitor, &mi)) {
                    // Convert to monitor-relative coordinates
                    int rel_x = p.x - mi.rcMonitor.left;
                    int rel_y = p.y - mi.rcMonitor.top;
                    mouse_position = Vector2(rel_x, rel_y);
                } else {
                    // fallback to screen coordinates if something fails
                    mouse_position = Vector2(p.x, p.y);
                }
            }


            int buttons[] = {VK_LBUTTON, VK_RBUTTON, VK_MBUTTON};
            int godot_buttons[] = {MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT, MOUSE_BUTTON_MIDDLE};
            for (int i = 0; i < 3; i++) {
                bool pressed = (GetAsyncKeyState(buttons[i]) & 0x8000) != 0;
                if (pressed && !mouse_state[godot_buttons[i]]) mouse_just_pressed_frame[godot_buttons[i]] = current_frame;
                if (!pressed && mouse_state[godot_buttons[i]]) mouse_just_released_frame[godot_buttons[i]] = current_frame;
                mouse_state[godot_buttons[i]] = pressed;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }
#endif
}

#ifdef _WIN32
void GlobalInputWindows::init_key_map() {
    vk_to_godot.clear();
	// Fn vk_to_godot
	vk_to_godot[VK_F1] = KEY_F1;
	vk_to_godot[VK_F2] = KEY_F2;
	vk_to_godot[VK_F3] = KEY_F3;
	vk_to_godot[VK_F4] = KEY_F4;
	vk_to_godot[VK_F5] = KEY_F5;
	vk_to_godot[VK_F6] = KEY_F6;
	vk_to_godot[VK_F7] = KEY_F7;
	vk_to_godot[VK_F8] = KEY_F8;
	vk_to_godot[VK_F9] = KEY_F9;
	vk_to_godot[VK_F10] = KEY_F10;
	vk_to_godot[VK_F11] = KEY_F11;
	vk_to_godot[VK_F12] = KEY_F12;
	vk_to_godot[VK_F13] = KEY_F13;
	vk_to_godot[VK_F14] = KEY_F14;
	vk_to_godot[VK_F15] = KEY_F15;
	vk_to_godot[VK_F16] = KEY_F16;
	vk_to_godot[VK_F17] = KEY_F17;
	vk_to_godot[VK_F18] = KEY_F18;
	vk_to_godot[VK_F19] = KEY_F19;
	vk_to_godot[VK_F20] = KEY_F20;
	vk_to_godot[VK_F21] = KEY_F21;
	vk_to_godot[VK_F22] = KEY_F22;
	vk_to_godot[VK_F23] = KEY_F23;
	vk_to_godot[VK_F24] = KEY_F24;

	// Control vk_to_godot
	vk_to_godot[VK_CONTROL] = KEY_CTRL;
	// vk_to_godot[VK_LCONTROL] = KEY_CTRL;
	// vk_to_godot[VK_RCONTROL] = KEY_CTRL;
	vk_to_godot[VK_SHIFT] = KEY_SHIFT;
	// vk_to_godot[VK_LSHIFT] = KEY_SHIFT;
	// vk_to_godot[VK_RSHIFT] = KEY_SHIFT;
	vk_to_godot[VK_MENU] = KEY_ALT;
	// vk_to_godot[VK_LMENU] = KEY_ALT;
	// vk_to_godot[VK_RMENU] = KEY_ALT;
	vk_to_godot[VK_TAB] = KEY_TAB;
	vk_to_godot[VK_SPACE] = KEY_SPACE;
	vk_to_godot[VK_BACK] = KEY_BACKSPACE;
	vk_to_godot[VK_INSERT] = KEY_INSERT;
	vk_to_godot[VK_DELETE] = KEY_DELETE;
	vk_to_godot[VK_HOME] = KEY_HOME;
	vk_to_godot[VK_END] = KEY_END;
	vk_to_godot[VK_PRIOR] = KEY_PAGEUP;
	vk_to_godot[VK_NEXT] = KEY_PAGEDOWN;

	// Arrow vk_to_godot
	vk_to_godot[VK_UP] = KEY_UP;
	vk_to_godot[VK_DOWN] = KEY_DOWN;
	vk_to_godot[VK_LEFT] = KEY_LEFT;
	vk_to_godot[VK_RIGHT] = KEY_RIGHT;

	// Numpad vk_to_godot
	vk_to_godot[VK_NUMPAD0] = KEY_KP_0;
	vk_to_godot[VK_NUMPAD1] = KEY_KP_1;
	vk_to_godot[VK_NUMPAD2] = KEY_KP_2;
	vk_to_godot[VK_NUMPAD3] = KEY_KP_3;
	vk_to_godot[VK_NUMPAD4] = KEY_KP_4;
	vk_to_godot[VK_NUMPAD5] = KEY_KP_5;
	vk_to_godot[VK_NUMPAD6] = KEY_KP_6;
	vk_to_godot[VK_NUMPAD7] = KEY_KP_7;
	vk_to_godot[VK_NUMPAD8] = KEY_KP_8;
	vk_to_godot[VK_NUMPAD9] = KEY_KP_9;
	vk_to_godot[VK_NUMLOCK] = KEY_NUMLOCK;
	vk_to_godot[VK_ADD] = KEY_KP_ADD;
	vk_to_godot[VK_SUBTRACT] = KEY_KP_SUBTRACT;
	vk_to_godot[VK_MULTIPLY] = KEY_KP_MULTIPLY;
	vk_to_godot[VK_DIVIDE] = KEY_KP_DIVIDE;
	vk_to_godot[VK_DECIMAL] = KEY_KP_PERIOD;

	// Letters (these map out to the same as Godot)
	for (int i = KEY_A; i <= KEY_Z; i++) vk_to_godot[i] = i;

	// Numbers (these also map out to the same as Godot)
	for (int i = KEY_0; i <= KEY_9; i++) vk_to_godot[i] = i;

	// Regional vk_to_godot
	vk_to_godot[VK_OEM_1] = KEY_SEMICOLON;
	vk_to_godot[VK_OEM_2] = KEY_SLASH;
	vk_to_godot[VK_OEM_3] = KEY_ASCIITILDE;
	vk_to_godot[VK_OEM_4] = KEY_BRACKETLEFT;
	vk_to_godot[VK_OEM_5] = KEY_BACKSLASH;
	vk_to_godot[VK_OEM_6] = KEY_BRACKETRIGHT;
	vk_to_godot[VK_OEM_7] = KEY_QUOTEDBL;
	vk_to_godot[VK_OEM_PLUS] = KEY_PLUS;
	vk_to_godot[VK_OEM_COMMA] = KEY_COMMA;
	vk_to_godot[VK_OEM_MINUS] = KEY_MINUS;
	vk_to_godot[VK_OEM_PERIOD] = KEY_PERIOD;

	// Mouse buttons
	vk_to_godot[VK_LBUTTON] = MOUSE_BUTTON_LEFT;
	vk_to_godot[VK_RBUTTON] = MOUSE_BUTTON_RIGHT;
	vk_to_godot[VK_MBUTTON] = MOUSE_BUTTON_MIDDLE;
	vk_to_godot[VK_XBUTTON1] = MOUSE_BUTTON_XBUTTON1;
	vk_to_godot[VK_XBUTTON2] = MOUSE_BUTTON_XBUTTON2;

}

int GlobalInputWindows::translate_vk_to_godot(int vk) {
    auto it = vk_to_godot.find(vk);
    return it != vk_to_godot.end() ? it->second : 0;
}
#endif