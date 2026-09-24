#ifndef GLOBAL_INPUT_H
#define GLOBAL_INPUT_H
#pragma once
#include "trackers/common.h"

#ifdef _WIN32
#include "trackers/windows/windows_global_input.h"
#endif

#ifdef __linux__
#include "trackers/linux/x11_global_input.h"
#endif

#include "trackers/dummy.h"

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"
#include <memory>
#include <godot_cpp/classes/ref.hpp>


using namespace godot;

class GlobalInput : public Node {
    GDCLASS(GlobalInput, Node);

protected:
    static void _bind_methods();

public:
    GlobalInput();
    ~GlobalInput();

    // Hook control
    void start_hook();
    void stop_hook();
    void refresh_action_cache();
    void set_use_physics_frames(bool p_use) { use_physics_frames = p_use; }
    bool get_use_physics_frames() const { return use_physics_frames; }

    // Joypad stick/trigger deadzone. Single source of truth: the GDScript
    // binding UI reads it, so "a direction that can be bound" and "a direction
    // that triggers" can never diverge.
    void set_joy_deadzone(float p_deadzone) { GlobalInputCommon::set_joy_deadzone_value(p_deadzone); }
    float get_joy_deadzone() const { return GlobalInputCommon::joy_deadzone; }

    // Test hook (see GlobalInputCommon::joy_debug_enabled): drives joypad state
    // from script so the judgement chain can be regression-tested without a
    // physical pad. Godot offers no way to register a virtual joypad, and
    // Input.is_joy_button_pressed() reports false while no device exists.
    void set_debug_joy_enabled(bool p_enabled) { GlobalInputCommon::joy_debug_enabled = p_enabled; }
    bool get_debug_joy_enabled() const { return GlobalInputCommon::joy_debug_enabled; }
    void set_debug_joy_button(int p_button, bool p_pressed) { GlobalInputCommon::joy_debug_buttons[p_button] = p_pressed; }
    void set_debug_joy_axis(int p_axis, float p_value) { GlobalInputCommon::joy_debug_axes[p_axis] = p_value; }
    void clear_debug_joy() {
        GlobalInputCommon::joy_debug_buttons.clear();
        GlobalInputCommon::joy_debug_axes.clear();
    }


    void _process(double delta) override;
    void _physics_process(double delta) override;
    void _input(const Ref<InputEvent> &event) override;

    // Advances the per-frame snapshot at most once per engine frame.
    // Also called lazily from the checks below, so the snapshot stays correct
    // even when a GDScript attached to this node overrides _process.
    void ensure_frame_state();

    // Input Checks
    Vector2 get_mouse_position();
    bool is_key_pressed(int keycode);
    bool is_key_just_pressed(int keycode);
    bool is_key_just_released(int keycode);
    bool is_mouse_pressed(int button);
    bool is_mouse_just_pressed(int button);
    bool is_mouse_just_released(int button);

    // Event Checks
    bool is_input_pressed(const Ref<InputEvent> &event, bool inclusive);
    bool is_input_just_pressed(const Ref<InputEvent> &event, bool inclusive);
    bool is_input_just_released(const Ref<InputEvent> &event, bool inclusive);

    // Actions
    bool is_action_pressed(const String &action_name, bool inclusive);
    bool is_action_just_pressed(const String &action_name, bool inclusive);
    bool is_action_just_released(const String &action_name, bool inclusive);

    // Get Details
    Dictionary get_keys_pressed_detailed();
    Dictionary get_keys_just_pressed_detailed();
    Dictionary get_keys_just_released_detailed();

    // Modifier detection
    bool is_shift_pressed();
    bool is_ctrl_pressed();
    bool is_alt_pressed();
    bool is_meta_pressed();


    // Backend selection
    void set_backend(const String &backend_name);
    String get_backend();

private:
    enum BackendType {
        BACKEND_WINDOWS,
        BACKEND_X11,
        BACKEND_DUMMY
    };

    BackendType active_backend = BACKEND_DUMMY;
    Ref<GlobalInputCommon> backend;

    static bool hook_started;
    static bool use_physics_frames;
    String selected_backend = "dummy";
    int64_t last_frame_id = -1;

    void check_backend(){

        #ifdef _WIN32
            if (selected_backend == "windows") {
                backend = Ref<WindowsGlobalInput>(memnew(WindowsGlobalInput));
                active_backend = BACKEND_WINDOWS;
            } 
            else {
                backend = Ref<DummyGlobalInput>(memnew(DummyGlobalInput));
                active_backend = BACKEND_DUMMY;
            } 

        #endif

        #ifdef __linux__
            const char* wayland = std::getenv("WAYLAND_DISPLAY");
            if (wayland) {
                godot::print_line("Wayland detected, skipping global inputs for now.");
                backend = Ref<DummyGlobalInput>(memnew(DummyGlobalInput));
                active_backend = BACKEND_DUMMY;
                return;
            }
            else {
                if (selected_backend == "x11") {
                    backend = Ref<LinuxGlobalInput>(memnew(LinuxGlobalInput));
                    active_backend = BACKEND_X11;
                } 
                else {
                    backend = Ref<DummyGlobalInput>(memnew(DummyGlobalInput));
                    active_backend = BACKEND_DUMMY;
                } 
            }
        #endif

    }

};

#endif // GLOBAL_INPUT_H
