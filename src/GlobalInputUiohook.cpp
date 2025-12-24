#include "GlobalInputUiohook.h"

using namespace godot;


int GlobalInputUiohook::translate_hook_key_to_godot(uint16_t hook_code) {
    using namespace godot;

    switch (hook_code) {

        case VC_A: return KEY_A;
        case VC_B: return KEY_B;
        case VC_C: return KEY_C;
        case VC_D: return KEY_D;
        case VC_E: return KEY_E;
        case VC_F: return KEY_F;
        case VC_G: return KEY_G;
        case VC_H: return KEY_H;
        case VC_I: return KEY_I;
        case VC_J: return KEY_J;
        case VC_K: return KEY_K;
        case VC_L: return KEY_L;
        case VC_M: return KEY_M;
        case VC_N: return KEY_N;
        case VC_O: return KEY_O;
        case VC_P: return KEY_P;
        case VC_Q: return KEY_Q;
        case VC_R: return KEY_R;
        case VC_S: return KEY_S;
        case VC_T: return KEY_T;
        case VC_U: return KEY_U;
        case VC_V: return KEY_V;
        case VC_W: return KEY_W;
        case VC_X: return KEY_X;
        case VC_Y: return KEY_Y;
        case VC_Z: return KEY_Z;

        case VC_0: return KEY_0;
        case VC_1: return KEY_1;
        case VC_2: return KEY_2;
        case VC_3: return KEY_3;
        case VC_4: return KEY_4;
        case VC_5: return KEY_5;
        case VC_6: return KEY_6;
        case VC_7: return KEY_7;
        case VC_8: return KEY_8;
        case VC_9: return KEY_9;

        case VC_F1: return KEY_F1;
        case VC_F2: return KEY_F2;
        case VC_F3: return KEY_F3;
        case VC_F4: return KEY_F4;
        case VC_F5: return KEY_F5;
        case VC_F6: return KEY_F6;
        case VC_F7: return KEY_F7;
        case VC_F8: return KEY_F8;
        case VC_F9: return KEY_F9;
        case VC_F10: return KEY_F10;
        case VC_F11: return KEY_F11;
        case VC_F12: return KEY_F12;

        case VC_ESCAPE: return KEY_ESCAPE;
        case VC_BACKQUOTE: return KEY_ASCIITILDE;
        case VC_MINUS: return KEY_MINUS;
        case VC_EQUALS: return KEY_EQUAL;
        case VC_BACKSPACE: return KEY_BACKSPACE;
        case VC_TAB: return KEY_TAB;
        case VC_OPEN_BRACKET: return KEY_BRACKETLEFT;
        case VC_CLOSE_BRACKET: return KEY_BRACKETRIGHT;
        case VC_BACK_SLASH: return KEY_BACKSLASH;
        case VC_SEMICOLON: return KEY_SEMICOLON;
        case VC_QUOTE: return KEY_APOSTROPHE;
        case VC_ENTER: return KEY_ENTER;
        case VC_COMMA: return KEY_COMMA;
        case VC_PERIOD: return KEY_PERIOD;
        case VC_SLASH: return KEY_SLASH;
        case VC_SPACE: return KEY_SPACE;

        case VC_SHIFT_L: return KEY_SHIFT;
        case VC_SHIFT_R: return KEY_SHIFT;
        case VC_CONTROL_L: return KEY_CTRL;
        case VC_CONTROL_R: return KEY_CTRL;
        case VC_ALT_L: return KEY_ALT;
        case VC_ALT_R: return KEY_ALT;
        case VC_META_L: return KEY_META;
        case VC_META_R: return KEY_META;
        case VC_CAPS_LOCK: return KEY_CAPSLOCK;
        case VC_NUM_LOCK: return KEY_NUMLOCK;
        case VC_SCROLL_LOCK: return KEY_SCROLLLOCK;

        case VC_KP_0: return KEY_KP_0;
        case VC_KP_1: return KEY_KP_1;
        case VC_KP_2: return KEY_KP_2;
        case VC_KP_3: return KEY_KP_3;
        case VC_KP_4: return KEY_KP_4;
        case VC_KP_5: return KEY_KP_5;
        case VC_KP_6: return KEY_KP_6;
        case VC_KP_7: return KEY_KP_7;
        case VC_KP_8: return KEY_KP_8;
        case VC_KP_9: return KEY_KP_9;
        case VC_KP_SEPARATOR: return KEY_KP_PERIOD;
        case VC_KP_ADD: return KEY_KP_ADD;
        case VC_KP_SUBTRACT: return KEY_KP_SUBTRACT;
        case VC_KP_MULTIPLY: return KEY_KP_MULTIPLY;
        case VC_KP_DIVIDE: return KEY_KP_DIVIDE;
        case VC_KP_ENTER: return KEY_KP_ENTER;

        case VC_INSERT: return KEY_INSERT;
        case VC_DELETE: return KEY_DELETE;
        case VC_HOME: return KEY_HOME;
        case VC_END: return KEY_END;
        case VC_PAGE_UP: return KEY_PAGEUP;
        case VC_PAGE_DOWN: return KEY_PAGEDOWN;
        case VC_UP: return KEY_UP;
        case VC_DOWN: return KEY_DOWN;
        case VC_LEFT: return KEY_LEFT;
        case VC_RIGHT: return KEY_RIGHT;

        case VC_PRINTSCREEN: return KEY_PRINT;
        case VC_PAUSE: return KEY_PAUSE;
        case VC_CONTEXT_MENU: return KEY_MENU;

        case VC_KP_UP: return KEY_UP;
        case VC_KP_DOWN: return KEY_DOWN;
        case VC_KP_LEFT: return KEY_LEFT;
        case VC_KP_RIGHT: return KEY_RIGHT;

        case VC_KP_INSERT: return KEY_INSERT;
        case VC_KP_DELETE: return KEY_DELETE;
        case VC_KP_HOME: return KEY_HOME;
        case VC_KP_END: return KEY_END;
        case VC_KP_PAGE_UP: return KEY_PAGEUP;
        case VC_KP_PAGE_DOWN: return KEY_PAGEDOWN;


        default:
            return 0;
    }
}

std::unordered_map<int, bool> GlobalInputUiohook::key_state;
std::unordered_map<int, uint64_t> GlobalInputUiohook::key_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputUiohook::key_just_released_frame;

std::unordered_map<int, bool> GlobalInputUiohook::mouse_state;
std::unordered_map<int, uint64_t> GlobalInputUiohook::mouse_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputUiohook::mouse_just_released_frame;

std::unordered_map<int, bool> GlobalInputUiohook::joy_state;
std::unordered_map<int, uint64_t> GlobalInputUiohook::joy_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputUiohook::joy_just_released_frame;

int GlobalInputUiohook::wheel_delta = 0;
Vector2 GlobalInputUiohook::mouse_position;
uint64_t GlobalInputUiohook::current_frame = 0;
std::recursive_mutex GlobalInputUiohook::state_mutex;
std::atomic<bool> GlobalInputUiohook::running = false;
std::thread GlobalInputUiohook::hook_thread;

void GlobalInputUiohook::hook_event_dispatch(uiohook_event *event) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    if (!running) return;

    switch (event->type) {
        case EVENT_KEY_PRESSED: {
            int key = translate_hook_key_to_godot(event->data.keyboard.keycode);
            if (!key) break;

            bool was_pressed = key_state[key];
            if (!was_pressed) {
                key_state[key] = true;
                key_just_pressed_frame[key] = current_frame;
            }
            break;
        }

        case EVENT_KEY_RELEASED: {
            int key = translate_hook_key_to_godot(event->data.keyboard.keycode);
            if (!key) break;

            bool was_pressed = key_state[key];
            if (was_pressed) {
                key_state[key] = false;
                key_just_released_frame[key] = current_frame;
            }
            break;
        }

        case EVENT_MOUSE_PRESSED: {
            int btn = event->data.mouse.button;
            if (!mouse_state[btn]) {
                mouse_state[btn] = true;
                mouse_just_pressed_frame[btn] = current_frame;
            }
            break;
        }

        case EVENT_MOUSE_RELEASED: {
            int btn = event->data.mouse.button;
            if (mouse_state[btn]) {
                mouse_state[btn] = false;
                mouse_just_released_frame[btn] = current_frame;
            }
            break;
        }

        case EVENT_MOUSE_MOVED:
        case EVENT_MOUSE_DRAGGED:
            mouse_position = Vector2(event->data.mouse.x, event->data.mouse.y);
            break;

        case EVENT_MOUSE_WHEEL:
            wheel_delta = event->data.wheel.rotation;
            break;

        default:
            break;
    }
}


void GlobalInputUiohook::start() {
    if (running) return;
    running = true;

    hook_thread = std::thread([]() {
        hook_set_dispatch_proc(GlobalInputUiohook::hook_event_dispatch);
        int res = hook_run(); 
    });
    hook_thread.detach();
}

void GlobalInputUiohook::stop() {
    if (!running) return;
    running = false;

    hook_stop();   

    if (hook_thread.joinable()) {
        hook_thread.join(); 
    }
}

void GlobalInputUiohook::increment_frame() {
    std::lock_guard<std::recursive_mutex> lock(GlobalInputUiohook::state_mutex);
    current_frame++;
}

Vector2 GlobalInputUiohook::get_mouse_position() {
    std::lock_guard<std::recursive_mutex> lock(GlobalInputUiohook::state_mutex);
    return mouse_position;
}

bool GlobalInputUiohook::is_key_pressed(int keycode) {
    std::lock_guard<std::recursive_mutex> lock(GlobalInputUiohook::state_mutex);
    auto it = key_state.find(keycode);
    return it != key_state.end() && it->second;
}

bool GlobalInputUiohook::is_key_just_pressed(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_just_pressed_frame.find(key);
    return it != key_just_pressed_frame.end() &&
           (current_frame - it->second) <= JUST_BUFFER_FRAMES;
}

bool GlobalInputUiohook::is_key_just_released(int key) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = key_just_released_frame.find(key);
    return it != key_just_released_frame.end() &&
           (current_frame - it->second) <= JUST_BUFFER_FRAMES;
}

bool GlobalInputUiohook::is_mouse_pressed(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_state.find(button);
    return it != mouse_state.end() && it->second;
}

bool GlobalInputUiohook::is_mouse_just_pressed(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_just_pressed_frame.find(button);
    return it != mouse_just_pressed_frame.end() &&
           (current_frame - it->second) <= JUST_BUFFER_FRAMES;
}

bool GlobalInputUiohook::is_mouse_just_released(int button) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    auto it = mouse_just_released_frame.find(button);
    return it != mouse_just_released_frame.end() &&
           (current_frame - it->second) <= JUST_BUFFER_FRAMES;
}

bool GlobalInputUiohook::is_action_pressed(const String &action_name) {
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
        else if (ev->is_class("InputEventJoypadButton")) {
            InputEventJoypadButton *joy_ev = Object::cast_to<InputEventJoypadButton>(ev.ptr());
            if (!joy_ev) continue;

            if (joy_state[joy_ev->get_button_index()]) return true;
        }
    }

    return false;
}


bool GlobalInputUiohook::is_action_just_pressed(const String &action_name) {
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
        else if (ev->is_class("InputEventJoypadButton")) {
            InputEventJoypadButton *joy_ev = Object::cast_to<InputEventJoypadButton>(ev.ptr());
            if (!joy_ev) continue;

            int btn = joy_ev->get_button_index();
            auto it = joy_just_pressed_frame.find(btn);
            if (it != joy_just_pressed_frame.end() && (current_frame - it->second) <= 1) {
                return true;
            }
        }
    }

    return false;
}

bool GlobalInputUiohook::is_action_just_released(const String &action_name) {
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
        else if (ev->is_class("InputEventJoypadButton")) {
            InputEventJoypadButton *joy_ev = Object::cast_to<InputEventJoypadButton>(ev.ptr());
            if (!joy_ev) continue;

            int btn = joy_ev->get_button_index();
            auto it = joy_just_released_frame.find(btn);
            if (it != joy_just_released_frame.end() && (current_frame - it->second) <= 1) {

                return true;
            }
        }
    }

    return false;
}



bool GlobalInputUiohook::is_shift_pressed() {
    return is_key_pressed(KEY_SHIFT) ||
           is_key_pressed(VC_SHIFT_L) ||
           is_key_pressed(VC_SHIFT_R);
}

bool GlobalInputUiohook::is_ctrl_pressed() {
    return is_key_pressed(KEY_CTRL) ||
           is_key_pressed(VC_CONTROL_L) ||
           is_key_pressed(VC_CONTROL_R);
}

bool GlobalInputUiohook::is_alt_pressed() {
    return is_key_pressed(KEY_ALT) ||
           is_key_pressed(VC_ALT_L) ||
           is_key_pressed(VC_ALT_R);
}

bool GlobalInputUiohook::is_meta_pressed() {
    return is_key_pressed(KEY_META) ||
           is_key_pressed(VC_META_L) ||
           is_key_pressed(VC_META_R);
}

static Dictionary make_key_info(int keycode) {
    Dictionary info;
    info["keycode"] = keycode;
    info["name"] = OS::get_singleton()->get_keycode_string((Key)keycode);
    return info;
}

Dictionary GlobalInputUiohook::get_keys_pressed_detailed()  {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(GlobalInputUiohook::state_mutex);
    for (const auto &[key, down] : key_state)
        if (down && OS::get_singleton())
            dict[OS::get_singleton()->get_keycode_string((Key)key)] = true;
    return dict;
}

Dictionary GlobalInputUiohook::get_keys_just_pressed_detailed()  {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(GlobalInputUiohook::state_mutex);
    for (const auto &[key, frame] : key_just_pressed_frame)
        if ((current_frame - frame) <= 1 && OS::get_singleton()) 
            dict[OS::get_singleton()->get_keycode_string((Key)key)] = true;
    return dict;
}

Dictionary GlobalInputUiohook::get_keys_just_released_detailed()  {
    Dictionary dict;
    std::lock_guard<std::recursive_mutex> lock(GlobalInputUiohook::state_mutex);
    for (const auto &[key, frame] : key_just_released_frame)
        if ((current_frame - frame) <= 1 && OS::get_singleton())
            dict[OS::get_singleton()->get_keycode_string((Key)key)] = true;
    return dict;
}

bool GlobalInputUiohook::modifiers_match(InputEvent *ev) {
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

