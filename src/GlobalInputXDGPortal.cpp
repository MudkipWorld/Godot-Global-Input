#include "GlobalInputXDGPortal.h"

std::unordered_map<int, bool> GlobalInputXDGPortal::key_state;
std::unordered_map<int, uint64_t> GlobalInputXDGPortal::key_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputXDGPortal::key_just_released_frame;

std::unordered_map<String, int> GlobalInputXDGPortal::action_to_key;
std::unordered_map<int, String> GlobalInputXDGPortal::key_to_action;
std::unordered_map<uint32_t, int> GlobalInputXDGPortal::shortcut_id_to_key;

uint64_t GlobalInputXDGPortal::current_frame = 0;
std::atomic<bool> GlobalInputXDGPortal::running = false;
std::recursive_mutex GlobalInputXDGPortal::state_mutex;

GlobalInputXDGPortal::~GlobalInputXDGPortal() {
    stop();
}

void GlobalInputXDGPortal::increment_frame() {
    current_frame++;
}

bool GlobalInputXDGPortal::is_key_pressed(int key) {
    std::lock_guard lock(state_mutex);
    return key_state[key];
}

bool GlobalInputXDGPortal::is_key_just_pressed(int key) {
    std::lock_guard lock(state_mutex);
    return key_just_pressed_frame[key] == current_frame;
}

bool GlobalInputXDGPortal::is_key_just_released(int key) {
    std::lock_guard lock(state_mutex);
    return key_just_released_frame[key] == current_frame;
}

bool GlobalInputXDGPortal::is_action_pressed(const String &action) {
    return is_key_pressed(action_to_key[action]);
}

bool GlobalInputXDGPortal::is_action_just_pressed(const String &action) {
    return is_key_just_pressed(action_to_key[action]);
}

bool GlobalInputXDGPortal::is_action_just_released(const String &action) {
    return is_key_just_released(action_to_key[action]);
}

Dictionary GlobalInputXDGPortal::get_keys_pressed_detailed() {
    Dictionary d;
    for (auto &it : key_state)
        if (it.second) d[it.first] = true;
    return d;
}

Dictionary GlobalInputXDGPortal::get_keys_just_pressed_detailed() {
    Dictionary d;
    for (auto &it : key_just_pressed_frame)
        if (it.second == current_frame) d[it.first] = true;
    return d;
}

Dictionary GlobalInputXDGPortal::get_keys_just_released_detailed() {
    Dictionary d;
    for (auto &it : key_just_released_frame)
        if (it.second == current_frame) d[it.first] = true;
    return d;
}

void GlobalInputXDGPortal::start() {
#if defined(__linux__)
    if (running.exchange(true))
        return;

    DBusError err;
    dbus_error_init(&err);

    bus = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (!bus || dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        running = false;
        return;
    }

    dbus_bus_add_match(
        bus,
        "type='signal',interface='org.freedesktop.portal.GlobalShortcuts'",
        nullptr
    );
    dbus_connection_flush(bus);

    // Example shortcut
    uint32_t sid = _register_shortcut("toggle_overlay", "<Ctrl><Shift>O");

    int key = PORTAL_KEY_BASE;
    shortcut_id_to_key[sid] = key;
    action_to_key["toggle_overlay"] = key;
    key_to_action[key] = "toggle_overlay";

    dbus_thread = std::thread(&GlobalInputXDGPortal::_dbus_loop, this);
#endif
}

void GlobalInputXDGPortal::stop() {
#if defined(__linux__)
    if (!running.exchange(false))
        return;

    if (dbus_thread.joinable())
        dbus_thread.join();

    if (bus) {
        dbus_connection_unref(bus);
        bus = nullptr;
    }
#endif
}

#if defined(__linux__)

uint32_t GlobalInputXDGPortal::_register_shortcut(
    const String &action,
    const String &accelerator
) {
    static uint32_t next_id = 1;
    uint32_t id = next_id++;

    DBusMessage *msg = dbus_message_new_method_call(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.GlobalShortcuts",
        "RegisterShortcut"
    );

    DBusMessageIter it, dict, entry, var;
    dbus_message_iter_init_append(msg, &it);

    dbus_message_iter_append_basic(&it, DBUS_TYPE_UINT32, &id);

    dbus_message_iter_open_container(&it, DBUS_TYPE_ARRAY, "{sv}", &dict);

    const char *k = "accelerator";
    const char *acc = accelerator.utf8().get_data();

    dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &k);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &var);
    dbus_message_iter_append_basic(&var, DBUS_TYPE_STRING, &acc);
    dbus_message_iter_close_container(&entry, &var);
    dbus_message_iter_close_container(&dict, &entry);

    dbus_message_iter_close_container(&it, &dict);

    dbus_connection_send(bus, msg, nullptr);
    dbus_message_unref(msg);

    return id;
}

void GlobalInputXDGPortal::_dbus_loop() {
    while (running) {
        dbus_connection_read_write(bus, 100);
        DBusMessage *msg = dbus_connection_pop_message(bus);
        if (!msg)
            continue;

        _handle_signal(msg);
        dbus_message_unref(msg);
    }
}

void GlobalInputXDGPortal::_handle_signal(DBusMessage *msg) {
    if (dbus_message_is_signal(msg,
        "org.freedesktop.portal.GlobalShortcuts",
        "Activated")) {

        uint32_t id;
        dbus_message_get_args(msg, nullptr,
            DBUS_TYPE_UINT32, &id,
            DBUS_TYPE_INVALID);

        _on_shortcut_activated(id);
    }

    if (dbus_message_is_signal(msg,
        "org.freedesktop.portal.GlobalShortcuts",
        "Deactivated")) {

        uint32_t id;
        dbus_message_get_args(msg, nullptr,
            DBUS_TYPE_UINT32, &id,
            DBUS_TYPE_INVALID);

        _on_shortcut_deactivated(id);
    }
}

#endif

void GlobalInputXDGPortal::_on_shortcut_activated(uint32_t shortcut_id) {
    std::lock_guard lock(state_mutex);
    int key = shortcut_id_to_key[shortcut_id];
    key_state[key] = true;
    key_just_pressed_frame[key] = current_frame;
}

void GlobalInputXDGPortal::_on_shortcut_deactivated(uint32_t shortcut_id) {
    std::lock_guard lock(state_mutex);
    int key = shortcut_id_to_key[shortcut_id];
    key_state[key] = false;
    key_just_released_frame[key] = current_frame;
}
