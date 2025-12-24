#ifndef GLOBAL_INPUT_COMMON_H
#define GLOBAL_INPUT_COMMON_H


#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/object.hpp>
#include <unordered_set>
#include <unordered_map>
#include <thread>

using namespace godot;


class IGlobalInputBackend : public RefCounted{
public:
    virtual ~IGlobalInputBackend() {
    }

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

};
static constexpr uint64_t JUST_BUFFER_FRAMES = 5;



#endif // GLOBAL_INPUT_COMMON_H
