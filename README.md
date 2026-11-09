# Godot-Global-Input
Godot Global input/ Global Shortcuts

### Supported OS
- Windows
- Linux (x11)
- macOS (requires the Input Monitoring permission for global input)

### Partially works (Check example)
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
		# "x11" backend works with both x11 and Wayland
		global_input.backend = "x11"

	elif OS.has_feature("macos"):
		global_input.backend = "macos"
	
    # Don't forget to start the hook for it to work. Windows and Macos don't need a keyboard name passed, check the next comment.
	# if the start_hook() doesn't allow for strings to be passed, this means it is on an older build, maybe updated in the future or you can rebuild it yourself.
	global_input.start_hook('')
	
	# If you have issues on linux for starting hook, make sure to pass keyboard name
	global_input.start_hook('keyboard_device_name_here')

func _process(_delta: float) -> void:
	# Check the keyboard keys pressed.
	print(global_input.get_keys_pressed_detailed())
	
	# Check Specific Key.
	print(global_input.is_key_just_pressed(KEY_0))
	
	# Check InputMap Action.
	print(global_input.is_action_just_pressed('ui_up'))

	# Get Global Mouse Position (Tracking sensitivity, screen size and clamp mouse in screen)
	var mouse : Vector2 = global_input.get_virtual_mouse_position()
	global_input.update_virtual_mouse(1.0, Vector2.ZERO, DisplayServer.screen_get_size(0), true)

func _exit_tree() -> void:
	# Don't forget to stop the hook when needed/ for safety.
	global_input.stop_hook()
```

### Example Test Project (Label shows keys pressed and Follow Mouse similar to PNGTube-Remix)

```
extends Node2D

var global_input : GlobalInput = GlobalInput.new()

# Don't forget to assign these from the inspector
@export var label : Label = null
@export var sprite : Sprite2D = null

@export var range_x : Vector2 = Vector2(-100, 100)
@export var range_y : Vector2 = Vector2(-100, 100)

func _ready() -> void:
	if OS.has_feature('windows'):
		global_input.backend = 'windows'
		global_input.start_hook('')
	elif OS.has_feature('linuxbsd') or OS.has_feature('linux'):
		global_input.backend = 'x11'
		global_input.start_hook('keyboard_device_name_here')
	elif OS.has_feature("macos"):
		global_input.backend = "macos"
		global_input.start_hook('')

func _exit_tree() -> void:
	global_input.stop_hook()

static func get_follow_range(val_x : Vector2, val_y : Vector2, dir : Vector2, m_dist : Vector2 = Vector2.ZERO, swap : bool = false) -> Vector2:
	var dist = dir
	
	var min_x = val_x.x
	var max_x = val_x.y

	var min_y = val_y.x
	var max_y = val_y.y
	
	if m_dist != Vector2.ZERO:
		dist = m_dist
	else:
		dist = Vector2(abs(min_x) + max_x, abs(min_y) + max_y)*0.5
		
	var clamped_x_min = 0.0
	var clamped_y_min  = 0.0
	
	var final_dir = Vector2(dir.y, dir.x) if swap else dir
	
	if min_x != 0 && max_x == 0:
		if final_dir.x <= 0:
			clamped_x_min = final_dir.x * min(dist.x, abs(min_x))
			
	elif min_x == 0 && max_x != 0:
		if final_dir.x >= 0:
			clamped_x_min = final_dir.x * min(dist.x, max_x)
	else:
		var clm_x = clamp(final_dir.x*dist.x, -abs(final_dir.x*max_x), abs(final_dir.x*min_x))
		clamped_x_min = clm_x
		
	
	if min_y != 0 && max_y == 0:
		if final_dir.y >= 0:
			clamped_y_min = final_dir.y * clamp(dist.y, 0, abs(min_y))
	
	elif min_y == 0 && max_y != 0:
		if final_dir.y <= 0:
			clamped_y_min = final_dir.y * clamp(dist.y, 0, max_y)
		
	else:
		var clm_y = clamp(final_dir.y*dist.y, -abs(final_dir.y*max_y), abs(final_dir.y*min_y))
		clamped_y_min = clm_y
	
	return Vector2(clamped_x_min, clamped_y_min)

func _physics_process(_delta: float) -> void:
	var key_details : Dictionary = global_input.get_keys_pressed_detailed()
	if is_instance_valid(label):
		label.text = str(key_details)

	if is_instance_valid(sprite):
		var display_size : Vector2 = Vector2(DisplayServer.screen_get_size(0))/2
		var mouse : Vector2 = global_input.get_virtual_mouse_position()
		global_input.update_virtual_mouse(1.0, Vector2.ZERO, DisplayServer.screen_get_size(0), true)
		
		var m : Vector2 = Vector2(mouse - display_size)
		var dir = m.normalized()
		var dist = m.length()
		
		var new_pos : Vector2 = get_follow_range(range_x, range_y, dir, Vector2(dist, dist))
		sprite.position = sprite.position.lerp(new_pos, 0.15)
```


### Known Issues
- While unsure if this is because I am using a VMBox, but Linux (x11) seems to sometimes get hung up. Needs testing.
- To use it on x11, you must add the software to Input Group. Don't forget to run these.
- Macos doesn't have a Virtual/ Raw mouse implementation yet

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
