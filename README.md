# Godot-Global-Input
Godot Global input/ Global Shortcuts

### Supported OS
- Windows
- Linux (x11)
- macOS (requires the Input Monitoring permission for global input)

### Wip/ Hopefully would support
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

	elif OS.has_feature("macos"):
		global_input.backend = "macos"
	
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

### Building on macOS

Requires the Xcode command line tools and SCons. The `godot-cpp` submodule must be checked out.

```
cd src
scons platform=macos build=debug
scons platform=macos build=release
```

This produces universal (arm64 + x86_64) libraries in `src/bin/macos/`.
