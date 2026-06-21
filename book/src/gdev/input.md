# Input

## Keyboard

```olrn
if mk.key_down(mk.keys.RIGHT)    { # held }
if mk.key_pressed(mk.keys.SPACE) { # just pressed this frame }
if mk.key_released(mk.keys.ESC)  { @exit(0) }

c :str = mk.key_char()    # last typed character as str, "" if none
```

### Common key constants

```olrn
mk.keys.UP    mk.keys.DOWN   mk.keys.LEFT   mk.keys.RIGHT
mk.keys.W     mk.keys.A      mk.keys.S      mk.keys.D
mk.keys.SPACE mk.keys.ENTER  mk.keys.ESC    mk.keys.BACKSPACE
mk.keys.LSHIFT mk.keys.LCTRL mk.keys.TAB
mk.keys.F1 .. mk.keys.F12
```

## Mouse

```olrn
pos   :Vec2 = mk.mouse_pos()
delta :Vec2 = mk.mouse_delta()
scroll :f32 = mk.mouse_scroll()

if mk.mouse_btn(0)         { # left held }
if mk.mouse_btn_pressed(1) { # right just clicked }
if mk.mouse_btn_released(2){ # middle just released }

mk.mouse_set_pos(Vec2{.x=400.0, .y=300.0})
mk.mouse_hide()
mk.mouse_show()
```

Mouse button indices: `0` = left, `1` = right, `2` = middle.

## Gamepad

Supports up to 4 slots. Gamepads hot-plug automatically.

```olrn
n :i32 = mk.pad_count()       # number of connected gamepads

if mk.pad_connected(0) {
    name :str = mk.pad_name(0)

    if mk.pad_btn_press(0, mk.pad_btn.A)    { # held }
    if mk.pad_btn_hit(0, mk.pad_btn.START)  { # just pressed }
    if mk.pad_btn_release(0, mk.pad_btn.B)  { # just released }

    lx :f32 = mk.pad_axis(0, 0)                  # left stick X, -1..1
    ly :f32 = mk.pad_axis_dz(0, 1, 0.15)         # left stick Y with deadzone
    rt :f32 = mk.pad_axis(0, 5)                   # right trigger, 0..1

    mk.pad_rumble(0, 0.5, 0.5, 200)   # low, high intensity, duration ms
}
```

### Button constants

```olrn
mk.pad_btn.A  mk.pad_btn.B  mk.pad_btn.X  mk.pad_btn.Y
mk.pad_btn.LB mk.pad_btn.RB
mk.pad_btn.START mk.pad_btn.SELECT
mk.pad_btn.DPAD_UP mk.pad_btn.DPAD_DOWN
mk.pad_btn.DPAD_LEFT mk.pad_btn.DPAD_RIGHT
```
