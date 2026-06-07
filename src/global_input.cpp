#include "global_input.h"

#include <thread>

using namespace godot;

bool GlobalInput::hook_started = false;
uint64_t GlobalInput::current_frame = 0;
bool GlobalInput::use_physics_frames = false;

GlobalInput::GlobalInput() {}

GlobalInput::~GlobalInput() {}

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

    ClassDB::bind_method(D_METHOD("start_hook"), &GlobalInput::start_hook);
    ClassDB::bind_method(D_METHOD("stop_hook"), &GlobalInput::stop_hook);
    
    ClassDB::bind_method(D_METHOD("set_backend", "backend_name"), &GlobalInput::set_backend);
    ClassDB::bind_method(D_METHOD("get_backend"), &GlobalInput::get_backend);

    ClassDB::bind_method(D_METHOD("set_use_physics_frames", "enabled"), &GlobalInput::set_use_physics_frames);
    ClassDB::bind_method(D_METHOD("get_use_physics_frames"), &GlobalInput::get_use_physics_frames);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "backend", PROPERTY_HINT_ENUM, "windows,x11, dummy"),
                 "set_backend", "get_backend");

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_physics_frames"), 
                 "set_use_physics_frames", "get_use_physics_frames");
}

void GlobalInput::set_backend(const String &backend_name) {
    String new_backend = backend_name.to_lower();

    if (selected_backend == new_backend)
        return;

    if (backend.is_valid()) {
        backend->stop();
        backend.unref();
    }

    hook_started = false;
    selected_backend = new_backend;

    check_backend();

    if (backend.is_valid()){
        hook_started = true;
        backend->start();
    } 
    else godot::print_line("Invalid Backend");
}

String GlobalInput::get_backend() { return selected_backend; }

void GlobalInput::start_hook() {
    if (hook_started) return;

    check_backend();

    if (backend.is_valid()){
        hook_started = true;
        backend->start();
    } 
    else godot::print_line("Invalid Backend");
}

void GlobalInput::stop_hook() {
    if (!hook_started) return;
    hook_started = false;

    if (backend.is_valid()) {
        backend->stop();
        backend.unref();
    }
}

void GlobalInput::_process(double delta) {
    if (use_physics_frames) return;
    if (backend.is_valid()) {
        backend->poll_data();
        backend->increment_frame();
        }
}

void GlobalInput::_physics_process(double delta) {
    if (!use_physics_frames) return;
    if (backend.is_valid()) {
        backend->poll_data();
        backend->increment_frame();
        }

}

void GlobalInput::_input(const Ref<InputEvent> &event){
    if (backend.is_valid()) {
        backend->handle_input(event);
    }
}

// --- Input Checks ---
Vector2 GlobalInput::get_mouse_position() { return backend.is_valid() ? backend->get_mouse_position() : Vector2(); }
bool GlobalInput::is_key_pressed(int key) { return backend.is_valid() && backend->is_key_pressed(key); }
bool GlobalInput::is_key_just_pressed(int key) { return backend.is_valid() && backend->is_key_just_pressed(key); }
bool GlobalInput::is_key_just_released(int key) { return backend.is_valid() && backend->is_key_just_released(key); }
bool GlobalInput::is_mouse_pressed(int button) { return backend.is_valid() && backend->is_mouse_pressed(button); }
bool GlobalInput::is_mouse_just_pressed(int button) { return backend.is_valid() && backend->is_mouse_just_pressed(button); }
bool GlobalInput::is_mouse_just_released(int button) { return backend.is_valid() && backend->is_mouse_just_released(button); }

bool GlobalInput::is_action_pressed(const String &action) { return backend.is_valid() && backend->is_action_pressed(action); }
bool GlobalInput::is_action_just_pressed(const String &action) { return backend.is_valid() && backend->is_action_just_pressed(action); }
bool GlobalInput::is_action_just_released(const String &action) { return backend.is_valid() && backend->is_action_just_released(action); }

Dictionary GlobalInput::get_keys_pressed_detailed() { return backend.is_valid() ? backend->get_keys_pressed_detailed() : Dictionary(); }
Dictionary GlobalInput::get_keys_just_pressed_detailed() { return backend.is_valid() ? backend->get_keys_just_pressed_detailed() : Dictionary(); }
Dictionary GlobalInput::get_keys_just_released_detailed() { return backend.is_valid() ? backend->get_keys_just_released_detailed() : Dictionary(); }

bool GlobalInput::is_shift_pressed() { return backend.is_valid() && backend->is_shift_pressed(); }
bool GlobalInput::is_ctrl_pressed() { return backend.is_valid() && backend->is_ctrl_pressed(); }
bool GlobalInput::is_alt_pressed() { return backend.is_valid() && backend->is_alt_pressed(); }
bool GlobalInput::is_meta_pressed() { return backend.is_valid() && backend->is_meta_pressed(); }
