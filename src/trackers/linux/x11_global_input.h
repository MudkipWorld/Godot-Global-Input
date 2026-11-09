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
#include <vector>
#include <string>
#include <algorithm>
#include <errno.h>

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define IS_SET(bit, bits) ((bits[bit/BITS_PER_LONG] & (1UL << (bit % BITS_PER_LONG))) != 0)
#endif

#include <cctype>

using namespace godot;

class LinuxGlobalInput : public GlobalInputCommon {
private:

    #ifdef __linux__

    struct MouseDevice {
        int fd = -1;
        bool is_absolute = false;
        int max_x = 0;
        int max_y = 0;
        float last_x = 0.0f;
        float last_y = 0.0f;

        float tracked_x = 0.0f;
        float tracked_y = 0.0f;
    };

    std::vector<MouseDevice> mouse_devices;

    int keyboard_fd = -1;




    int get_godot_mouse_button(int code) {
        switch (code) {
            case 0x110: return MOUSE_BUTTON_LEFT;    // BTN_LEFT
            case 0x111: return MOUSE_BUTTON_RIGHT;   // BTN_RIGHT
            case 0x112: return MOUSE_BUTTON_MIDDLE;  // BTN_MIDDLE
            case 0x113: return MOUSE_BUTTON_XBUTTON1; // BTN_SIDE (Back)
            case 0x114: return MOUSE_BUTTON_XBUTTON2; // BTN_EXTRA (Forward)
            case 0x116: return MOUSE_BUTTON_XBUTTON1; // BTN_BACK
            case 0x115: return MOUSE_BUTTON_XBUTTON2; // BTN_FORWARD
            default: return -1;
        }
    }

    int open_keyboard_device(const String &custom_keyboard_name) {
        const char *input_dir = "/dev/input/";
        DIR *dir = opendir(input_dir);
        if (!dir) {
            return -1;
        }

        struct dirent *entry;
        int best_fd = -1;
        int best_score = -1;

        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "event", 5) != 0) continue;

            char path[512];
            snprintf(path, sizeof(path), "%s%s", input_dir, entry->d_name);

            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                char errbuf[1024];
                snprintf(errbuf, sizeof(errbuf), "Global Input: open(%s) failed: %s", path, strerror(errno));
                godot::print_line(errbuf);
                continue;
            }

            unsigned long evbit[NBITS(EV_CNT)] = {};
            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) {
                close(fd);
                continue;
            }

            if (!IS_SET(EV_KEY, evbit)) {
                close(fd);
                continue;
            }

            char name[256] = {};
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            std::string name_str(name);
            std::transform(name_str.begin(), name_str.end(), name_str.begin(),
                [](unsigned char c){ return std::tolower(c); });

            int score = 0;
            if (name_str.find("keyboard") != std::string::npos) {
                score += 1000;
            }

            if (!custom_keyboard_name.is_empty()) {

                std::string target_name = custom_keyboard_name.utf8().get_data();
                std::transform(target_name.begin(), target_name.end(), target_name.begin(),
                [](unsigned char c){ return std::tolower(c); });

                if (name_str.find(target_name) != std::string::npos) {
                     score += 999999;
                }
            }

            unsigned long keybit[256] = {};
            if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) >= 0) {
                int key_count = 0;
                for (size_t word = 0; word < sizeof(keybit) / sizeof(keybit[0]); ++word) {
                    key_count += __builtin_popcountl(keybit[word]);
                }
                score += key_count;
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
;

        return best_fd;
    }

    int open_mouse_device() {
        const char *input_dir = "/dev/input/";
        DIR *dir = opendir(input_dir);
        if (!dir) return 0;

        struct dirent *entry;

        while ((entry = readdir(dir)) != nullptr) {
            if (strncmp(entry->d_name, "event", 5) != 0) continue;

            char path[512];
            snprintf(path, sizeof(path), "%s%s", input_dir, entry->d_name);
            int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                char errbuf[1024];
                snprintf(errbuf, sizeof(errbuf), "Global Input: open(%s) failed: %s", path, strerror(errno));
                godot::print_line(errbuf);
                continue;
            }
            unsigned long evbit[NBITS(EV_CNT)] = {};
            if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) { close(fd); continue; }

            bool has_rel = IS_SET(EV_REL, evbit);
            bool has_abs = IS_SET(EV_ABS, evbit);
            bool has_keys = IS_SET(EV_KEY, evbit);

            if (!has_keys || (!has_rel && !has_abs)) {
                close(fd);
                continue;
            }

            char name[256] = {};
            ioctl(fd, EVIOCGNAME(sizeof(name)), name);

            MouseDevice dev;
            dev.fd = fd;
            dev.is_absolute = has_abs;

            if (dev.is_absolute) {
                struct input_absinfo abs_info = {};
                if (ioctl(fd, EVIOCGABS(ABS_X), &abs_info) >= 0) {
                    dev.max_x = abs_info.maximum;
                    dev.last_x = abs_info.value;
                }
                if (ioctl(fd, EVIOCGABS(ABS_Y), &abs_info) >= 0) {
                    dev.max_y = abs_info.maximum;
                    dev.last_y = abs_info.value;
                }
            }

            mouse_devices.push_back(dev);
        }
        closedir(dir);
        return mouse_devices.size();
    }
    #endif

public:
    LinuxGlobalInput(){}
    ~LinuxGlobalInput(){ stop(); }

    void start(const String &custom_keyboard_name = "") override {
        if (running) return;
        if (!OS::get_singleton()) { running = false; return; }
        if (OS::get_singleton()->has_feature("editor_hint")){ running = false; return; }

        key_state.clear();
        key_just_pressed_frame.clear();
        key_just_released_frame.clear();
        mouse_state.clear();
        mouse_just_pressed_frame.clear();
        mouse_just_released_frame.clear();

        mouse_delta = Vector2(0, 0);

        key_maps->get_platform_key_mapping(key_map);

        calculate_virtual_screen_bounds();

        #ifdef __linux__

            if (virtual_screen_rect.size.x > 0 && virtual_screen_rect.size.y > 0) {
                Vector2 center = virtual_screen_rect.position + (virtual_screen_rect.size / 2.0f);
                for (auto &dev : mouse_devices) {
                    Vector2 p = Vector2(DisplayServer::get_singleton()->mouse_get_position());
                    mouse_virtual.x = p.x;
                    mouse_virtual.y = p.y;
                }
            }

            keyboard_fd = open_keyboard_device(custom_keyboard_name);
            if (keyboard_fd < 0) godot::print_line("Global Input: Failed to open keyboard device.");

            open_mouse_device();
            if (mouse_devices.empty()) godot::print_line("Global Input: Failed to open mouse device.");

            if (keyboard_fd >= 0 || !mouse_devices.empty()) {
                running = true;
                hook_thread = std::thread(&LinuxGlobalInput::poll_input, this);
            }
            else{
                godot::print_line("Global Input: Failed to start input thread.");
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

        for (auto &dev : mouse_devices) {
            if (dev.fd >= 0) close(dev.fd);
        }
        mouse_devices.clear();
        #endif
    }


 // bool set_keyboard_device(const String &name) override {

 //     #ifdef __linux__
 //     customKeyboardName = name.utf8();
 //
 //     return true;
 //     #endif
 //     return false;

 // }



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

    Vector2 get_hook_mouse_position() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        return mouse_hook_raw;
    }

    Vector2 get_virtual_mouse_position()  override {
        return mouse_virtual;
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

    void set_virtual_mouse_position(Vector2 p) override {
        mouse_hook_raw -= mouse_virtual - p;
        mouse_virtual = p;
    }

    void handle_input(const Ref<InputEvent> &event) override {}

    void calculate_virtual_screen_bounds() {
        if (!DisplayServer::get_singleton()) return;

        int screen_count = DisplayServer::get_singleton()->get_screen_count();
        if (screen_count <= 0) return;

        int min_x = 1000000000;
        int min_y = 1000000000;
        int max_x = -1000000000;
        int max_y = -1000000000;

        for (int i = 0; i < screen_count; i++) {
            Vector2 pos = DisplayServer::get_singleton()->screen_get_position(i);
            Vector2 size = DisplayServer::get_singleton()->screen_get_size(i);

            if (pos.x < min_x) min_x = pos.x;
            if (pos.y < min_y) min_y = pos.y;
            if (pos.x + size.x > max_x) max_x = pos.x + size.x;
            if (pos.y + size.y > max_y) max_y = pos.y + size.y;
        }

        virtual_screen_rect.position = Vector2(min_x, min_y);
        virtual_screen_rect.size = Vector2(max_x - min_x, max_y - min_y);
    }

    void poll_input() {
    #ifdef __linux__
        int timeout = 5;

        while (running) {
            std::vector<struct pollfd> fds;

            if (keyboard_fd >= 0) {
                fds.push_back({keyboard_fd, POLLIN, 0});
            }

            for (const auto &dev : mouse_devices) {
                fds.push_back({dev.fd, POLLIN, 0});
            }

            if (fds.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
                continue;
            }

            int ret = poll(fds.data(), fds.size(), timeout);

            if (ret > 0) {
                std::lock_guard<std::recursive_mutex> lock(state_mutex);

                if (keyboard_fd >= 0) {
                    for (auto &p : fds) {
                        if (p.fd == keyboard_fd && (p.revents & POLLIN)) {
                            struct input_event ev;
                            while (read(keyboard_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                                if (ev.type != EV_KEY) continue;
                                auto it = key_map.find((int)ev.code);
                                if (it == key_map.end()) continue;
                                int godot_key = it->second;
                                bool pressed = (ev.value != 0);
                                bool was_pressed = key_state[godot_key];
                                key_state[godot_key] = pressed;
                                if (pressed && !was_pressed) key_just_pressed_frame[godot_key] = 0;
                                if (!pressed && was_pressed) key_just_released_frame[godot_key] = 0;
                            }
                        }
                    }
                }

                for (auto &p : fds) {
                    if (p.revents & POLLIN) {
                        MouseDevice *dev = nullptr;
                        for (auto &d : mouse_devices) {
                            if (d.fd == p.fd) {
                                dev = &d;
                                break;
                            }
                        }

                        if (dev) {
                            struct input_event ev;
                            while (read(dev->fd, &ev, sizeof(ev)) == sizeof(ev)) {

                                if (ev.type == EV_REL) {

                                    if (ev.code == REL_X) mouse_delta.x += (float)ev.value;
                                    else if (ev.code == REL_Y) mouse_delta.y += (float)ev.value;
                                    else if (ev.code == REL_WHEEL) { wheel_delta = ev.value; }

                                    /*

                                    if (ev.code == REL_X) {
                                        dev->tracked_x += (float)ev.value;
                                        dev->tracked_x = CLAMP(dev->tracked_x, virtual_screen_rect.position.x, virtual_screen_rect.position.x + virtual_screen_rect.size.x);
                                    }
                                    else if (ev.code == REL_Y) {
                                        dev->tracked_y += (float)ev.value;
                                        dev->tracked_y = CLAMP(dev->tracked_y, virtual_screen_rect.position.y, virtual_screen_rect.position.y + virtual_screen_rect.size.y);
                                    }
                                    mouse_virtual.x = dev->tracked_x;
                                    mouse_virtual.y = dev->tracked_y;*/
                                }

                                else if (ev.type == EV_ABS) {
                                    if (virtual_screen_rect.size.x > 0 && virtual_screen_rect.size.y > 0) {

                                        float target_x = 0.0f;
                                        float target_y = 0.0f;

                                        if (ev.code == ABS_X) {
                                            target_x = ((float)ev.value / (float)dev->max_x) * virtual_screen_rect.size.x + virtual_screen_rect.position.x;
                                            float delta = mouse_virtual.x - target_x;
                                            mouse_hook_raw.x -= delta;
                                            mouse_virtual.x = target_x;
                                            dev->tracked_x = target_x;
                                            mouse_delta = Vector2();
                                            dev->last_x = target_x;
                                        }
                                        else if (ev.code == ABS_Y) {
                                            target_y = ((float)ev.value / (float)dev->max_y) * virtual_screen_rect.size.y + virtual_screen_rect.position.y;
                                            float delta = mouse_virtual.y - target_y;
                                            mouse_hook_raw.y -= delta;

                                            mouse_virtual.y = target_y;
                                            dev->tracked_y = target_y;
                                            mouse_delta = Vector2();
                                            dev->last_y = target_y;
                                        }

                                    }
                                }
                                else if (ev.type == EV_KEY) {
                                    int godot_btn = get_godot_mouse_button(ev.code);
                                    if (godot_btn >= 0) {
                                        bool pressed = (ev.value != 0);
                                        update_mouse_state(godot_btn, pressed);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    #endif
    }

private:
    bool use_absolute_mode = false;
    int abs_max_x = 0;
    int abs_max_y = 0;
    int last_abs_x = 0;
    int last_abs_y = 0;
    Rect2 virtual_screen_rect;

    void update_mouse_state(int button, bool is_pressed = true) {
        bool was_pressed = mouse_state[button];
        mouse_state[button] = is_pressed;
        if (is_pressed && !was_pressed) mouse_just_pressed_frame[button] = 0;
        if (!is_pressed && was_pressed) mouse_just_released_frame[button] = 0;
    }
};
