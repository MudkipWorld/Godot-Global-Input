#pragma once
#include "GlobalInputCommon.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/record.h>
#include <X11/XKBlib.h>
#include <fcntl.h>
#include <unistd.h>
#endif

using namespace godot;

class GlobalInputX11 : public IGlobalInputBackend {
public:


    ~GlobalInputX11(){
        stop();
    }
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
    static std::unordered_map<int, bool> key_state;
    static std::unordered_map<int, uint64_t> key_just_pressed_frame;
    static std::unordered_map<int, uint64_t> key_just_released_frame;

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
    #ifdef __linux__
    int translate_x11_to_godot(KeySym ks);
    #endif
    void init_key_map();
    std::unordered_map<int, int> x11_to_godot;

#ifdef __linux__
    Display *display = nullptr;
    ::Window root_window = 0;
    int keyboard_fd = -1;
    int mice_fd = -1;
#endif
};
