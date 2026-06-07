#pragma once
#include "common.h"

using namespace godot;

class DummyGlobalInput : public GlobalInputCommon {
public:
    DummyGlobalInput(){}
    ~DummyGlobalInput(){
        stop();
    }

    // Start/ Stop Hook

    void start() {
        if (running) return;

        if (!OS::get_singleton()) {
            running = false;
            return;
        }

        key_state.clear();
        key_just_pressed_frame.clear();
        key_just_released_frame.clear();

        mouse_state.clear();
        mouse_just_pressed_frame.clear();
        mouse_just_released_frame.clear();
        
        running = true;
    }

    void stop(){
        if (!running) return;
        running = false;
    }

    // Polling Data

    void poll_data() override {}

    void increment_frame(){
        current_frame++;
    }

    // Basic Key Input

    bool is_key_pressed(int key) override{
        check_key(key);
        return key_state[key];
    }

    bool is_key_just_pressed(int key) override{
        check_key(key);

        auto it = key_just_pressed_frame.find(key);
        return it != key_just_pressed_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_key_just_released(int key) override{
        check_key(key);

        auto it = key_just_released_frame.find(key);
        return it != key_just_released_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    // Mouse Input

    bool is_mouse_pressed(int button) override{
        return Input::get_singleton()->is_mouse_button_pressed((MouseButton)button);
    }

    bool is_mouse_just_pressed(int button) override{
        check_mouse(button);
        auto it = mouse_just_pressed_frame.find(button);

        return it != mouse_just_pressed_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_mouse_just_released(int button) override{
        check_mouse(button);
        auto it = mouse_just_released_frame.find(button);
        return it != mouse_just_released_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    Vector2 get_mouse_position() override{
        return DisplayServer::get_singleton()->mouse_get_position();
    }

    // Godot InputMap Action Detection

    bool is_action_pressed(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        bool has = InputMap::get_singleton()->has_action(action);
        if (!has) return false;
        return Input::get_singleton()->is_action_pressed(action);
    }

    bool is_action_just_pressed(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        bool has = InputMap::get_singleton()->has_action(action);
        if (!has) return false;
        return Input::get_singleton()->is_action_just_pressed(action);
    }

    bool is_action_just_released(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        bool has = InputMap::get_singleton()->has_action(action);
        if (!has) return false;
        return Input::get_singleton()->is_action_just_released(action);
    }
    
    // Debug Returns

    Dictionary get_keys_pressed_detailed() override{
        Dictionary dict;
        for (const auto &[key, down] : key_state){
            if (!down) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            if (OS::get_singleton()->has_feature("windows")) dict["os"] = "Windows";
            else if (OS::get_singleton()->has_feature("linuxbsd")) dict["os"] = "Linux or BSD"; 
        }
        return dict;
    }

    Dictionary get_keys_just_pressed_detailed() override{
        Dictionary dict;
        for (const auto &[key, frame] : key_just_pressed_frame) {
            if ((current_frame - frame) > 1) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            if (OS::get_singleton()->has_feature("windows")) dict["os"] = "Windows";
            else if (OS::get_singleton()->has_feature("linuxbsd")) dict["os"] = "Linux or BSD";

        }
        return dict;
    }

    Dictionary get_keys_just_released_detailed() override{
        Dictionary dict;
        for (const auto &[key, frame] : key_just_released_frame) {
            if ((current_frame - frame) > 1) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            if (OS::get_singleton()->has_feature("windows")) dict["os"] = "Windows";
            else if (OS::get_singleton()->has_feature("linuxbsd")) dict["os"] = "Linux or BSD";
        }
        return dict;
    }

    // Modifiers

    bool is_shift_pressed() {return Input::get_singleton()->is_key_pressed(KEY_SHIFT);}

    bool is_ctrl_pressed() {return Input::get_singleton()->is_key_pressed(KEY_CTRL);}

    bool is_alt_pressed() {return Input::get_singleton()->is_key_pressed(KEY_ALT);}

    bool is_meta_pressed() { return Input::get_singleton()->is_key_pressed(KEY_META);}

    bool modifiers_match(InputEvent *key_ev){
        bool ev_shift = false;
        bool ev_ctrl  = false;
        bool ev_alt   = false;
        bool ev_meta  = false;
        int  ev_keycode = 0;

        if (InputEventKey *key_ev = Object::cast_to<InputEventKey>(key_ev)) {
            ev_shift   = key_ev->is_shift_pressed();
            ev_ctrl    = key_ev->is_ctrl_pressed();
            ev_alt     = key_ev->is_alt_pressed();
            ev_meta    = key_ev->is_meta_pressed();
            ev_keycode = key_ev->get_keycode();
        } else if (InputEventMouseButton *mouse_ev = Object::cast_to<InputEventMouseButton>(key_ev)) {
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

    // Misc

    void handle_input(const Ref<InputEvent> &event) override {
        auto *key = Object::cast_to<InputEventKey>(*event);
        if (!key)
            return;

        int code = key->get_keycode();

        if (key->is_pressed() && !key->is_echo()) {
            key_state[code] = true;
            key_just_pressed_frame[code] = current_frame;
        } else if (!key->is_pressed()) {
            key_state[code] = false;
            key_just_released_frame[code] = current_frame;
        }

    }

    void check_key(int keycode) {
        Input *input = Input::get_singleton();
        if (!input) return;

        bool now = input->is_key_pressed((Key)keycode);
        bool prev = key_state[keycode];

        if (now && !prev) {
            key_just_pressed_frame[keycode] = current_frame;
        }
        if (!now && prev) {
            key_just_released_frame[keycode] = current_frame;
        }

        key_state[keycode] = now;
    }

    void check_mouse(int button) {
        Input *input = Input::get_singleton();
        if (!input) return;

        bool now = input->is_mouse_button_pressed((MouseButton)button);
        bool prev = mouse_state[button];

        if (now && !prev) {
            mouse_just_pressed_frame[button] = current_frame;
        }
        if (!now && prev) {
            mouse_just_released_frame[button] = current_frame;
        }

        mouse_state[button] = now;
    }

};
