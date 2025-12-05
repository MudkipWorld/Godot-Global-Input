#ifndef GLOBAL_INPUT_H
#define GLOBAL_INPUT_H
#pragma once
#include "GlobalInputCommon.h"
#include "godot_cpp/classes/node.hpp"
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

    // Godot-accessible methods
    Vector2 get_mouse_position_b() { return get_mouse_position(); }
    bool is_key_pressed_b(int keycode) { return is_key_pressed(keycode); }
    bool is_key_just_pressed_b(int keycode) { return is_key_just_pressed(keycode); }
    bool is_key_just_released_b(int keycode) { return is_key_just_released(keycode); }
    bool is_mouse_pressed_b(int button) { return is_mouse_pressed(button); }
    bool is_mouse_just_pressed_b(int button) { return is_mouse_just_pressed(button); }
    bool is_mouse_just_released_b(int button) { return is_mouse_just_released(button); }
    bool is_action_pressed_b(const String &action) { return is_action_pressed(action); }
    bool is_action_just_pressed_b(const String &action) { return is_action_just_pressed(action); }
    bool is_action_just_released_b(const String &action) { return is_action_just_released(action); }
    Dictionary get_keys_pressed_detailed_b() { return get_keys_pressed_detailed(); }
    Dictionary get_keys_just_pressed_detailed_b() { return get_keys_just_pressed_detailed(); }
    Dictionary get_keys_just_released_detailed_b() { return get_keys_just_released_detailed(); }

    // Hook control
    void start_hook();
    void stop_hook();
    void set_use_physics_frames(bool p_use) { use_physics_frames = p_use; }
    bool get_use_physics_frames() const { return use_physics_frames; }


    void _process(double delta) override;
    void _physics_process(double delta) override;

    // Input queries
    Vector2 get_mouse_position();
    bool is_key_pressed(int keycode);
    bool is_key_just_pressed(int keycode);
    bool is_key_just_released(int keycode);
    bool is_mouse_pressed(int button);
    bool is_mouse_just_pressed(int button);
    bool is_mouse_just_released(int button);

    // Actions
    bool is_action_pressed(const String &action_name);
    bool is_action_just_pressed(const String &action_name);
    bool is_action_just_released(const String &action_name);

    // Detailed key states
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
    String get_active_backend_name() const;

private:
    enum BackendType {
        BACKEND_AUTO = 0,
        BACKEND_UIOHOOK,
        BACKEND_WINDOWS
    };

    BackendType active_backend = BACKEND_AUTO;
    Ref<IGlobalInputBackend> backend;

    static bool hook_started;
    static uint64_t current_frame;
    static bool use_physics_frames;
    String selected_backend = "default";
};

#endif // GLOBAL_INPUT_H
