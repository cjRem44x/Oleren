# Malkur — Oleren Gamedev Library

## Philosophy

Malkur is the built-in gamedev library for Oleren. Its API is inspired by
Raylib — simple, flat, and readable — but the implementation compiles down
to raw backend code (OpenGL, SDL2, Vulkan, etc.) with no managed runtime.

Include via the import block at the top of your file. The alias is how all
Malkur calls are made — `mk` is the conventional short alias.

```rust
@import (
    mk = @malkur,
)
```

---

## Window & Core Loop

The window handle owns the core loop, timing, and draw state.

```rust
fn main() -> !void
{
    win := try mk.init_window(1280, 720, "My Game")
    defer win.close()

    win.set_fps(60)
    win.set_vsync(true)

    while !win.should_close() {
        dt :: win.dt()   # delta time in seconds (f32)

        win.begin_draw()
            win.clear_bg(mk.colors.BLACK)
            # ... drawing calls go here
        win.end_draw()
    }
}
```

### Window API

```rust
mk.init_window(w: i32, h: i32, title: []chr) -> !Window

win.close()
win.set_fps(target: i32)
win.set_vsync(on: bool)
win.should_close() -> bool

win.dt()      -> f32   # delta time (seconds since last frame)
win.fps()     -> i32   # current frames per second
win.width()   -> i32
win.height()  -> i32
win.resize(w: i32, h: i32)
win.set_title(title: []chr)
win.fullscreen(on: bool)

win.begin_draw()
win.end_draw()
win.clear_bg(color: Color)
```

---

## Color

```rust
# Color type
struct Color { r: u8, g: u8, b: u8, a: u8 }

# construct
mk.rgba(r: u8, g: u8, b: u8, a: u8) -> Color
mk.hex(val: u32) -> Color                       # 0xRRGGBBAA

# modify
mk.color_fade(c: Color, alpha: f32) -> Color    # alpha 0.0 - 1.0
mk.color_lerp(a: Color, b: Color, t: f32) -> Color

# built-in palette (accessed as mk.colors.NAME)
mk.colors.BLACK
mk.colors.WHITE
mk.colors.RED
mk.colors.GREEN
mk.colors.BLUE
mk.colors.YELLOW
mk.colors.ORANGE
mk.colors.PURPLE
mk.colors.PINK
mk.colors.GRAY
mk.colors.DARKGRAY
mk.colors.TRANSPARENT
```

---

## Input

### Keyboard

```rust
mk.key_pressed(key: Key)  -> bool   # true on the frame the key was first pressed
mk.key_down(key: Key)     -> bool   # true while the key is held
mk.key_released(key: Key) -> bool   # true on the frame the key was released
mk.key_char()             -> chr    # last character typed this frame (0 if none)

# key constants: mk.keys.NAME
mk.keys.A ... mk.keys.Z
mk.keys.N0 ... mk.keys.N9
mk.keys.F1 ... mk.keys.F12
mk.keys.SPACE
mk.keys.ENTER
mk.keys.ESCAPE
mk.keys.BACKSPACE
mk.keys.TAB
mk.keys.LEFT  mk.keys.RIGHT  mk.keys.UP  mk.keys.DOWN
mk.keys.LSHIFT  mk.keys.RSHIFT
mk.keys.LCTRL   mk.keys.RCTRL
mk.keys.LALT    mk.keys.RALT
```

### Mouse

```rust
mk.mouse_pos()             -> Vec2   # screen position
mk.mouse_delta()           -> Vec2   # movement since last frame
mk.mouse_scroll()          -> f32    # scroll wheel delta
mk.mouse_btn(btn: MBtn)    -> bool   # held
mk.mouse_btn_pressed(btn)  -> bool   # first frame
mk.mouse_btn_released(btn) -> bool   # release frame
mk.mouse_set_pos(pos: Vec2)
mk.mouse_hide()
mk.mouse_show()

# mk.mbtn.LEFT  mk.mbtn.RIGHT  mk.mbtn.MIDDLE
```

### Gamepad

```rust
mk.pad_connected(id: i32)           -> bool
mk.pad_btn(id: i32, btn: PadBtn)    -> bool
mk.pad_btn_pressed(id, btn)         -> bool
mk.pad_axis(id: i32, axis: PadAxis) -> f32   # -1.0 to 1.0

# mk.pad_btn.CROSS  .CIRCLE  .SQUARE  .TRIANGLE
# mk.pad_btn.L1  .R1  .L2  .R2
# mk.pad_btn.DPAD_UP  .DPAD_DOWN  .DPAD_LEFT  .DPAD_RIGHT
# mk.pad_axis.LX  .LY  .RX  .RY
```

---

## 2D Drawing

All 2D draw calls must be inside `win.begin_draw()` / `win.end_draw()`.

```rust
# Shapes
mk.draw_rect(x: f32, y: f32, w: f32, h: f32, color: Color)
mk.draw_rect_rot(x, y, w, h: f32, origin: Vec2, rot: f32, color: Color)
mk.draw_rect_lines(x, y, w, h: f32, thick: f32, color: Color)
mk.draw_circle(cx: f32, cy: f32, r: f32, color: Color)
mk.draw_circle_lines(cx, cy, r: f32, color: Color)
mk.draw_line(x1, y1, x2, y2: f32, thick: f32, color: Color)
mk.draw_triangle(v1, v2, v3: Vec2, color: Color)
mk.draw_poly(center: Vec2, sides: i32, r: f32, rot: f32, color: Color)

# Text
mk.draw_text(text: []chr, x: f32, y: f32, size: f32, color: Color)
mk.draw_text_ex(font: Font, text: []chr, pos: Vec2, size: f32, spacing: f32, color: Color)
mk.measure_text(text: []chr, size: f32) -> Vec2

# Textures / Sprites
mk.draw_texture(tex: Texture, x: f32, y: f32, tint: Color)
mk.draw_texture_ex(tex: Texture, pos: Vec2, rot: f32, scale: f32, tint: Color)
mk.draw_texture_rect(tex: Texture, src: Rect, dst: Rect, origin: Vec2, rot: f32, tint: Color)
```

---

## Camera 2D

```rust
struct Camera2D {
    target:   Vec2,  # world position the camera looks at
    offset:   Vec2,  # screen offset (usually screen center)
    rotation: f32,
    zoom:     f32,
}

win.begin_camera2d(cam: Camera2D)
win.end_camera2d()

# coordinate conversion
mk.screen_to_world2d(pos: Vec2, cam: Camera2D) -> Vec2
mk.world_to_screen2d(pos: Vec2, cam: Camera2D) -> Vec2
```

```rust
# usage
cam := Camera2D{
    .target   = Vec2{.x=player.x, .y=player.y},
    .offset   = Vec2{.x=640.0,    .y=360.0},
    .rotation = 0.0,
    .zoom     = 1.0,
}

win.begin_draw()
    win.clear_bg(mk.colors.BLACK)
    win.begin_camera2d(cam)
        mk.draw_texture(map_tex, 0.0, 0.0, mk.colors.WHITE)
        mk.draw_texture(player_tex, player.x, player.y, mk.colors.WHITE)
    win.end_camera2d()
win.end_draw()
```

---

## Camera 3D

```rust
struct Camera3D {
    pos:    Vec3,
    target: Vec3,
    up:     Vec3,
    fovy:   f32,
    proj:   CamProj,  # mk.proj.PERSPECTIVE | mk.proj.ORTHOGRAPHIC
}

win.begin_camera3d(cam: Camera3D)
win.end_camera3d()

mk.screen_to_world3d(pos: Vec2, cam: Camera3D) -> Ray
```

---

## 3D Drawing

```rust
mk.draw_cube(pos: Vec3, w: f32, h: f32, d: f32, color: Color)
mk.draw_cube_wires(pos: Vec3, w, h, d: f32, color: Color)
mk.draw_sphere(center: Vec3, r: f32, color: Color)
mk.draw_sphere_wires(center: Vec3, r: f32, rings: i32, slices: i32, color: Color)
mk.draw_plane(center: Vec3, size: Vec2, color: Color)
mk.draw_grid(slices: i32, spacing: f32)
mk.draw_ray(ray: Ray, color: Color)

mk.draw_model(model: Model, pos: Vec3, scale: f32, tint: Color)
mk.draw_model_ex(model: Model, pos: Vec3, axis: Vec3, angle: f32, scale: Vec3, tint: Color)
mk.draw_model_wires(model: Model, pos: Vec3, scale: f32, tint: Color)

mk.draw_billboard(cam: Camera3D, tex: Texture, pos: Vec3, size: f32, tint: Color)
```

---

## Textures & Images

```rust
# loading
mk.load_texture(path: []chr) -> !Texture
mk.load_texture_from_image(img: Image) -> Texture
mk.unload_texture(tex: Texture)

mk.load_image(path: []chr) -> !Image
mk.unload_image(img: Image)

# image manipulation (CPU-side, before upload)
mk.image_resize(img: *Image, w: i32, h: i32)
mk.image_flip_v(img: *Image)
mk.image_flip_h(img: *Image)
mk.image_crop(img: *Image, rect: Rect)

# render texture (draw to texture)
mk.load_render_tex(w: i32, h: i32) -> RenderTex
mk.unload_render_tex(rt: RenderTex)
win.begin_render_tex(rt: RenderTex)
win.end_render_tex()
```

---

## Fonts & Text

```rust
mk.load_font(path: []chr) -> !Font
mk.load_font_ex(path: []chr, size: i32, chars: []i32) -> !Font
mk.unload_font(font: Font)

mk.draw_text(text: []chr, x: f32, y: f32, size: f32, color: Color)
mk.draw_text_ex(font: Font, text: []chr, pos: Vec2, size: f32, spacing: f32, color: Color)
mk.measure_text(text: []chr, size: f32)    -> Vec2
mk.measure_text_ex(font: Font, text: []chr, size: f32, spacing: f32) -> Vec2
```

---

## Models & Meshes

```rust
mk.load_model(path: []chr) -> !Model       # .obj .gltf .glb etc.
mk.unload_model(model: Model)

mk.load_mesh_cube(w: f32, h: f32, d: f32)    -> Mesh
mk.load_mesh_sphere(r: f32, rings: i32, slices: i32) -> Mesh
mk.load_mesh_plane(w: f32, d: f32, res_x: i32, res_z: i32) -> Mesh
mk.upload_mesh(mesh: *Mesh)
mk.unload_mesh(mesh: Mesh)
mk.model_from_mesh(mesh: Mesh) -> Model

# materials / shaders on a model
mk.model_set_material(model: *Model, slot: i32, mat: Material)
```

---

## Shaders

```rust
mk.load_shader(vs_path: []chr, fs_path: []chr) -> !Shader
mk.load_shader_src(vs_src: []chr, fs_src: []chr) -> !Shader
mk.unload_shader(shader: Shader)

win.begin_shader(shader: Shader)
win.end_shader()

mk.shader_set_i32(shader: Shader, name: []chr, val: i32)
mk.shader_set_f32(shader: Shader, name: []chr, val: f32)
mk.shader_set_vec2(shader: Shader, name: []chr, val: Vec2)
mk.shader_set_vec3(shader: Shader, name: []chr, val: Vec3)
mk.shader_set_vec4(shader: Shader, name: []chr, val: Vec4)
mk.shader_set_mat4(shader: Shader, name: []chr, val: Mat4)
mk.shader_set_texture(shader: Shader, name: []chr, tex: Texture)
```

---

## Audio

```rust
mk.init_audio()       # call once at startup
mk.close_audio()      # call at shutdown

# Sounds (short, loaded fully into memory)
mk.load_sound(path: []chr) -> !Sound
mk.unload_sound(sound: Sound)
mk.play_sound(sound: Sound)
mk.stop_sound(sound: Sound)
mk.pause_sound(sound: Sound)
mk.resume_sound(sound: Sound)
mk.sound_playing(sound: Sound) -> bool
mk.sound_set_volume(sound: Sound, vol: f32)   # 0.0 - 1.0
mk.sound_set_pitch(sound: Sound, pitch: f32)

# Music (streamed from disk)
mk.load_music(path: []chr) -> !Music
mk.unload_music(music: Music)
mk.play_music(music: Music)
mk.stop_music(music: Music)
mk.pause_music(music: Music)
mk.resume_music(music: Music)
mk.update_music(music: Music)   # must be called every frame while playing
mk.music_playing(music: Music) -> bool
mk.music_set_volume(music: Music, vol: f32)
mk.music_set_pitch(music: Music, pitch: f32)
mk.music_length(music: Music) -> f32    # total duration in seconds
mk.music_pos(music: Music)    -> f32    # current playback position
```

---

## Math Types

These are also available via `std.math` but Malkur re-exports them for
convenience since they are used everywhere in gamedev.

```rust
struct Vec2 { x: f32, y: f32 }
struct Vec3 { x: f32, y: f32, z: f32 }
struct Vec4 { x: f32, y: f32, z: f32, w: f32 }
struct Rect { x: f32, y: f32, w: f32, h: f32 }
struct Ray  { pos: Vec3, dir: Vec3 }
struct Mat4 { m: [16]f32 }   # column-major

# common constructors
mk.vec2(x: f32, y: f32)             -> Vec2
mk.vec3(x: f32, y: f32, z: f32)    -> Vec3
mk.vec2_zero() -> Vec2
mk.vec3_zero() -> Vec3

# Vec2 ops
mk.v2_add(a: Vec2, b: Vec2)  -> Vec2
mk.v2_sub(a: Vec2, b: Vec2)  -> Vec2
mk.v2_scale(v: Vec2, s: f32) -> Vec2
mk.v2_len(v: Vec2)           -> f32
mk.v2_norm(v: Vec2)          -> Vec2
mk.v2_dot(a: Vec2, b: Vec2)  -> f32
mk.v2_dist(a: Vec2, b: Vec2) -> f32
mk.v2_lerp(a: Vec2, b: Vec2, t: f32) -> Vec2

# Vec3 ops (same pattern: mk.v3_*)
# Mat4 ops
mk.mat4_identity()                         -> Mat4
mk.mat4_translate(m: Mat4, v: Vec3)        -> Mat4
mk.mat4_rotate(m: Mat4, axis: Vec3, angle: f32) -> Mat4
mk.mat4_scale(m: Mat4, v: Vec3)            -> Mat4
mk.mat4_mul(a: Mat4, b: Mat4)              -> Mat4
mk.mat4_inverse(m: Mat4)                   -> Mat4
mk.mat4_perspective(fovy: f32, aspect: f32, near: f32, far: f32) -> Mat4
mk.mat4_ortho(l: f32, r: f32, b: f32, t: f32, near: f32, far: f32) -> Mat4
mk.mat4_look_at(eye: Vec3, target: Vec3, up: Vec3) -> Mat4
```

---

## Collision

```rust
# 2D
mk.check_rects(a: Rect, b: Rect)                    -> bool
mk.check_circles(c1: Vec2, r1: f32, c2: Vec2, r2: f32) -> bool
mk.check_point_rect(pt: Vec2, rect: Rect)            -> bool
mk.check_point_circle(pt: Vec2, center: Vec2, r: f32) -> bool
mk.rect_intersect(a: Rect, b: Rect)                  -> Rect  # overlap rect

# 3D
mk.check_ray_sphere(ray: Ray, center: Vec3, r: f32)  -> bool
mk.check_ray_box(ray: Ray, min: Vec3, max: Vec3)     -> bool
mk.check_boxes(min1: Vec3, max1: Vec3, min2: Vec3, max2: Vec3) -> bool
```

---

## Minimal Full Example

```rust
@import (
    mk = @malkur,
)

fn main() -> !void
{
    win := try mk.init_window(800, 600, "Hello Malkur")
    defer win.close()
    win.set_fps(60)

    tex := try mk.load_texture("assets/sprite.png")
    defer mk.unload_texture(tex)

    try mk.init_audio()
    defer mk.close_audio()

    music := try mk.load_music("assets/bg.ogg")
    mk.play_music(music)

    pos := mk.vec2(400.0, 300.0)

    while !win.should_close() {
        dt :: win.dt()

        if mk.key_down(mk.keys.RIGHT) { pos.x += 200.0 * dt }
        if mk.key_down(mk.keys.LEFT)  { pos.x -= 200.0 * dt }

        mk.update_music(music)

        win.begin_draw()
            win.clear_bg(mk.colors.BLACK)
            mk.draw_texture(tex, pos.x, pos.y, mk.colors.WHITE)
            mk.draw_text("Arrow keys to move", 10, 10, 20.0, mk.colors.WHITE)
        win.end_draw()
    }
}
```
