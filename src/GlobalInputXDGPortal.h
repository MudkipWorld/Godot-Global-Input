#pragma once
#include "GlobalInputCommon.h"

#include <unordered_map>
#include <mutex>
#include <atomic>

#ifdef __linux__
#include <dbus/dbus.h>
#endif


using namespace godot;

class GlobalInputXDGPortal : public IGlobalInputBackend {
public:
    ~GlobalInputXDGPortal() {
        stop();
    }

    void start() override {}
    void stop() override{}
    void increment_frame() override{}

    Vector2 get_mouse_position() override { return Vector2(); }

    bool is_mouse_pressed(int) override { return false; }
    bool is_mouse_just_pressed(int) override { return false; }
    bool is_mouse_just_released(int) override { return false; }

    bool is_alt_pressed() override { return false; }
    bool is_ctrl_pressed() override { return false; }
    bool is_shift_pressed() override { return false; }
    bool is_meta_pressed() override { return false; }
    bool modifiers_match(InputEvent*) { return false; }

    bool is_key_pressed(int key) override { return false; }
    bool is_key_just_pressed(int key) override { return false; }
    bool is_key_just_released(int key) override { return false; }

    Dictionary get_keys_pressed_detailed() override { return {}; }
    Dictionary get_keys_just_pressed_detailed() override { return {}; }
    Dictionary get_keys_just_released_detailed() override { return {}; }

    bool is_action_pressed(const String &action) override { return false; }
    bool is_action_just_pressed(const String &action) override { return false; }
    bool is_action_just_released(const String &action) override { return false; }

    void poll_data() override {}

private:
    static constexpr int PORTAL_KEY_BASE = 0x700000;

    static std::unordered_map<int, bool> key_state;
    static std::unordered_map<int, uint64_t> key_just_pressed_frame;
    static std::unordered_map<int, uint64_t> key_just_released_frame;
    static std::unordered_map<int, bool> last_key_state;

    static std::unordered_map<String, int> action_to_key;
    static std::unordered_map<int, String> key_to_action;
    static std::unordered_map<uint32_t, int> shortcut_id_to_key;

    static uint64_t current_frame;
    static std::atomic<bool> running;
    static std::recursive_mutex state_mutex;

#ifdef __linux__
    static DBusConnection *bus;
    static std::thread dbus_thread;

    static void _dbus_loop(){}
    static void _handle_signal(DBusMessage *msg){}
#endif

    static void _on_shortcut_activated(uint32_t shortcut_id);
    static void _on_shortcut_deactivated(uint32_t shortcut_id);
};