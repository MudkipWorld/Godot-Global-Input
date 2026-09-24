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
#include <godot_cpp/variant/typed_array.hpp>
#include <unordered_map>
#include <vector>
#include <thread>

#include "keymaps.h"

using namespace godot;

// Window (in frames) during which a just_pressed / just_released edge is still
// reported. Hoisted above the class so the joypad checks can share it.
static constexpr uint64_t JUST_BUFFER_FRAMES = 1;

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

    virtual bool is_action_pressed(const String &action, bool inclusive = false) = 0;
    virtual bool is_action_just_pressed(const String &action, bool inclusive = false) = 0;
    virtual bool is_action_just_released(const String &action, bool inclusive = false) = 0;

    virtual Dictionary get_keys_pressed_detailed() = 0;
    virtual Dictionary get_keys_just_pressed_detailed() = 0;
    virtual Dictionary get_keys_just_released_detailed() = 0;

    // Modifier state comes from the per-frame snapshot built in update_frame_state(),
    // so an event check costs no hash lookups and cannot tear mid-check.
    //
    // Every platform keys key_state / frame_keys by *Godot* keycode and folds the
    // left/right variants of a modifier onto that single keycode, so one lookup per
    // modifier covers every keyboard. Never query these with raw platform constants:
    // on Windows VK_LWIN is 0x5B == 91 == KEY_BRACKETLEFT and VK_RWIN is 0x5C == 92
    // == KEY_BACKSLASH, which silently aliases real keys and makes '[' and '\'
    // report Meta as held.
    virtual bool is_alt_pressed() { return frame_alt; }
    virtual bool is_ctrl_pressed() { return frame_ctrl; }
    virtual bool is_shift_pressed() { return frame_shift; }
    virtual bool is_meta_pressed() { return frame_meta; }

    virtual void poll_data() = 0;
    virtual void handle_input(const Ref<InputEvent> &event) = 0;

    struct StringHasher {
        size_t operator()(const String &s) const { return (size_t)s.hash(); }
    };

    static constexpr uint64_t ACTION_CACHE_FRAMES = 60;

    const Array &get_action_events(const String &action) {
        if (!action_cache_valid || current_frame - action_cache_frame >= ACTION_CACHE_FRAMES) {
            action_cache.clear();
            action_cache_frame = current_frame;
            action_cache_valid = true;
        }

        auto it = action_cache.find(action);
        if (it != action_cache.end()) return it->second;

        Array events;
        InputMap *im = InputMap::get_singleton();
        if (im && im->has_action(action)) events = im->action_get_events(action);
        return action_cache.emplace(action, events).first->second;
    }

    void refresh_action_cache() {
        action_cache.clear();
        action_cache_frame = current_frame;
        action_cache_valid = true;
    }

    Dictionary keys_to_detailed(const std::unordered_map<int, bool> &keys) {
        Dictionary dict;
        if (!OS::get_singleton()) return dict;

        for (const auto &[key, down] : keys) {
            if (!down) continue;
            dict[OS::get_singleton()->get_keycode_string((Key)key)] = true;
        }

        if (!dict.is_empty()) dict["os"] = OS::get_singleton()->get_name();
        return dict;
    }

    void update_frame_state() {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

        frame_keys_prev = frame_keys;
        frame_mouse_prev = frame_mouse;

        frame_keys.clear();
        for (const auto &[key, down] : key_state) {
            if (down) frame_keys[key] = true;
        }

        frame_mouse.clear();
        for (const auto &[button, down] : mouse_state) {
            if (down) frame_mouse[button] = true;
        }

        // Snapshot the modifier state once per frame instead of looking it up again
        // for every event of every action.
        frame_ctrl = frame_keys.count(KEY_CTRL) > 0;
        frame_shift = frame_keys.count(KEY_SHIFT) > 0;
        frame_alt = frame_keys.count(KEY_ALT) > 0;
        frame_meta = frame_keys.count(KEY_META) > 0;

        keys_just_pressed.clear();
        keys_just_released.clear();
        for (const auto &[key, down] : frame_keys) {
            if (!frame_keys_prev.count(key)) keys_just_pressed[key] = true;
        }
        for (const auto &[key, down] : frame_keys_prev) {
            if (!frame_keys.count(key)) keys_just_released[key] = true;
        }

        mouse_just_pressed.clear();
        mouse_just_released.clear();
        for (const auto &[button, down] : frame_mouse) {
            if (!frame_mouse_prev.count(button)) mouse_just_pressed[button] = true;
        }
        for (const auto &[button, down] : frame_mouse_prev) {
            if (!frame_mouse.count(button)) mouse_just_released[button] = true;
        }
    }

    struct EventInfo {
        bool valid = false;
        bool is_mouse = false;
        int code = 0;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        bool meta = false;

        // Joypad. `joy_positive` is the direction of a stick axis and is always
        // true for a button. Modifiers never apply to a joypad event.
        bool is_joy = false;
        bool is_joy_axis = false;
        int joy_code = 0;
        bool joy_positive = true;
    };

    EventInfo parse_event(InputEvent *ev) {
        EventInfo info;
        if (InputEventKey *key_ev = Object::cast_to<InputEventKey>(ev)) {
            info.valid = true;
            info.code = key_ev->get_keycode();
            info.ctrl = key_ev->is_ctrl_pressed();
            info.shift = key_ev->is_shift_pressed();
            info.alt = key_ev->is_alt_pressed();
            info.meta = key_ev->is_meta_pressed();
        } else if (InputEventMouseButton *mouse_ev = Object::cast_to<InputEventMouseButton>(ev)) {
            info.valid = true;
            info.is_mouse = true;
            info.code = mouse_ev->get_button_index();
            info.ctrl = mouse_ev->is_ctrl_pressed();
            info.shift = mouse_ev->is_shift_pressed();
            info.alt = mouse_ev->is_alt_pressed();
            info.meta = mouse_ev->is_meta_pressed();
        } else if (InputEventJoypadButton *joy_button_ev = Object::cast_to<InputEventJoypadButton>(ev)) {
            info.valid = true;
            info.is_joy = true;
            info.joy_code = joy_button_ev->get_button_index();
        } else if (InputEventJoypadMotion *joy_motion_ev = Object::cast_to<InputEventJoypadMotion>(ev)) {
            info.valid = true;
            info.is_joy = true;
            info.is_joy_axis = true;
            info.joy_code = joy_motion_ev->get_axis();
            // The direction is the only thing a binding stores: magnitudes vary
            // per device, and a stick returning to centre reports ~0, not the
            // pushed value, so the sign is what stays comparable.
            info.joy_positive = joy_motion_ev->get_axis_value() >= 0.0f;
        }
        return info;
    }

    // --- Joypad (read lazily) ------------------------------------------------
    //
    // Joypad state is sampled only when a bound joypad event is actually asked
    // about, so an app with no joypad bindings pays nothing per frame. Key and
    // mouse cannot work this way: get_keys_pressed_detailed() and the inclusive
    // checks want the whole pressed set, while here nothing ever asks for "all
    // joypad buttons" -- only "is this one bound event down".
    //
    // The device list is cached once per engine frame, so a model with N bound
    // joypad events does not call get_connected_joypads() N times.

    static constexpr int JOY_SLOT_AXIS_OFFSET = 256;
    static constexpr float JOY_DEADZONE_DEFAULT = 0.5f;

    // Static on purpose: a backend rebuild (set_backend) must not reset it.
    static float joy_deadzone;

    // Clamped, so a deadzone of 1.0 (never crossable) or 0.0 (a resting stick
    // counts as pushed) cannot be configured from script.
    static void set_joy_deadzone_value(float p_value) {
        if (p_value < 0.05f) p_value = 0.05f;
        else if (p_value > 0.95f) p_value = 0.95f;
        joy_deadzone = p_value;
    }

    // Test hook: lets the joypad judgement chain be exercised without a physical
    // pad, which is the only way to regression-test it headless (Godot offers no
    // way to register a virtual joypad). Off by default and free when off.
    inline static bool joy_debug_enabled = false;
    inline static std::unordered_map<int, bool> joy_debug_buttons;
    inline static std::unordered_map<int, float> joy_debug_axes;

    void ensure_joy_devices() {
        if (joy_devices_frame == current_frame) return;
        joy_devices_frame = current_frame;

        joy_devices.clear();
        Input *input = Input::get_singleton();
        if (!input) return;

        TypedArray<int> pads = input->get_connected_joypads();
        for (int i = 0; i < pads.size(); i++) {
            joy_devices.push_back((int)pads[i]);
        }
    }

    // Slot id of one bindable joypad thing: a button, or one direction of a
    // stick axis. Device is intentionally not part of the key (see the
    // joy_state declaration at the bottom of this file).
    static int joy_slot(int p_code, bool p_is_axis, bool p_positive) {
        return (p_is_axis ? JOY_SLOT_AXIS_OFFSET : 0) + p_code * 2 + (p_positive ? 1 : 0);
    }

    // Reads one slot, diffs it against the previous read and timestamps the
    // edge. Deliberately mirrors DummyGlobalInput::check_key().
    bool poll_joy_slot(int p_code, bool p_is_axis, bool p_positive) {
        ensure_joy_devices();

        bool now = false;
        Input *input = Input::get_singleton();
        if (joy_debug_enabled) {
            // Same threshold and direction rule as the real path, so a green
            // regression here means the real path agrees.
            if (p_is_axis) {
                const float value = joy_debug_axes.count(p_code) ? joy_debug_axes[p_code] : 0.0f;
                now = p_positive ? (value >= joy_deadzone) : (value <= -joy_deadzone);
            } else {
                now = joy_debug_buttons.count(p_code) > 0 && joy_debug_buttons[p_code];
            }
        } else if (input && !joy_devices.empty()) {
            for (size_t i = 0; i < joy_devices.size(); i++) {
                const int device = joy_devices[i];
                if (p_is_axis) {
                    const float value = input->get_joy_axis(device, (JoyAxis)p_code);
                    // The sign picks the direction, the magnitude only has to
                    // clear the deadzone. The binding UI uses this same
                    // threshold, so a direction that could be bound always
                    // triggers.
                    if (p_positive ? (value >= joy_deadzone) : (value <= -joy_deadzone)) {
                        now = true;
                        break;
                    }
                } else if (input->is_joy_button_pressed(device, (JoyButton)p_code)) {
                    now = true;
                    break;
                }
            }
        }

        const int slot = joy_slot(p_code, p_is_axis, p_positive);
        if (now != joy_state[slot]) {
            joy_state[slot] = now;
            // Stamped one frame back, exactly like the key/mouse path: the
            // key/mouse hook writes its edge marker inside poll_data(), which
            // GlobalInput::ensure_frame_state() runs *before* it advances the
            // counter, while this lazy read always happens after. Stamping the
            // live counter here would leave the edge inside the
            // JUST_BUFFER_FRAMES window for two frames in a row, so every bound
            // action fired twice (measured: true on frames 0 and 1).
            const uint64_t stamp = current_frame > 0 ? current_frame - 1 : 0;
            if (now) joy_just_pressed_frame[slot] = stamp;
            else joy_just_released_frame[slot] = stamp;
        }
        return now;
    }

    bool is_joy_pressed(const EventInfo &e) {
        return poll_joy_slot(e.joy_code, e.is_joy_axis, e.joy_positive);
    }

    bool is_joy_just_pressed(const EventInfo &e) {
        const int slot = joy_slot(e.joy_code, e.is_joy_axis, e.joy_positive);
        poll_joy_slot(e.joy_code, e.is_joy_axis, e.joy_positive);

        auto it = joy_just_pressed_frame.find(slot);
        return it != joy_just_pressed_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_joy_just_released(const EventInfo &e) {
        const int slot = joy_slot(e.joy_code, e.is_joy_axis, e.joy_positive);
        poll_joy_slot(e.joy_code, e.is_joy_axis, e.joy_positive);

        auto it = joy_just_released_frame.find(slot);
        return it != joy_just_released_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool event_in_set(const EventInfo &e, const std::unordered_map<int, bool> &keys, const std::unordered_map<int, bool> &mouse) {
        const auto &set = e.is_mouse ? mouse : keys;
        return set.count(e.code) > 0;
    }

    bool any_in_set(const EventInfo &e, const std::unordered_map<int, bool> &keys, const std::unordered_map<int, bool> &mouse) {
        if (event_in_set(e, keys, mouse)) return true;
        if (e.ctrl && keys.count(KEY_CTRL)) return true;
        if (e.shift && keys.count(KEY_SHIFT)) return true;
        if (e.alt && keys.count(KEY_ALT)) return true;
        if (e.meta && keys.count(KEY_META)) return true;
        return false;
    }

    bool modifiers_match_state(const EventInfo &e) {
        bool want_ctrl = e.ctrl || (!e.is_mouse && e.code == KEY_CTRL);
        bool want_shift = e.shift || (!e.is_mouse && e.code == KEY_SHIFT);
        bool want_alt = e.alt || (!e.is_mouse && e.code == KEY_ALT);
        bool want_meta = e.meta || (!e.is_mouse && e.code == KEY_META);

        return want_ctrl == is_ctrl_pressed() &&
            want_shift == is_shift_pressed() &&
            want_alt == is_alt_pressed() &&
            want_meta == is_meta_pressed();
    }

    // Inclusive check: every modifier of the event has to be held, extra ones are allowed.
    bool modifiers_contained(const EventInfo &e) {
        if ((e.ctrl || (!e.is_mouse && e.code == KEY_CTRL)) && !is_ctrl_pressed()) return false;
        if ((e.shift || (!e.is_mouse && e.code == KEY_SHIFT)) && !is_shift_pressed()) return false;
        if ((e.alt || (!e.is_mouse && e.code == KEY_ALT)) && !is_alt_pressed()) return false;
        if ((e.meta || (!e.is_mouse && e.code == KEY_META)) && !is_meta_pressed()) return false;
        return true;
    }

    bool check_input_sets(const EventInfo &e, bool inclusive, const std::unordered_map<int, bool> &keys, const std::unordered_map<int, bool> &mouse) {
        if (!inclusive) {
            if (!event_in_set(e, keys, mouse)) return false;
            return modifiers_match_state(e);
        }

        if (!any_in_set(e, keys, mouse)) return false;
        if (!event_in_set(e, frame_keys, frame_mouse)) return false;
        return modifiers_contained(e);
    }

    bool is_input_pressed(const Ref<InputEvent> &event, bool inclusive) {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        EventInfo e = parse_event(event.ptr());
        if (!e.valid) return false;
        // A joypad event carries no modifiers, so the key/mouse set logic --
        // which is entirely about modifier agreement -- does not apply to it.
        if (e.is_joy) return is_joy_pressed(e);
        return check_input_sets(e, inclusive, frame_keys, frame_mouse);
    }

    bool is_input_just_pressed(const Ref<InputEvent> &event, bool inclusive) {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        EventInfo e = parse_event(event.ptr());
        if (!e.valid) return false;
        if (e.is_joy) return is_joy_just_pressed(e);
        return check_input_sets(e, inclusive, keys_just_pressed, mouse_just_pressed);
    }

    bool is_input_just_released(const Ref<InputEvent> &event, bool inclusive) {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        EventInfo e = parse_event(event.ptr());
        if (!e.valid) return false;
        if (e.is_joy) return is_joy_just_released(e);

        int event_count = 1;
        int pressed_count = 0;
        int released_count = 0;

        if (event_in_set(e, frame_keys, frame_mouse)) pressed_count++;
        if (event_in_set(e, keys_just_released, mouse_just_released)) released_count++;

        if (e.ctrl) {
            event_count++;
            if (is_ctrl_pressed()) pressed_count++;
            if (keys_just_released.count(KEY_CTRL)) released_count++;
        }
        if (e.shift) {
            event_count++;
            if (is_shift_pressed()) pressed_count++;
            if (keys_just_released.count(KEY_SHIFT)) released_count++;
        }
        if (e.alt) {
            event_count++;
            if (is_alt_pressed()) pressed_count++;
            if (keys_just_released.count(KEY_ALT)) released_count++;
        }
        if (e.meta) {
            event_count++;
            if (is_meta_pressed()) pressed_count++;
            if (keys_just_released.count(KEY_META)) released_count++;
        }

        return released_count >= 1 && pressed_count + released_count == event_count;
    }

    bool any_action_event_matches(const String &action, bool inclusive, int check_mode) {
        if (!InputMap::get_singleton()) return false;
        const Array &events = get_action_events(action);

        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> ev = events[i];
            if (!ev.is_valid()) continue;

            if (check_mode == 0 && is_input_pressed(ev, inclusive)) return true;
            if (check_mode == 1 && is_input_just_pressed(ev, inclusive)) return true;
            if (check_mode == 2 && is_input_just_released(ev, inclusive)) return true;
        }
        return false;
    }

    std::unordered_map<String, Array, StringHasher> action_cache;
    uint64_t action_cache_frame = 0;
    bool action_cache_valid = false;

    std::unordered_map<int, bool> frame_keys;
    std::unordered_map<int, bool> frame_mouse;
    std::unordered_map<int, bool> frame_keys_prev;
    std::unordered_map<int, bool> frame_mouse_prev;

    bool frame_ctrl = false;
    bool frame_shift = false;
    bool frame_alt = false;
    bool frame_meta = false;

    std::unordered_map<int, bool> keys_just_pressed;
    std::unordered_map<int, bool> keys_just_released;
    std::unordered_map<int, bool> mouse_just_pressed;
    std::unordered_map<int, bool> mouse_just_released;

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

    // Cached connected-device ids, refreshed at most once per engine frame so a
    // model with many bound joypad events does not re-query the device list.
    std::vector<int> joy_devices;
    uint64_t joy_devices_frame = 0;

    std::unordered_map<int, int> key_map;


};

inline std::unordered_map<int, bool> GlobalInputCommon::key_state;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::key_just_pressed_frame;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::key_just_released_frame;

inline std::unordered_map<int, bool> GlobalInputCommon::mouse_state;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::mouse_just_pressed_frame;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::mouse_just_released_frame;

// Joypad slots. Unlike key/mouse these are not per-device: the device is
// deliberately ignored so a binding survives a reconnect or a different USB
// port (Godot does not guarantee a stable joypad order across restarts, and
// this is a single-player desktop tool).
inline std::unordered_map<int, bool> GlobalInputCommon::joy_state;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::joy_just_pressed_frame;
inline std::unordered_map<int, uint64_t> GlobalInputCommon::joy_just_released_frame;
inline float GlobalInputCommon::joy_deadzone = GlobalInputCommon::JOY_DEADZONE_DEFAULT;

inline int GlobalInputCommon::wheel_delta = 0;
inline Vector2 GlobalInputCommon::mouse_position;
inline uint64_t GlobalInputCommon::current_frame = 1;
inline std::atomic<bool> GlobalInputCommon::running = false;
inline std::thread GlobalInputCommon::hook_thread;


#endif
