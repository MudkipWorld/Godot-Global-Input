#include "GlobalInputMacos.h"

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

using namespace godot;

std::unordered_map<int, bool> GlobalInputMac::key_state;
std::unordered_map<int, uint64_t> GlobalInputMac::key_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputMac::key_just_released_frame;

std::unordered_map<int, bool> GlobalInputMac::mouse_state;
std::unordered_map<int, uint64_t> GlobalInputMac::mouse_just_pressed_frame;
std::unordered_map<int, uint64_t> GlobalInputMac::mouse_just_released_frame;

std::unordered_map<int, bool> GlobalInputMac::last_key_state;
std::unordered_map<int, bool> GlobalInputMac::last_mouse_state;

Vector2 GlobalInputMac::mouse_position;
int GlobalInputMac::wheel_delta = 0;

uint64_t GlobalInputMac::current_frame = 0;
bool GlobalInputMac::running = false;
std::recursive_mutex GlobalInputMac::state_mutex;
std::thread GlobalInputMac::poll_thread;

#ifdef __APPLE__

static CGEventTapLocation tap_loc = kCGHIDEventTap;
static CGEventRef (*original_keyboard_handler)(CGEventTapProxy, CGEventType, CGEventRef, void*) = nullptr;
static CFRunLoopRef tap_runloop = nullptr;

#endif


void GlobalInputMac::start() {
#ifdef __APPLE__
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    if (running) return;

    running = true;
    init_key_map();

    poll_thread = std::thread(&GlobalInputMac::poll_input, this);
#endif
}

void GlobalInputMac::stop() {
#ifdef __APPLE__
    {
        std::lock_guard<std::recursive_mutex> lock(state_mutex);
        running = false;
    }

    if (tap_runloop)
        CFRunLoopStop(tap_runloop);

    if (poll_thread.joinable())
        poll_thread.join();
#endif
}

#ifdef __APPLE__
CGEventRef event_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* refcon) {
    if (type == kCGEventTapDisabledByTimeout)
        CGEventTapEnable((CFMachPortRef)refcon, true);

    std::lock_guard<std::recursive_mutex> lock(GlobalInputMac::state_mutex);

    int key = 0;
    bool down = false;

    switch (type) {
        case kCGEventKeyDown:
        case kCGEventKeyUp: {
            key = GlobalInputMac::translate_mac_keycode(
                (int)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)
            );
            down = (type == kCGEventKeyDown);

            if (key != 0) {
                if (down && !GlobalInputMac::key_state[key])
                    GlobalInputMac::key_just_pressed_frame[key] = GlobalInputMac::current_frame;

                if (!down && GlobalInputMac::key_state[key])
                    GlobalInputMac::key_just_released_frame[key] = GlobalInputMac::current_frame;

                GlobalInputMac::key_state[key] = down;
            }
        } break;

        case kCGEventLeftMouseDown:
        case kCGEventLeftMouseUp:
        case kCGEventRightMouseDown:
        case kCGEventRightMouseUp:
        case kCGEventOtherMouseDown:
        case kCGEventOtherMouseUp: {
            int btn = (int)CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
            int godot_btn = btn == 0 ? MOUSE_BUTTON_LEFT :
                            btn == 1 ? MOUSE_BUTTON_RIGHT :
                            btn == 2 ? MOUSE_BUTTON_MIDDLE :
                            MOUSE_BUTTON_XBUTTON1 + btn - 3;

            down = (type == kCGEventLeftMouseDown ||
                    type == kCGEventRightMouseDown ||
                    type == kCGEventOtherMouseDown);

            if (down && !GlobalInputMac::mouse_state[godot_btn])
                GlobalInputMac::mouse_just_pressed_frame[godot_btn] = GlobalInputMac::current_frame;

            if (!down && GlobalInputMac::mouse_state[godot_btn])
                GlobalInputMac::mouse_just_released_frame[godot_btn] = GlobalInputMac::current_frame;

            GlobalInputMac::mouse_state[godot_btn] = down;
        } break;

        default: break;
    }

    return event;
}
#endif

void GlobalInputMac::poll_input() {
#ifdef __APPLE__
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) |
                       CGEventMaskBit(kCGEventKeyUp) |
                       CGEventMaskBit(kCGEventLeftMouseDown) |
                       CGEventMaskBit(kCGEventLeftMouseUp) |
                       CGEventMaskBit(kCGEventRightMouseDown) |
                       CGEventMaskBit(kCGEventRightMouseUp) |
                       CGEventMaskBit(kCGEventOtherMouseDown) |
                       CGEventMaskBit(kCGEventOtherMouseUp);

    CFMachPortRef tap = CGEventTapCreate(
        kCGHIDEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        event_callback,
        nullptr
    );

    if (!tap) return;

    tap_runloop = CFRunLoopGetCurrent();
    CFRunLoopSourceRef src = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
    CFRunLoopAddSource(tap_runloop, src, kCFRunLoopCommonModes);

    CGEventTapEnable(tap, true);

    while (true) {
        {
            std::lock_guard<std::recursive_mutex> lock(state_mutex);
            if (!running) break;
            current_frame++;
        }

        CGEventRef loc = CGEventCreate(nullptr);
        CGPoint p = CGEventGetLocation(loc);
        CFRelease(loc);

        mouse_position = Vector2(p.x, p.y);

        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.008, true);
    }
#endif
}

#ifdef __APPLE__
void GlobalInputMac::init_key_map() {
    mac_to_godot.clear();

    mac_to_godot[0] = KEY_A;
    mac_to_godot[1] = KEY_S;
    mac_to_godot[2] = KEY_D;
    mac_to_godot[3] = KEY_F;
    mac_to_godot[4] = KEY_H;
    mac_to_godot[5] = KEY_G;

    mac_to_godot[6] = KEY_Z;
    mac_to_godot[7] = KEY_X;
    mac_to_godot[8] = KEY_C;
    mac_to_godot[9] = KEY_V;

    mac_to_godot[11] = KEY_B;
    mac_to_godot[12] = KEY_Q;
    mac_to_godot[13] = KEY_W;
    mac_to_godot[14] = KEY_E;
    mac_to_godot[15] = KEY_R;
    mac_to_godot[16] = KEY_Y;
    mac_to_godot[17] = KEY_T;

    mac_to_godot[18] = KEY_1;
    mac_to_godot[19] = KEY_2;
    mac_to_godot[20] = KEY_3;
    mac_to_godot[21] = KEY_4;
    mac_to_godot[22] = KEY_6;
    mac_to_godot[23] = KEY_5;
    mac_to_godot[24] = KEY_EQUAL;
    mac_to_godot[25] = KEY_9;
    mac_to_godot[26] = KEY_7;
    mac_to_godot[27] = KEY_MINUS;
    mac_to_godot[28] = KEY_8;
    mac_to_godot[29] = KEY_0;
    mac_to_godot[30] = KEY_BRACKETRIGHT;
    mac_to_godot[31] = KEY_O;
    mac_to_godot[32] = KEY_U;
    mac_to_godot[33] = KEY_BRACKETLEFT;
    mac_to_godot[34] = KEY_I;
    mac_to_godot[35] = KEY_P;

    mac_to_godot[36] = KEY_ENTER;
    mac_to_godot[37] = KEY_L;
    mac_to_godot[38] = KEY_J;
    mac_to_godot[39] = KEY_QUOTEDBL;
    mac_to_godot[40] = KEY_K;
    mac_to_godot[41] = KEY_SEMICOLON;
    mac_to_godot[42] = KEY_BACKSLASH;

    mac_to_godot[43] = KEY_COMMA;
    mac_to_godot[44] = KEY_SLASH;
    mac_to_godot[45] = KEY_N;
    mac_to_godot[46] = KEY_M;
    mac_to_godot[47] = KEY_PERIOD;

    mac_to_godot[49] = KEY_SPACE;

    mac_to_godot[50] = KEY_ASCIITILDE;
    mac_to_godot[51] = KEY_BACKSPACE;
    mac_to_godot[52] = KEY_KP_ENTER;
    mac_to_godot[53] = KEY_ESCAPE;

    mac_to_godot[55] = KEY_META;
    mac_to_godot[56] = KEY_SHIFT;
    mac_to_godot[57] = KEY_CAPSLOCK;
    mac_to_godot[58] = KEY_ALT;
    mac_to_godot[59] = KEY_CTRL;
    mac_to_godot[60] = KEY_SHIFT;
    mac_to_godot[61] = KEY_ALT;
    mac_to_godot[62] = KEY_CTRL;

    mac_to_godot[123] = KEY_LEFT;
    mac_to_godot[124] = KEY_RIGHT;
    mac_to_godot[125] = KEY_DOWN;
    mac_to_godot[126] = KEY_UP;
}

int GlobalInputMac::translate_mac_keycode(int code) {
    auto it = mac_to_godot.find(code);
    return it != mac_to_godot.end() ? it->second : 0;
}
#endif
