# Godot-Global-Input
Godot Global input/ Global Shortcuts

### Supported OS
- Windows
- Linux (x11)

### Wip/ Hopefully would support
- Macos
- Linux (Wayland)

### Usage Example

```
var global_input : GlobalInput = GlobalInput.new()

func _ready() -> void:
	add_child(global_input)
	
	# Dummy Backend if you need to only check inputs in Window.
	# global_input.backend = "dummy"
	
	if OS.has_feature("windows"):
		global_input.backend = "windows"
		
	elif OS.has_feature("linuxbsd"):
		global_input.backend = "x11"
	
    # Don't forget to start the hook for it to work.
	global_input.start_hook()

func _process(_delta: float) -> void:
	# Check the keyboard keys pressed.
	print(global_input.get_keys_pressed_detailed())
	
	# Check Specific Key.
	print(global_input.is_key_just_pressed(KEY_0))
	
	# Check InputMap Action.
	print(global_input.is_action_just_pressed('ui_up'))

func _exit_tree() -> void:
	# Don't forget to stop the hook when needed/ for safety.
	global_input.stop_hook()
```

### Event Based Checks

`is_input_pressed(event, inclusive)`, `is_input_just_pressed(event, inclusive)` and `is_input_just_released(event, inclusive)` take an `InputEventKey` or `InputEventMouseButton` and check it against the keys held globally, so hotkeys bound by the user can be matched directly. Modifiers are compared per modifier, so combined hotkeys such as Ctrl + Shift + A are only reported while every modifier of the event is held. With `inclusive = true`, a single held key or modifier of the event is enough to report a match.

`is_action_pressed(action, inclusive)`, `is_action_just_pressed(action, inclusive)` and `is_action_just_released(action, inclusive)` take the same `inclusive` flag. It defaults to `false`, which keeps the strict behaviour: every modifier of the action's event has to match the modifiers currently held.

InputMap action events are cached for 60 frames, so checking actions every frame does not rebuild the event array each time. Call `refresh_action_cache()` after changing an action's events at runtime to apply the change immediately.

The per-frame snapshot behind the `just_pressed` / `just_released` checks is advanced at most once per engine frame. It is driven from `_process()`, and also lazily from the checks themselves, so attaching a script that overrides `_process()` on the `GlobalInput` node does not stop the snapshot from updating.

### Known Issues
- While unsure if this is because I am using a VMBox, but Linux (x11) seems to sometimes get hung up. Needs testing.
- To use it on x11, you must add the software to Input Group. Don't forget to run these.

```
getent group input
sudo usermod -aG input $USER

// to check if you are in the group
ls -l /dev/input/event*
```

### References
- Horobol's Demo (Which was used for PNGTuber+) [godot-background-inputs-demo](https://github.com/Horobol/godot-background-inputs-demo)
- CVRain's Extension (for the KeyMap idea) [rainer-global-input](https://github.com/CvRain/rainer-global-input)