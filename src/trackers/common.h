#pragma once
#ifndef GLOBAL_INPUT_COMMON_H
#define GLOBAL_INPUT_COMMON_H

#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/os.hpp>
#include "godot_cpp/classes/display_server.hpp"
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/object.hpp>
#include <unordered_map>
#include <thread>

#include "keymaps.h"

using namespace godot;

class GlobalInputCommon : public RefCounted{
public:
    virtual ~GlobalInputCommon() {}

    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void increment_frame() = 0;
    virtual Vector2 get_mouse_position() = 0;

    virtual bool is_key_pressed(int key) = 0;
    virtual bool is_key_just_pressed(int key) = 0;
    virtual bool is_key_just_released(int key) = 0;

    virtual bool is_mouse_pressed(int button) = 0;
    virtual bool is_mouse_just_pressed(int button) = 0;
    virtual bool is_mouse_just_released(int button) = 0;

    virtual bool is_action_pressed(const String &action) = 0;
    virtual bool is_action_just_pressed(const String &action) = 0;
    virtual bool is_action_just_released(const String &action) = 0;

    virtual Dictionary get_keys_pressed_detailed() = 0;
    virtual Dictionary get_keys_just_pressed_detailed() = 0;
    virtual Dictionary get_keys_just_released_detailed() = 0;

    virtual bool is_alt_pressed() = 0;
    virtual bool is_ctrl_pressed() = 0;
    virtual bool is_shift_pressed() = 0;
    virtual bool is_meta_pressed() = 0;

    virtual void poll_data() = 0;
    virtual void handle_input(const Ref<InputEvent> &event) = 0;

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


    KeyMaps* key_maps = new KeyMaps();

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

    static std::atomic<bool> running;
    static std::thread hook_thread;
    std::recursive_mutex state_mutex;

    std::unordered_map<int, int> key_map;


};
static constexpr uint64_t JUST_BUFFER_FRAMES = 1;

inline std::unordered_map<int, bool> GlobalInputCommon::key_state;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::key_just_pressed_frame;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::key_just_released_frame;

inline std::unordered_map<int, bool> GlobalInputCommon::mouse_state;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::mouse_just_pressed_frame;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::mouse_just_released_frame;

inline int GlobalInputCommon::wheel_delta = 0;
inline Vector2 GlobalInputCommon::mouse_position;
inline uint64_t GlobalInputCommon::current_frame = 0;
inline std::atomic<bool> GlobalInputCommon::running = false;
inline std::thread GlobalInputCommon::hook_thread;


#endif
