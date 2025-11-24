#pragma once
#include "GlobalInputCommon.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

using namespace godot;

class GlobalInputMac : public IGlobalInputBackend {
public:
    void start() override;
    void stop() override;
    void increment_frame() override;

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

    bool is_alt_pressed() override;
    bool is_ctrl_pressed() override;
    bool is_shift_pressed() override;
    bool is_meta_pressed() override;

    bool modifiers_match(InputEvent *key_ev);

private:
    // KEYBOARD
    static std::unordered_map<int, bool> key_state;
    static std::unordered_map<int, uint64_t> key_just_pressed_frame;
    static std::unordered_map<int, uint64_t> key_just_released_frame;

    // MOUSE
    static std::unordered_map<int, bool> mouse_state;
    static std::unordered_map<int, uint64_t> mouse_just_pressed_frame;
    static std::unordered_map<int, uint64_t> mouse_just_released_frame;

    static std::unordered_map<int, bool> last_key_state;
    static std::unordered_map<int, bool> last_mouse_state;

    static Vector2 mouse_position;
    static int wheel_delta;

    static uint64_t current_frame;
    static bool running;
    static std::recursive_mutex state_mutex;
    static std::thread poll_thread;

    void poll_input();

    // macOS: translation from macOS hardware keycode → Godot keycode
    int translate_mac_keycode(int mac_key);
    void init_key_map();
    std::unordered_map<int, int> mac_to_godot;
};
