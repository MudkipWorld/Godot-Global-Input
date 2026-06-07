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
    void set_use_physics_frames(bool p_use) { use_physics_frames = p_use; }
    bool get_use_physics_frames() const { return use_physics_frames; }


    void _process(double delta) override;
    void _physics_process(double delta) override;
    void _input(const Ref<InputEvent> &event) override;

    // Input Checks
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
    static uint64_t current_frame;
    static bool use_physics_frames;
    String selected_backend = "dummy";

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
            if (selected_backend == "x11") {
                backend = Ref<LinuxGlobalInput>(memnew(LinuxGlobalInput));
                active_backend = BACKEND_X11;
            } 
            else {
                backend = Ref<DummyGlobalInput>(memnew(DummyGlobalInput));
                active_backend = BACKEND_DUMMY;
            } 

        #endif

    }

};

#endif // GLOBAL_INPUT_H
