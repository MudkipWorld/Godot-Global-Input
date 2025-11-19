#include "GlobalInput.h"
#include "GlobalInputUiohook.h"
#ifdef _WIN32
#include "GlobalInputWindows.h"
#endif
#include <thread>

using namespace godot;

// Static members
bool GlobalInput::hook_started = false;
uint64_t GlobalInput::current_frame = 0;
bool GlobalInput::use_physics_frames = false;

GlobalInput::GlobalInput() {
    if (OS::get_singleton()) {
        start_hook(); // synchronous, safe
    }
}


GlobalInput::~GlobalInput() {
    stop_hook();
}

void GlobalInput::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_mouse_position"), &GlobalInput::get_mouse_position);
    ClassDB::bind_method(D_METHOD("is_key_pressed", "key"), &GlobalInput::is_key_pressed);
    ClassDB::bind_method(D_METHOD("is_key_just_pressed", "key"), &GlobalInput::is_key_just_pressed);
    ClassDB::bind_method(D_METHOD("is_key_just_released", "key"), &GlobalInput::is_key_just_released);
    ClassDB::bind_method(D_METHOD("is_mouse_pressed", "button"), &GlobalInput::is_mouse_pressed);
    ClassDB::bind_method(D_METHOD("is_mouse_just_pressed", "button"), &GlobalInput::is_mouse_just_pressed);
    ClassDB::bind_method(D_METHOD("is_mouse_just_released", "button"), &GlobalInput::is_mouse_just_released);

    ClassDB::bind_method(D_METHOD("is_action_pressed", "action"), &GlobalInput::is_action_pressed);
    ClassDB::bind_method(D_METHOD("is_action_just_pressed", "action"), &GlobalInput::is_action_just_pressed);
    ClassDB::bind_method(D_METHOD("is_action_just_released", "action"), &GlobalInput::is_action_just_released);

    ClassDB::bind_method(D_METHOD("get_keys_pressed_detailed"), &GlobalInput::get_keys_pressed_detailed);
    ClassDB::bind_method(D_METHOD("get_keys_just_pressed_detailed"), &GlobalInput::get_keys_just_pressed_detailed);
    ClassDB::bind_method(D_METHOD("get_keys_just_released_detailed"), &GlobalInput::get_keys_just_released_detailed);

    ClassDB::bind_method(D_METHOD("is_shift_pressed"), &GlobalInput::is_shift_pressed);
    ClassDB::bind_method(D_METHOD("is_ctrl_pressed"), &GlobalInput::is_ctrl_pressed);
    ClassDB::bind_method(D_METHOD("is_alt_pressed"), &GlobalInput::is_alt_pressed);
    ClassDB::bind_method(D_METHOD("is_meta_pressed"), &GlobalInput::is_meta_pressed);


    ClassDB::bind_method(D_METHOD("set_backend", "backend_name"), &GlobalInput::set_backend);
    ClassDB::bind_method(D_METHOD("get_backend"), &GlobalInput::get_backend);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "backend", PROPERTY_HINT_ENUM, "default,uiohook,windows"),
                 "set_backend", "get_backend");
}

void GlobalInput::set_backend(const String &backend_name) {
    String new_backend = backend_name.to_lower();

    // If it's already the active backend, nothing to do
    if (selected_backend == new_backend)
        return;

    // Stop current backend
    if (backend) {
        backend->stop();
        backend.reset();
    }

    // Assign new backend type
    selected_backend = new_backend;

#ifdef _WIN32
    if (selected_backend == "windows") {
        backend = std::make_unique<GlobalInputWindows>();
        active_backend = BACKEND_WINDOWS;
    } else
#endif
    if (selected_backend == "uiohook") {
        backend = std::make_unique<GlobalInputUiohook>();
        active_backend = BACKEND_UIOHOOK;
    } else {
        // Default fallback to uiohook if unknown
        backend = std::make_unique<GlobalInputUiohook>();
        active_backend = BACKEND_UIOHOOK;
    }

    // Start the new backend
    if (backend)
        backend->start();
}


String GlobalInput::get_backend() { return selected_backend; }
String GlobalInput::get_active_backend_name() const { return selected_backend; }

void GlobalInput::start_hook() {
    if (hook_started) return;

#ifdef _WIN32
    if (selected_backend == "windows") {
        backend = std::make_unique<GlobalInputWindows>();
        active_backend = BACKEND_WINDOWS;
    } else
#endif
    {
        backend = std::make_unique<GlobalInputUiohook>();
        active_backend = BACKEND_UIOHOOK;
    }

    hook_started = true;

    backend->start(); // <-- backend may spawn its own thread
}


void GlobalInput::stop_hook() {
    if (!hook_started) return;
    if (backend) {
        backend->stop();
        backend.reset();
    }
    hook_started = false;
}

void GlobalInput::_process(double delta) {
    if (!use_physics_frames) current_frame++;
    if (backend) backend->increment_frame();
}

void GlobalInput::_physics_process(double delta) {
    if (use_physics_frames) current_frame++;
    if (backend) backend->increment_frame();
}

// --- Input queries ---
Vector2 GlobalInput::get_mouse_position() { return backend ? backend->get_mouse_position() : Vector2(); }
bool GlobalInput::is_key_pressed(int key) { return backend && backend->is_key_pressed(key); }
bool GlobalInput::is_key_just_pressed(int key) { return backend && backend->is_key_just_pressed(key); }
bool GlobalInput::is_key_just_released(int key) { return backend && backend->is_key_just_released(key); }
bool GlobalInput::is_mouse_pressed(int button) { return backend && backend->is_mouse_pressed(button); }
bool GlobalInput::is_mouse_just_pressed(int button) { return backend && backend->is_mouse_just_pressed(button); }
bool GlobalInput::is_mouse_just_released(int button) { return backend && backend->is_mouse_just_released(button); }

bool GlobalInput::is_action_pressed(const String &action) { return backend && backend->is_action_pressed(action); }
bool GlobalInput::is_action_just_pressed(const String &action) { return backend && backend->is_action_just_pressed(action); }
bool GlobalInput::is_action_just_released(const String &action) { return backend && backend->is_action_just_released(action); }

Dictionary GlobalInput::get_keys_pressed_detailed() { return backend ? backend->get_keys_pressed_detailed() : Dictionary(); }
Dictionary GlobalInput::get_keys_just_pressed_detailed() { return backend ? backend->get_keys_just_pressed_detailed() : Dictionary(); }
Dictionary GlobalInput::get_keys_just_released_detailed() { return backend ? backend->get_keys_just_released_detailed() : Dictionary(); }

bool GlobalInput::is_shift_pressed() { return backend && backend->is_shift_pressed();}
bool GlobalInput::is_ctrl_pressed() { return backend && backend->is_ctrl_pressed();}
bool GlobalInput::is_alt_pressed() { return backend && backend->is_alt_pressed();}
bool GlobalInput::is_meta_pressed() { return backend && backend->is_meta_pressed();}

