#pragma once
#include "../common.h"

using namespace godot;

class WindowsGlobalInput : public GlobalInputCommon {
public:
    WindowsGlobalInput(){}
    ~WindowsGlobalInput(){ stop();}

    // Start/ Stop Hook

    void start() override {
        if (running)
            return;

        if (!OS::get_singleton()) {
            running = false;
            return;
        }

        if (OS::get_singleton()->has_feature("editor_hint")) {
            running = false;
            return;
        }

        key_state.clear();
        key_just_pressed_frame.clear();
        key_just_released_frame.clear();

        mouse_state.clear();
        mouse_just_pressed_frame.clear();
        mouse_just_released_frame.clear();

        running = true;

        key_maps->get_platform_key_mapping(key_map);

        hook_thread = std::thread(&WindowsGlobalInput::poll_input, this);

        HWND hwnd = (HWND)DisplayServer::get_singleton()->window_get_native_handle(
            DisplayServer::WINDOW_HANDLE);

        if (!init_raw_input(hwnd))
            ERR_PRINT("Failed to initialize raw input");
    }

    void stop() override {
        if (!running)
            return;

        running = false;

        if (raw_thread_id)
            PostThreadMessage(raw_thread_id, WM_QUIT, 0, 0);

        if (hook_thread.joinable())
            hook_thread.join();

        if (raw_thread.joinable())
            raw_thread.join();
    }

    // Polling Data

    void poll_data() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        for (auto &it : key_just_pressed_frame) {
            if (it.second == 0) {
                it.second = current_frame;
            }
        }

        for (auto &it : key_just_released_frame) {
            if (it.second == 0) {
                it.second = current_frame;
            }
        }

        for (auto &it : mouse_just_pressed_frame) {
            if (it.second == 0) {
                it.second = current_frame;
            }
        }

        for (auto &it : mouse_just_released_frame) {
            if (it.second == 0) {
                it.second = current_frame;
            }
        }
    }

    void increment_frame() override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        current_frame++;    
    }

    // Basic Key Input

    bool is_key_pressed(int key) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_state.find(key);
        return it != key_state.end() && it->second;
    }

    bool is_key_just_pressed(int key) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_just_pressed_frame.find(key);
        return it != key_just_pressed_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_key_just_released(int key) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = key_just_released_frame.find(key);
        return it != key_just_released_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    // Mouse Input

    bool is_mouse_pressed(int button) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_state.find(button);
        return it != mouse_state.end() && it->second;
    }

    bool is_mouse_just_pressed(int button) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_just_pressed_frame.find(button);
        return it != mouse_just_pressed_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    bool is_mouse_just_released(int button) override{
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        auto it = mouse_just_released_frame.find(button);
        return it != mouse_just_released_frame.end() &&
            it->second != 0 &&
            (current_frame - it->second) <= JUST_BUFFER_FRAMES;
    }

    Vector2 get_mouse_position() override{
        return mouse_position;
    }

    Vector2 get_hook_mouse_position() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        return mouse_hook_raw;
    }

    Vector2 get_virtual_mouse_position() override {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        return mouse_virtual;
    }

    // Godot InputMap Action Detection

    bool is_action_pressed(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

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
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

        for (int i = 0; i < events.size(); i++) {
            Ref<InputEvent> ev = events[i];
            if (!ev.is_valid()) continue;

            if (auto *key_ev = Object::cast_to<InputEventKey>(ev.ptr())) {
                auto it = key_just_pressed_frame.find(key_ev->get_keycode());
                if (!modifiers_match(key_ev)) continue; 
                if (it != key_just_pressed_frame.end() &&
                    it->second != 0 &&
                    (current_frame - it->second) <= JUST_BUFFER_FRAMES)
                    return true;
            } else if (auto *mouse_ev = Object::cast_to<InputEventMouseButton>(ev.ptr())) {
                auto it = mouse_just_pressed_frame.find(mouse_ev->get_button_index());
                if (!modifiers_match(mouse_ev)) continue; 
                if (it != mouse_just_pressed_frame.end() &&
                    it->second != 0 &&
                    (current_frame - it->second) <= JUST_BUFFER_FRAMES)
                    return true;
            }
        }
        return false;
    }

    bool is_action_just_released(const String &action) override{
        if (!InputMap::get_singleton()) return false;
        const Array events = InputMap::get_singleton()->action_get_events(action);
        std::lock_guard<std::recursive_mutex> lock(state_mutex);

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
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        for (const auto &[key, down] : key_state) {
            if (!down) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            dict["os"] = "Windows";
        }
        return dict;
    }

    Dictionary get_keys_just_pressed_detailed() override{
        Dictionary dict;
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        for (const auto &[key, frame] : key_just_pressed_frame) {
            if ((current_frame - frame) > 1) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            dict["os"] = "Windows";
        }
        return dict;
    }

    Dictionary get_keys_just_released_detailed() override{
        Dictionary dict;
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        for (const auto &[key, frame] : key_just_released_frame) {
            if ((current_frame - frame) > 1) continue;
            String name = "Unknown";
            if (OS::get_singleton() && key >= 0 && key <= KEY_MENU)
                name = OS::get_singleton()->get_keycode_string((Key)key);
            dict[name] = true;
            dict["os"] = "Windows";
        }
        return dict;
    }

    // Modifiers

    bool is_alt_pressed() override{
        #ifdef _WIN32
        return is_key_pressed(VK_MENU) || is_key_pressed(VK_LMENU) || is_key_pressed(VK_RMENU) || is_key_pressed(KEY_ALT);
        #endif
        return false;
    }

    bool is_ctrl_pressed() override{
        #ifdef _WIN32
        return is_key_pressed(VK_CONTROL) || is_key_pressed(VK_LCONTROL) || is_key_pressed(VK_RCONTROL) || is_key_pressed(KEY_CTRL);
        #endif
        return false;

    }

    bool is_shift_pressed() override{
        #ifdef _WIN32
        return is_key_pressed(VK_SHIFT) || is_key_pressed(VK_LSHIFT) || is_key_pressed(VK_RSHIFT) ||  is_key_pressed(KEY_SHIFT);
        #endif
        return false;
    }

    bool is_meta_pressed() override{
        #ifdef _WIN32
        return is_key_pressed(VK_LWIN) || is_key_pressed(VK_RWIN) || is_key_pressed(KEY_META);
        #endif
        return false;
    }

    // Misc

    bool is_captured() {
        RECT r;
        GetClipCursor(&r);
        // If the cursor is clipped to a smaller area than the screen, it is captured/locked
        return (r.right - r.left) < GetSystemMetrics(SM_CXVIRTUALSCREEN);
    }

    bool init_raw_input(HWND hwnd){
        raw_input_hwnd = hwnd;

        if (!hwnd || !IsWindow(hwnd)){
            ERR_PRINT("Invalid Godot window");
            return false;
        }

        SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(this));

        original_wndproc = (WNDPROC)SetWindowLongPtr(
            hwnd,
            GWLP_WNDPROC,
            (LONG_PTR)RawInputWndProc);

        if (!original_wndproc){
            ERR_PRINT("Failed to subclass window");
            return false;
        }

        // Mouse
        RAWINPUTDEVICE rid_mouse{};
        rid_mouse.usUsagePage = 0x01;
        rid_mouse.usUsage = 0x02;
        rid_mouse.dwFlags = RIDEV_INPUTSINK;
        rid_mouse.hwndTarget = hwnd;

        if (!RegisterRawInputDevices(&rid_mouse, 1, sizeof(rid_mouse))) {
            ERR_PRINT("Mouse RID registration failed");
            return false;
        }

        RAWINPUTDEVICE rid_pen{};
        rid_pen.usUsagePage = 0x0D;
        rid_pen.usUsage = 0x00;
        rid_pen.dwFlags = RIDEV_INPUTSINK;
        rid_pen.hwndTarget = hwnd;
        has_last_pen = false;

        RegisterRawInputDevices(&rid_pen, 1, sizeof(rid_pen));

        raw_input_registered = true;
        return true;
    }
    
    void release_raw_input(){
        if (!raw_input_registered)
            return;

        RAWINPUTDEVICE rid{};
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;
        rid.dwFlags = RIDEV_REMOVE;
        rid.hwndTarget = nullptr;

        RegisterRawInputDevices(&rid, 1, sizeof(rid));

        if (raw_input_hwnd && original_wndproc){
            SetWindowLongPtr(
                raw_input_hwnd,
                GWLP_WNDPROC,
                (LONG_PTR)original_wndproc);

            original_wndproc = nullptr;
        }

        raw_input_hwnd = nullptr;
        raw_input_registered = false;
    }

    static LRESULT CALLBACK RawInputWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
        auto *instance = reinterpret_cast<WindowsGlobalInput *>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));

        if (msg == WM_SETFOCUS || msg == WM_ACTIVATE) {
            RAWINPUTDEVICE rid_mouse{};
            rid_mouse.usUsagePage = 0x01;
            rid_mouse.usUsage = 0x02;
            rid_mouse.dwFlags = RIDEV_INPUTSINK;
            rid_mouse.hwndTarget = hwnd;
            RegisterRawInputDevices(&rid_mouse, 1, sizeof(rid_mouse));

            RAWINPUTDEVICE rid_pen{};
            rid_pen.usUsagePage = 0x0D;
            rid_pen.usUsage = 0x00;
            rid_pen.dwFlags = RIDEV_INPUTSINK;
            rid_pen.hwndTarget = hwnd;
            has_last_pen = false;

            RegisterRawInputDevices(&rid_pen, 1, sizeof(rid_pen));
        }

        if (msg == WM_INPUT && instance){

            UINT size = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));

            std::vector<BYTE> buffer(size);

            UINT result = GetRawInputData(
                (HRAWINPUT)lParam,
                RID_INPUT,
                buffer.data(),
                &size,
                sizeof(RAWINPUTHEADER));

            if (result == (UINT)-1){
                ERR_PRINT(vformat("Error: %d", (int)GetLastError()));
                return DefWindowProc(hwnd, msg, wParam, lParam);
            }

            RAWINPUT *raw = reinterpret_cast<RAWINPUT *>(buffer.data());

            std::lock_guard<std::recursive_mutex> lock(instance->state_mutex);
            const RAWMOUSE &m = raw->data.mouse;

            if (raw->header.dwType == RIM_TYPEMOUSE){

                has_last_pen = (m.usFlags == 3);
                LONG x = m.lLastX;
                LONG y = m.lLastY;

                Vector2 current_pos ((float)x, (float)y);
                Vector2 delta = current_pos - instance->last_pen_pos;
                instance->last_pen_pos = current_pos;

                bool captured = instance->is_captured();

                if (captured){
                    instance->mouse_delta.x += x;
                    instance->mouse_delta.y += y;
                }

                else{
                    if (has_last_pen){

                        if (((m.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0) && ((m.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)){
                            Vector2 converted = instance->virtual_absolute_mouse(m);
                            Vector2 delta = mouse_virtual - converted;
                            instance->set_virtual_mouse_position(converted);
                            mouse_hook_raw -= delta;
                            instance->mouse_delta = Vector2();
                        }

                        else if (((m.usFlags & MOUSE_VIRTUAL_DESKTOP) == 0) && ((m.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)){
                            Vector2 converted = instance->absolute_mouse(m);
                            Vector2 delta = mouse_virtual - converted;
                            instance->set_virtual_mouse_position(converted);
                            mouse_hook_raw -= delta;
                            instance->mouse_delta = Vector2();
                        }
                        else {
                            POINT p;
                            GetCursorPos(&p);
                            Vector2 point = Vector2((float)p.x, (float)p.y);
                            Vector2 delta = mouse_virtual - point;
                            instance->set_virtual_mouse_position(point);
                            mouse_hook_raw -= delta;
                            instance->mouse_delta = Vector2();
                        }
                    }
                    else {
                        POINT p;
                        GetCursorPos(&p);
                        Vector2 point = Vector2((float)p.x, (float)p.y);
                        Vector2 delta = mouse_virtual - point;
                        instance->set_virtual_mouse_position(point);
                        mouse_hook_raw -= delta;
                        instance->mouse_delta = Vector2();
                    }
                }
            }
            else if (raw->header.dwType == RIM_TYPEHID) {

                bool captured = instance->is_captured();
                const RAWHID &hid = raw->data.hid;

                const BYTE* data = hid.bRawData;
                UINT dataSize = hid.dwSizeHid;

                auto read_u16 = [&](int offset) -> int
                {
                    if (offset + 1 >= (int)dataSize)
                        return 0;

                    return (int)(data[offset] | (data[offset + 1] << 8));
                };

                int x = read_u16(0);
                int y = read_u16(2);

                std::lock_guard<std::recursive_mutex> lock(instance->state_mutex);

                Vector2 current((float)x, (float)y);

                if (!instance->has_last_pen)
                {
                    instance->last_pen_pos = current;
                    instance->has_last_pen = true;
                }
                else
                {
                    if (captured){

                        Vector2 delta = current - instance->last_pen_pos;
                        instance->mouse_delta.x += delta.x;
                        instance->mouse_delta.y += delta.y;
                        instance->last_pen_pos = current;
                    }
                    else{
                        POINT p;
                        GetCursorPos(&p);
                        Vector2 point = Vector2((float)p.x, (float)p.y);
                        Vector2 delta = mouse_virtual - point;
                        instance->set_virtual_mouse_position(point);
                        mouse_hook_raw -= delta;
                        instance->mouse_delta = Vector2();
                    }

                }
            }
        }

        if (instance && instance->original_wndproc){
            return CallWindowProc(instance->original_wndproc, hwnd, msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    Vector2 virtual_absolute_mouse(const RAWMOUSE &m){
        LONG vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
        LONG vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
        LONG vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        LONG vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        return Vector2((float)m.lLastX * (vw - 1) / 65535.0f + vx,(float)m.lLastY * (vh - 1) / 65535.0f + vy);
    }

    Vector2 absolute_mouse(const RAWMOUSE &m){
        LONG w = GetSystemMetrics(SM_CXSCREEN);
        LONG h = GetSystemMetrics(SM_CYSCREEN);

        float px = ((float)m.lLastX / 65535.0f) * (w - 1);
        float py = ((float)m.lLastY / 65535.0f) * (h - 1);

        return Vector2(px, py);
    }

    void handle_input(const Ref<InputEvent> &event) override {};

    void poll_input() {
        #ifdef _WIN32
            while (running) {
                {
                    if (!OS::get_singleton()) {
                        running = false;
                        return;
                    }
                    
                    std::lock_guard<std::recursive_mutex> lock(state_mutex);

                    for (const auto &[vk, godot_key] : key_map) {
                        SHORT state = GetAsyncKeyState(vk);
                        bool pressed = (state & 0x8000) != 0;
                        bool was_pressed = key_state[godot_key];

                        key_state[godot_key] = pressed;

                        if (pressed && !was_pressed)
                            key_just_pressed_frame[godot_key] = 0;

                        if (!pressed && was_pressed)
                            key_just_released_frame[godot_key] = 0;
                    }

                    POINT p;
                    if (GetCursorPos(&p)) {
                        mouse_position = Vector2(p.x, p.y);
                    }

                    int buttons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON };
                    int godot_buttons[] = {
                        MOUSE_BUTTON_LEFT,
                        MOUSE_BUTTON_RIGHT,
                        MOUSE_BUTTON_MIDDLE
                    };

                    for (int i = 0; i < 3; i++) {
                        SHORT state = GetAsyncKeyState(buttons[i]);
                        bool pressed = (state & 0x8000) != 0;
                        bool was_pressed = mouse_state[godot_buttons[i]];

                        mouse_state[godot_buttons[i]] = pressed;

                        if (pressed && !was_pressed)
                            mouse_just_pressed_frame[godot_buttons[i]] = 0;

                        if (!pressed && was_pressed)
                            mouse_just_released_frame[godot_buttons[i]] = 0;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        #endif
    }

    private:
        std::thread raw_thread;
        HWND raw_input_hwnd = nullptr;
        WNDPROC original_wndproc = nullptr;
        DWORD raw_thread_id = 0;
        bool raw_input_registered = false;
        HMONITOR last_monitor = nullptr;

        static inline std::once_flag raw_class_once;

};
