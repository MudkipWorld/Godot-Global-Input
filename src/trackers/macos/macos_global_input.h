#pragma once

#include "../common.h"

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

#include <chrono>
#include <unordered_map>

using namespace godot;

class MacOSGlobalInput : public GlobalInputCommon {
public:
    MacOSGlobalInput() = default;

    ~MacOSGlobalInput() override {
        stop();
    }

    void start() override {
        if (running) return;
        if (!OS::get_singleton() || OS::get_singleton()->has_feature("editor_hint")) {
            running = false;
            return;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(state_mutex);
            key_state.clear();
            key_just_pressed_frame.clear();
            key_just_released_frame.clear();
            mouse_state.clear();
            mouse_just_pressed_frame.clear();
            mouse_just_released_frame.clear();
            key_maps->get_platform_key_mapping(key_map);
        }

#ifdef __APPLE__
        if (!CGPreflightListenEventAccess()) {
            CGRequestListenEventAccess();
            print_line("Global Input: grant Input Monitoring permission in System Settings to capture global input.");
        }

        running = true;
        hook_thread = std::thread(&MacOSGlobalInput::poll_input, this);
#endif
    }

    void stop() override {
        running = false;
        if (hook_thread.joinable()) {
            hook_thread.join();
        }
    }

    void poll_data() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        for (auto &entry : key_just_pressed_frame) {
            if (entry.second == 0) entry.second = current_frame;
        }
        for (auto &entry : key_just_released_frame) {
            if (entry.second == 0) entry.second = current_frame;
        }
        for (auto &entry : mouse_just_pressed_frame) {
            if (entry.second == 0) entry.second = current_frame;
        }
        for (auto &entry : mouse_just_released_frame) {
            if (entry.second == 0) entry.second = current_frame;
        }
    }

    void increment_frame() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        current_frame++;
    }

    bool is_key_pressed(int key) override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_state.find(key);
        return it != key_state.end() && it->second;
    }

    bool is_key_just_pressed(int key) override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_just_pressed_frame.find(key);
        return it != key_just_pressed_frame.end() && it->second != 0 &&
                (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_key_just_released(int key) override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_just_released_frame.find(key);
        return it != key_just_released_frame.end() && it->second != 0 &&
                (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_mouse_pressed(int button) override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_state.find(button);
        return it != mouse_state.end() && it->second;
    }

    bool is_mouse_just_pressed(int button) override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_just_pressed_frame.find(button);
        return it != mouse_just_pressed_frame.end() && it->second != 0 &&
                (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_mouse_just_released(int button) override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_just_released_frame.find(button);
        return it != mouse_just_released_frame.end() && it->second != 0 &&
                (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    Vector2 get_mouse_position() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        return mouse_position;
    }

    bool is_action_pressed(const String &action) override {
        if (!InputMap::get_singleton() || !InputMap::get_singleton()->has_action(action)) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> event = events[i];
            if (!event.is_valid()) continue;
            if (auto *key_event = Object::cast_to<InputEventKey>(event.ptr())) {
                if (modifiers_match(key_event) && key_state[key_event->get_keycode()]) return true;
            } else if (auto *mouse_event = Object::cast_to<InputEventMouseButton>(event.ptr())) {
                if (modifiers_match(mouse_event) && mouse_state[mouse_event->get_button_index()]) return true;
            }
        }
        return false;
    }

    bool is_action_just_pressed(const String &action) override {
        if (!InputMap::get_singleton() || !InputMap::get_singleton()->has_action(action)) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> event = events[i];
            if (!event.is_valid()) continue;
            if (auto *key_event = Object::cast_to<InputEventKey>(event.ptr())) {
                auto it = key_just_pressed_frame.find(key_event->get_keycode());
                if (modifiers_match(key_event) && it != key_just_pressed_frame.end() && it->second != 0 &&
                        (current_frame - it->second) <= JUST_BUFFER_FRAMES) return true;
            } else if (auto *mouse_event = Object::cast_to<InputEventMouseButton>(event.ptr())) {
                auto it = mouse_just_pressed_frame.find(mouse_event->get_button_index());
                if (modifiers_match(mouse_event) && it != mouse_just_pressed_frame.end() && it->second != 0 &&
                        (current_frame - it->second) <= JUST_BUFFER_FRAMES) return true;
            }
        }
        return false;
    }

    bool is_action_just_released(const String &action) override {
        if (!InputMap::get_singleton() || !InputMap::get_singleton()->has_action(action)) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> event = events[i];
            if (!event.is_valid()) continue;
            if (auto *key_event = Object::cast_to<InputEventKey>(event.ptr())) {
                auto it = key_just_released_frame.find(key_event->get_keycode());
                if (modifiers_match(key_event) && it != key_just_released_frame.end() && it->second != 0 &&
                        (current_frame - it->second) <= JUST_BUFFER_FRAMES) return true;
            } else if (auto *mouse_event = Object::cast_to<InputEventMouseButton>(event.ptr())) {
                auto it = mouse_just_released_frame.find(mouse_event->get_button_index());
                if (modifiers_match(mouse_event) && it != mouse_just_released_frame.end() && it->second != 0 &&
                        (current_frame - it->second) <= JUST_BUFFER_FRAMES) return true;
            }
        }
        return false;
    }

    Dictionary get_keys_pressed_detailed() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        Dictionary dict;
        for (const auto &[key, down] : key_state) {
            if (!down) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU) {
                name = OS::get_singleton()->get_keycode_string((Key)key);
            }
            dict[name] = true;
        }
        dict["os"] = "macOS";
        return dict;
    }

    Dictionary get_keys_just_pressed_detailed() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        Dictionary dict;
        for (const auto &[key, frame] : key_just_pressed_frame) {
            if (frame == 0 || (current_frame - frame) > 1) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU) {
                name = OS::get_singleton()->get_keycode_string((Key)key);
            }
            dict[name] = true;
        }
        dict["os"] = "macOS";
        return dict;
    }

    Dictionary get_keys_just_released_detailed() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        Dictionary dict;
        for (const auto &[key, frame] : key_just_released_frame) {
            if (frame == 0 || (current_frame - frame) > 1) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU) {
                name = OS::get_singleton()->get_keycode_string((Key)key);
            }
            dict[name] = true;
        }
        dict["os"] = "macOS";
        return dict;
    }

    bool is_shift_pressed() override { return is_key_pressed(KEY_SHIFT); }
    bool is_ctrl_pressed() override { return is_key_pressed(KEY_CTRL); }
    bool is_alt_pressed() override { return is_key_pressed(KEY_ALT); }
    bool is_meta_pressed() override { return is_key_pressed(KEY_META); }

    void handle_input(const Ref<InputEvent> &event) override {}

    void poll_input() {
#ifdef __APPLE__
        while (running) {
            std::unordered_map<int, bool> current_key_states;
            for (const auto &[mac_key, godot_key] : key_map) {
                current_key_states[godot_key] = current_key_states[godot_key] ||
                        CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)mac_key);
            }

            const bool left_pressed = CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft);
            const bool right_pressed = CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonRight);
            const bool middle_pressed = CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonCenter);

            CGPoint position = CGPointZero;
            CGEventRef event = CGEventCreate(NULL);
            if (event) {
                position = CGEventGetLocation(event);
                CFRelease(event);
            }

            {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                for (const auto &[key, pressed] : current_key_states) {
                    const bool was_pressed = key_state[key];
                    key_state[key] = pressed;
                    if (pressed && !was_pressed) key_just_pressed_frame[key] = 0;
                    if (!pressed && was_pressed) key_just_released_frame[key] = 0;
                }

                update_mouse_state(MOUSE_BUTTON_LEFT, left_pressed);
                update_mouse_state(MOUSE_BUTTON_RIGHT, right_pressed);
                update_mouse_state(MOUSE_BUTTON_MIDDLE, middle_pressed);
                mouse_position = Vector2(position.x, position.y);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
#endif
    }

private:
    void update_mouse_state(int button, bool pressed) {
        const bool was_pressed = mouse_state[button];
        mouse_state[button] = pressed;
        if (pressed && !was_pressed) mouse_just_pressed_frame[button] = 0;
        if (!pressed && was_pressed) mouse_just_released_frame[button] = 0;
    }
};
