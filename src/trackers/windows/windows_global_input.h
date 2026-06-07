#pragma once
#include "../common.h"

using namespace godot;

class WindowsGlobalInput : public GlobalInputCommon {
public:
    WindowsGlobalInput(){}
    ~WindowsGlobalInput(){ stop();}

    // Start/ Stop Hook

    void start() {
        if (running) return;

        if (!OS::get_singleton()) {
            running = false;
            return;
        }
        if (OS::get_singleton()->has_feature("editor_hint")){
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
        key_maps->get_platform_key_mapping(key_map);
        hook_thread = std::thread(&poll_input, this);
    }

    void stop(){
        if (running){
            std::lock_guard<std::recursive_mutex> lock(state_mutex);
            running = false;
            if (hook_thread.joinable())
                hook_thread.join();  
        }
    }

    // Polling Data

    void poll_data() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

        for (auto &it : key_just_pressed_frame) {
            if (it.second == 0) {
                it.second = current_frame;
            }
        }

        for (auto &it : key_just_released_frame) {
            if (it.second == 0) {
                it.second = current_frame;
            }
        }

        for (auto &it : mouse_just_pressed_frame) {
            if (it.second == 0) {
                it.second = current_frame;
            }
        }

        for (auto &it : mouse_just_released_frame) {
            if (it.second == 0) {
                it.second = current_frame;
            }
        }
    }

    void increment_frame() override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        current_frame++;    
    }

    // Basic Key Input

    bool is_key_pressed(int key) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_state.find(key);
        return it != key_state.end() && it->second;
    }

    bool is_key_just_pressed(int key) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_just_pressed_frame.find(key);
        return it != key_just_pressed_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_key_just_released(int key) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_just_released_frame.find(key);
        return it != key_just_released_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    // Mouse Input

    bool is_mouse_pressed(int button) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_state.find(button);
        return it != mouse_state.end() && it->second;
    }

    bool is_mouse_just_pressed(int button) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_just_pressed_frame.find(button);
        return it != mouse_just_pressed_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_mouse_just_released(int button) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_just_released_frame.find(button);
        return it != mouse_just_released_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    Vector2 get_mouse_position() override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        return mouse_position;
    }

    // Godot InputMap Action Detection

    bool is_action_pressed(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
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

    bool is_action_just_pressed(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> ev = events[i];
            if (!ev.is_valid()) continue;

            if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
                auto it = key_just_pressed_frame.find(key_ev->get_keycode());
                if (!modifiers_match(key_ev)) continue; 
                if (it != key_just_pressed_frame.end() &&
                    it->second != 0 &&
                    (current_frame - it->second) <= JUST_BUFFER_FRAMES)
                    return true;
            } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
                auto it = mouse_just_pressed_frame.find(mouse_ev->get_button_index());
                if (!modifiers_match(key_ev)) continue; 
                if (it != mouse_just_pressed_frame.end() &&
                    it->second != 0 &&
                    (current_frame - it->second) <= JUST_BUFFER_FRAMES)
                    return true;
            }
        }
        return false;
    }

    bool is_action_just_released(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
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
    
    // Debug Returns

    Dictionary get_keys_pressed_detailed() override{
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

    Dictionary get_keys_just_pressed_detailed() override{
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

    Dictionary get_keys_just_released_detailed() override{
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

    // Modifiers

    bool is_alt_pressed() override{
        #ifdef _WIN32
        return is_key_pressed(VK_MENU) || is_key_pressed(VK_LMENU) || is_key_pressed(VK_RMENU) || is_key_pressed(KEY_ALT);
        #endif
        return false;
    }

    bool is_ctrl_pressed() override{
        #ifdef _WIN32
        return is_key_pressed(VK_CONTROL) || is_key_pressed(VK_LCONTROL) || is_key_pressed(VK_RCONTROL) || is_key_pressed(KEY_CTRL);
        #endif
        return false;

    }

    bool is_shift_pressed() override{
        #ifdef _WIN32
        return is_key_pressed(VK_SHIFT) || is_key_pressed(VK_LSHIFT) || is_key_pressed(VK_RSHIFT) ||  is_key_pressed(KEY_SHIFT);
        #endif
        return false;
    }

    bool is_meta_pressed() override{
        #ifdef _WIN32
        return is_key_pressed(VK_LWIN) || is_key_pressed(VK_RWIN) || is_key_pressed(KEY_META);
        #endif
        return false;
    }

    // Misc

    void handle_input(const Ref<InputEvent> &event) override {};

    void poll_input() {
        #ifdef _WIN32
            while (running) {
                {
                    if (!OS::get_singleton()) {
                        running = false;
                        return;
                    }
                    
                    std::lock_guard<std::recursive_mutex> lock(state_mutex);

                    for (const auto &[vk, godot_key] : key_map) {
                        SHORT state = GetAsyncKeyState(vk);
                        bool pressed = (state & 0x8000) != 0;
                        bool was_pressed = key_state[godot_key];

                        key_state[godot_key] = pressed;

                        if (pressed && !was_pressed)
                            key_just_pressed_frame[godot_key] = 0;

                        if (!pressed && was_pressed)
                            key_just_released_frame[godot_key] = 0;
                    }

                    POINT p;
                    if (GetCursorPos(&p)) {
                        mouse_position = Vector2(p.x, p.y);
                    }

                    int buttons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
                    int godot_buttons[] = {
                        MOUSE_BUTTON_LEFT,
                        MOUSE_BUTTON_RIGHT,
                        MOUSE_BUTTON_MIDDLE
                    };

                    for (int i = 0; i < 3; i++) {
                        SHORT state = GetAsyncKeyState(buttons[i]);
                        bool pressed = (state & 0x8000) != 0;
                        bool was_pressed = mouse_state[godot_buttons[i]];

                        mouse_state[godot_buttons[i]] = pressed;

                        if (pressed && !was_pressed)
                            mouse_just_pressed_frame[godot_buttons[i]] = 0;

                        if (!pressed && was_pressed)
                            mouse_just_released_frame[godot_buttons[i]] = 0;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        #endif
    }


};
