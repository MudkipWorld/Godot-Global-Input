// GlobalInputUiohook.h
#ifndef GLOBAL_INPUT_UIOHOOK_H
#define GLOBAL_INPUT_UIOHOOK_H

#include "GlobalInputCommon.h"
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "uiohook.h"


using namespace godot;
class GlobalInputUiohook : public IGlobalInputBackend {
public:

    ~GlobalInputUiohook(){
        stop();
    }

    void start() override;
    void stop() override;
    void increment_frame() override;
    void poll_events();

    Vector2 get_mouse_position() override;

    bool is_key_pressed(int key) override;
    bool is_key_just_pressed(int key) override;
    bool is_key_just_released(int key) override;

    bool is_mouse_pressed(int button) override;
    bool is_mouse_just_pressed(int button) override;
    bool is_mouse_just_released(int button) override;

    bool is_action_pressed(const String &action) override;
    bool is_action_just_pressed(const String &action) override;
    bool is_action_just_released(const String &action) override;

    Dictionary get_keys_pressed_detailed() override;
    Dictionary get_keys_just_pressed_detailed() override;
    Dictionary get_keys_just_released_detailed() override;
    void poll_data() override;

    bool is_alt_pressed() override;
    bool is_ctrl_pressed() override;
    bool is_shift_pressed() override;
    bool is_meta_pressed() override;

    bool modifiers_match(InputEvent *ev);
    void poll_input_loop();

private:
    static std::unordered_map<int, bool> key_state;
    static std::unordered_map<int, uint64_t> key_just_pressed_frame;
    static std::unordered_map<int, uint64_t> key_just_released_frame;

    static std::unordered_map<int, bool> mouse_state;
    static std::unordered_map<int, uint64_t> mouse_just_pressed_frame;
    static std::unordered_map<int, uint64_t> mouse_just_released_frame;

    static std::unordered_map<int, bool> joy_state;
    static std::unordered_map<int, uint64_t> joy_just_pressed_frame;
    static std::unordered_map<int, uint64_t> joy_just_released_frame;

    static Vector2 mouse_position;
    static int wheel_delta;
    static uint64_t current_frame;

    static std::recursive_mutex state_mutex;
    static std::atomic<bool> running;
    static std::thread hook_thread;

    static void hook_event_dispatch(uiohook_event *event);
    static int translate_hook_key_to_godot(uint16_t hook_code);
};

#endif // GLOBAL_INPUT_UIOHOOK_H
