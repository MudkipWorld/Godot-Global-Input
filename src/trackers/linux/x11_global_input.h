#pragma once

#include "../common.h"

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <string.h>
#include <poll.h>

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define IS_SET(bit, bits) ((bits[bit/BITS_PER_LONG] & (1UL << (bit % BITS_PER_LONG))) != 0)
#endif

#include <algorithm>
#include <cctype>

using namespace godot;

class LinuxGlobalInput : public GlobalInputCommon {
private:

    #ifdef __linux__
    int keyboard_fd = -1;
    int mice_fd = -1;

    int open_keyboard_device() {
        const char *input_dir = "/dev/input/";
        DIR *dir = opendir(input_dir);
        if (!dir) {
            return -1;
        }

        struct dirent *entry;

        int best_fd = -1;
        int best_score = -1;

        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "event", 5) != 0) {
                continue;
            }

            char path[512];
            snprintf(path, sizeof(path), "%s%s", input_dir, entry->d_name);

            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                continue;
            }

            unsigned long evbit[NBITS(EV_CNT)] = {};
            unsigned long keybit[256] = {};
            unsigned long relbit[NBITS(REL_CNT)] = {};

            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) {
                close(fd);
                continue;
            }

            if (!IS_SET(EV_KEY, evbit)) {
                close(fd);
                continue;
            }

            bool has_relative_motion = false;

            if (IS_SET(EV_REL, evbit)) {
                if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit) >= 0) {
                    if (IS_SET(REL_X, relbit) || IS_SET(REL_Y, relbit)) {
                        has_relative_motion = true;
                    }
                }
            }

            if (has_relative_motion) {
                close(fd);
                continue;
            }

            char name[256] = {};
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);

            std::string name_str(name);

            std::transform(
                name_str.begin(),
                name_str.end(),
                name_str.begin(),
                [](unsigned char c) { return std::tolower(c); });

            int score = 0;

            if (name_str.find("keyboard") != std::string::npos) {
                score += 1000;
            }

            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) {
                int score = 0;

                for (size_t word = 0; word < sizeof(keybit) / sizeof(keybit[0]); ++word) {
                    score += __builtin_popcountl(keybit[word]);
                }
            }

            if (score > best_score) {
                if (best_fd >= 0) {
                    close(best_fd);
                }

                best_fd = fd;
                best_score = score;
            } else {
                close(fd);
            }
        }

        closedir(dir);

        if (best_fd >= 0) {
            print_line(
                "Global Input: Opened keyboard device with score: " +
                String::num(best_score));
        }

        return best_fd;
    }
    #endif

public:
    LinuxGlobalInput(){}
    
    ~LinuxGlobalInput(){
        stop();
    }

    void start() override {
        if (running) return;
        if (!OS::get_singleton()) { running = false; return; }
        if (OS::get_singleton()->has_feature("editor_hint")){ running = false; return; }

        key_state.clear();
        key_just_pressed_frame.clear();
        key_just_released_frame.clear();
        mouse_state.clear();
        mouse_just_pressed_frame.clear();
        mouse_just_released_frame.clear();

        key_maps->get_platform_key_mapping(key_map);

        #ifdef __linux__
        keyboard_fd = open_keyboard_device();
        if (keyboard_fd < 0) godot::print_line("Failed to open keyboard device.");

        mice_fd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
        if (mice_fd < 0) godot::print_line("Failed to open mouse device /dev/input/mice.");

        if (keyboard_fd >= 0 || mice_fd >= 0) {
            running = true;
            hook_thread = std::thread(&LinuxGlobalInput::poll_input, this);
        }
        else{
            godot::print_line("Something went wrong. UGHHHH WORK I BEG YOU.");
        }
        #endif
    }

    void stop() override{
        if (!running) return;
        running = false;
        if (hook_thread.joinable())
            hook_thread.join();

        #ifdef __linux__
        if (keyboard_fd >= 0) {
            close(keyboard_fd);
            keyboard_fd = -1;
        }
        if (mice_fd >= 0) {
            close(mice_fd);
            mice_fd = -1;
        }
        #endif
    }

    void poll_data() override {
        for (auto &it : key_just_pressed_frame) { if (it.second == 0) it.second = current_frame; }
        for (auto &it : key_just_released_frame) { if (it.second == 0) it.second = current_frame; }
        for (auto &it : mouse_just_pressed_frame) { if (it.second == 0) it.second = current_frame; }
        for (auto &it : mouse_just_released_frame) { if (it.second == 0) it.second = current_frame; }
    }

    void increment_frame() override{
        current_frame++;    
    }

    bool is_key_pressed(int key) override{
        auto it = key_state.find(key);
        return it != key_state.end() && it->second;
    }

    bool is_key_just_pressed(int key) override{
        auto it = key_just_pressed_frame.find(key);
        return it != key_just_pressed_frame.end() && it->second != 0 && (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_key_just_released(int key) override{
        auto it = key_just_released_frame.find(key);
        return it != key_just_released_frame.end() && it->second != 0 && (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_mouse_pressed(int button) override{
        auto it = mouse_state.find(button);
        return it != mouse_state.end() && it->second;
    }
    
    bool is_mouse_just_pressed(int button) override{
        auto it = mouse_just_pressed_frame.find(button);
        return it != mouse_just_pressed_frame.end() && it->second != 0 && (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_mouse_just_released(int button) override{
        auto it = mouse_just_released_frame.find(button);
        return it != mouse_just_released_frame.end() && it->second != 0 && (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    Vector2 get_mouse_position() override{
        return mouse_position;
    }

    bool is_action_pressed(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> ev = events[i];
            if (!ev.is_valid()) continue;
            if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
                if (!modifiers_match(key_ev)) continue; 
                if (key_state[key_ev->get_keycode()]) return true;
            } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
                if (!modifiers_match(mouse_ev)) continue; 
                if (mouse_state[mouse_ev->get_button_index()]) return true;
            }
        }
        return false;
    }

    bool is_action_just_pressed(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> ev = events[i];
            if (!ev.is_valid()) continue;
            if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
                auto it = key_just_pressed_frame.find(key_ev->get_keycode());
                if (!modifiers_match(key_ev)) continue; 
                if (it != key_just_pressed_frame.end() && it->second != 0 && (current_frame - it->second) <= JUST_BUFFER_FRAMES) return true;
            } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
                auto it = mouse_just_pressed_frame.find(mouse_ev->get_button_index());
                if (!modifiers_match(mouse_ev)) continue; 
                if (it != mouse_just_pressed_frame.end() && it->second != 0 && (current_frame - it->second) <= JUST_BUFFER_FRAMES) return true;
            }
        }
        return false;
    }

    bool is_action_just_released(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> ev = events[i];
            if (!ev.is_valid()) continue;
            if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
                auto it = key_just_released_frame.find(key_ev->get_keycode());
                if (!modifiers_match(key_ev)) continue; 
                if (it != key_just_released_frame.end() && (current_frame - it->second) <= 1) return true;
            } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
                auto it = mouse_just_released_frame.find(mouse_ev->get_button_index());
                if (!modifiers_match(mouse_ev)) continue; 
                if (it != mouse_just_released_frame.end() && (current_frame - it->second) <= 1) return true;
            }
        }
        return false;
    }
    
    // Debug Returns

    Dictionary get_keys_pressed_detailed() override{
        Dictionary dict;
        for (const auto &[key, down] : key_state) {
            if (!down) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            dict["os"] = "Linux or BSD";
        }
        return dict;
    }

    Dictionary get_keys_just_pressed_detailed() override{
        Dictionary dict;
        for (const auto &[key, frame] : key_just_pressed_frame) {
            if ((current_frame - frame) > 1) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            dict["os"] = "Linux or BSD";
        }
        return dict;
    }

    Dictionary get_keys_just_released_detailed() override{
        Dictionary dict;
        for (const auto &[key, frame] : key_just_released_frame) {
            if ((current_frame - frame) > 1) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            dict["os"] = "Linux or BSD";
        }
        return dict;
    }

    // Modifiers
    bool is_alt_pressed() override{
        #ifdef __linux__
        return is_key_pressed(PH_KEY_LEFTALT) || is_key_pressed(PH_KEY_RIGHTALT);
        #endif
        return false;
    }

    bool is_ctrl_pressed() override{
        #ifdef __linux__
        return is_key_pressed(PH_KEY_LEFTCTRL) || is_key_pressed(PH_KEY_RIGHTCTRL);
        #endif
        return false;
    }

    bool is_shift_pressed() override{
        #ifdef __linux__
        return is_key_pressed(PH_KEY_LEFTSHIFT) || is_key_pressed(PH_KEY_RIGHTSHIFT);
        #endif
        return false;
    }

    bool is_meta_pressed() override{
        #ifdef __linux__
        return is_key_pressed(PH_KEY_LEFTMETA) || is_key_pressed(PH_KEY_RIGHTMETA);
        #endif
        return false;
    }
    
    void handle_input(const Ref<InputEvent> &event) override {}

    void poll_input() {
    #ifdef __linux__
        struct pollfd fds[2];

        while (running) {
            fds[0].fd = keyboard_fd;
            fds[0].events = POLLIN;
            fds[0].revents = 0;

            fds[1].fd = mice_fd;
            fds[1].events = POLLIN;
            fds[1].revents = 0;

            int ret = poll(fds, 2, 5);

            if (ret > 0) {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);
                if (keyboard_fd >= 0 && (fds[0].revents & POLLIN)) {
                    struct input_event ev;

                    while (read(keyboard_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                        if (ev.type != EV_KEY)
                            continue;

                        auto it = key_map.find((int)ev.code);
                        if (it == key_map.end())
                            continue;

                        int godot_key = it->second;
                        bool pressed = (ev.value != 0);

                        bool was_pressed = key_state[godot_key];
                        key_state[godot_key] = pressed;

                        if (pressed && !was_pressed)
                            key_just_pressed_frame[godot_key] = 0;

                        if (!pressed && was_pressed)
                            key_just_released_frame[godot_key] = 0;
                    }
                }
                
                if (mice_fd >= 0 && (fds[1].revents & POLLIN)) {
                    unsigned char data[3];

                    if (read(mice_fd, data, sizeof(data)) == sizeof(data)) {
                        bool left_pressed   = (data[0] & 0x1) != 0;
                        bool right_pressed  = (data[0] & 0x2) != 0;
                        bool middle_pressed = (data[0] & 0x4) != 0;

                        update_mouse_state(MOUSE_BUTTON_LEFT, left_pressed);
                        update_mouse_state(MOUSE_BUTTON_RIGHT, right_pressed);
                        update_mouse_state(MOUSE_BUTTON_MIDDLE, middle_pressed);

                        mouse_position.x += (signed char)data[1];
                        mouse_position.y += (signed char)data[2];
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    #endif
    }

private:
    void update_mouse_state(int button, bool is_pressed = true) {
        bool was_pressed = mouse_state[button];
        mouse_state[button] = is_pressed;
        if (is_pressed && !was_pressed) mouse_just_pressed_frame[button] = 0;
        if (!is_pressed && was_pressed) mouse_just_released_frame[button] = 0;
    }
};