# Malkur — Oleren Gamedev Library

## Philosophy

Malkur is the built-in gamedev library for Oleren. Its API is inspired by
Raylib — simple, flat, and readable — but the implementation compiles down
to raw backend code (OpenGL, SDL2, Vulkan, etc.) with no managed runtime.

Include via the import block at the top of your file (or a top-level bind:
`mk :: @std.malkur`). The alias is how all Malkur calls are made — `mk` is
the conventional short alias. Importing malkur pulls in the SDL2 backend;
the compiler resolves the SDL2 flags per platform (pkg-config, with
Linux/macOS/Windows-MinGW fallbacks). If SDL2 isn't installed, the build
stops with install instructions for your OS — `olrn deps` checks up front.

```rust
@import (
    mk = @std.malkur,
)
```

## Status (v0.4)

The API is flat — every call goes through the alias (`mk.draw_rect(...)`),
Raylib-style; window/renderer/input state lives in the backend.

**Implemented (SDL2 backend):** window & core loop, keyboard, mouse, gamepad
(up to 4 controllers), 2D shapes, `draw_rect_rot`, textures (BMP + PNG + JPG
via SDL_image, src/dst subrect), camera 2D (world↔screen transforms, all draw
calls go through it), embedded 8×8 bitmap font (`draw_text`/`measure_text`),
TTF/OTF fonts (`load_font`/`draw_text_ex`/`measure_text_ex` via SDL_ttf),
audio (sounds + streaming music via SDL_mixer), colors (including `hex()`),
Vec2 math, 2D collision.

**Planned:** 3D, images/render textures, models, shaders, Vec3/Mat4 math, 3D collision.
Sections below marked *Planned* are design spec, not yet implemented.

---

## Window & Core Loop

```rust
fn main() -> !void
{
    try mk.init_window(1280, 720, "My Game")
    defer mk.close_window()

    mk.set_fps(60)
    mk.set_vsync(true)

    while !mk.should_close() {
        dt :: mk.dt()   # delta time in seconds (f32)

        mk.begin_draw()
            mk.clear_bg(mk.BLACK)
            # ... drawing calls go here
        mk.end_draw()
    }
}
```

### Window API

```rust
mk.init_window(w: i32, h: i32, title: str) -> !void

mk.close_window()
mk.set_fps(target: i32)
mk.set_vsync(on: bool)
mk.should_close() -> bool   # polls events + refreshes input state; call once per frame

mk.dt()      -> f32   # delta time (seconds since last frame)
mk.fps()     -> i32   # current frames per second
mk.width()   -> i32
mk.height()  -> i32
mk.set_title(title: str)
mk.fullscreen(on: bool)

mk.begin_draw()
mk.end_draw()
mk.clear_bg(color: Color)
```

---

## Color

```rust
# Color type
struct Color { r: u8, g: u8, b: u8, a: u8 }

# construct
mk.rgba(r: i32, g: i32, b: i32, a: i32) -> Color
mk.hex(val: u32) -> Color                       # 0xRRGGBBAA packed u32

# modify
mk.color_fade(c: Color, alpha: f32) -> Color    # alpha 0.0 - 1.0
mk.color_lerp(a: Color, b: Color, t: f32) -> Color

# built-in palette (accessed as mk.NAME)
mk.BLACK
mk.WHITE
mk.RED
mk.GREEN
mk.BLUE
mk.YELLOW
mk.ORANGE
mk.PURPLE
mk.PINK
mk.GRAY
mk.DARKGRAY
mk.TRANSPARENT
```

---

## Input

### Keyboard

Key constants are the `keys` enum (i32 SDL scancodes) — pass `mk.keys.NAME`.

```rust
mk.key_pressed(key: i32)  -> bool   # true on the frame the key was first pressed
mk.key_down(key: i32)     -> bool   # true while the key is held
mk.key_released(key: i32) -> bool   # true on the frame the key was released
mk.key_char()             -> str    # last character typed this frame ("" if none)

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
mk.mouse_btn(btn: i32)     -> bool   # held — pass mk.mbtn.NAME
mk.mouse_btn_pressed(btn)  -> bool   # first frame
mk.mouse_btn_released(btn) -> bool   # release frame
mk.mouse_set_pos(pos: Vec2)
mk.mouse_hide()
mk.mouse_show()

# mk.mbtn.LEFT  mk.mbtn.RIGHT  mk.mbtn.MIDDLE
```

### Gamepad

Supports up to 4 SDL GameController-compatible pads (XInput, PS, etc.).
Hotplug is handled automatically inside `mk.should_close()`.

```rust
mk.pad_connected(id: i32)           -> bool
mk.pad_btn(id: i32, btn: i32)       -> bool   # held
mk.pad_btn_pressed(id: i32, btn: i32) -> bool  # first frame
mk.pad_axis(id: i32, axis: i32)     -> f32    # -1.0 to 1.0

# Button constants (mk.pad_btn.NAME)
mk.pad_btn.A  .B  .X  .Y
mk.pad_btn.BACK  .GUIDE  .START
mk.pad_btn.LS  .RS              # thumbstick click
mk.pad_btn.LB  .RB              # shoulder buttons
mk.pad_btn.UP  .DOWN  .LEFT  .RIGHT   # d-pad

# Axis constants (mk.pad_axis.NAME)
mk.pad_axis.LEFTX  .LEFTY
mk.pad_axis.RIGHTX  .RIGHTY
mk.pad_axis.LT  .RT             # triggers, 0.0 - 1.0
```

---

## 2D Drawing

All 2D draw calls must be inside `mk.begin_draw()` / `mk.end_draw()`.

```rust
# Shapes
mk.draw_rect(x: f32, y: f32, w: f32, h: f32, color: Color)
mk.draw_rect_lines(x, y, w, h: f32, thick: f32, color: Color)
mk.draw_rect_rot(x, y, w, h: f32, origin: Vec2, rot: f32, color: Color)   # rotated rect
mk.draw_circle(cx: f32, cy: f32, r: f32, color: Color)
mk.draw_circle_lines(cx, cy, r: f32, color: Color)
mk.draw_line(x1, y1, x2, y2: f32, thick: f32, color: Color)
mk.draw_triangle(v1, v2, v3: Vec2, color: Color)
mk.draw_poly(center: Vec2, sides: i32, r: f32, rot: f32, color: Color)

# Text (embedded 8x8 bitmap font; no external dep)
mk.draw_text(text: str, x: f32, y: f32, size: f32, color: Color)
mk.measure_text(text: str, size: f32) -> Vec2   # returns Vec2{w, h}

# Textures / Sprites
mk.draw_texture(tex: Texture, x: f32, y: f32, tint: Color)
mk.draw_texture_ex(tex: Texture, pos: Vec2, rot: f32, scale: f32, tint: Color)
mk.draw_texture_rect(tex: Texture, src: Rect, dst: Rect, tint: Color)  # sub-rect blit
```

---

## Camera 2D

All draw calls between `begin_camera2d` / `end_camera2d` are transformed
through the camera (pan, zoom). The camera is reset to identity by `end_camera2d`.

```rust
struct Camera2D {
    target:   Vec2,  # world position the camera looks at
    offset:   Vec2,  # screen offset (usually screen center)
    rotation: f32,
    zoom:     f32,
}

mk.camera2d(target: Vec2, offset: Vec2, zoom: f32) -> Camera2D   # helper ctor
mk.begin_camera2d(cam: Camera2D)
mk.end_camera2d()

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

mk.begin_draw()
    mk.clear_bg(mk.BLACK)
    mk.begin_camera2d(cam)
        mk.draw_texture(map_tex, 0.0, 0.0, mk.WHITE)
        mk.draw_texture(player_tex, player.x, player.y, mk.WHITE)
    mk.end_camera2d()
mk.end_draw()
```

---

## Camera 3D

> **Planned** — not yet implemented.

```rust
struct Camera3D {
    pos:    Vec3,
    target: Vec3,
    up:     Vec3,
    fovy:   f32,
    proj:   CamProj,  # mk.proj.PERSPECTIVE | mk.proj.ORTHOGRAPHIC
}

mk.begin_camera3d(cam: Camera3D)
mk.end_camera3d()

mk.screen_to_world3d(pos: Vec2, cam: Camera3D) -> Ray
```

---

## 3D Drawing

> **Planned** — not yet implemented.

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
# loading — BMP, PNG, JPG (SDL_image backend)
mk.load_texture(path: str) -> !Texture
mk.unload_texture(tex: Texture)

# planned
mk.load_texture_from_image(img: Image) -> Texture
mk.load_image(path: str) -> !Image
mk.unload_image(img: Image)

# image manipulation (planned; CPU-side, before upload)
mk.image_resize(img: *Image, w: i32, h: i32)
mk.image_flip_v(img: *Image)
mk.image_flip_h(img: *Image)
mk.image_crop(img: *Image, rect: Rect)

# render texture (planned; draw to texture)
mk.load_render_tex(w: i32, h: i32) -> RenderTex
mk.unload_render_tex(rt: RenderTex)
mk.begin_render_tex(rt: RenderTex)
mk.end_render_tex()
```

---

## Fonts & Text

An embedded 8×8 pixel-bitmap font (95 printable ASCII characters, no external
dep) is available via `draw_text`. TTF/OTF fonts are available via SDL_ttf.

```rust
# Embedded 8x8 font
mk.draw_text(text: str, x: f32, y: f32, size: f32, color: Color)
   # size=8 → 1px per bit; size=16 → 2×2 per bit; etc.
mk.measure_text(text: str, size: f32) -> Vec2   # Vec2{len*size, size}

# TTF font support (SDL_ttf)
mk.load_font(path: str, size: i32) -> MalkurError!Font
   # loads a TTF/OTF file at the given pixel size; defer mk.unload_font(font)
mk.unload_font(font: Font)
mk.draw_text_ex(font: Font, text: str, pos: Vec2, size: f32, spacing: f32, color: Color)
   # size scales the output relative to the loaded font height
   # spacing: extra pixels between glyphs
mk.measure_text_ex(font: Font, text: str, size: f32, spacing: f32) -> Vec2
```

System dep: `sudo pacman -S sdl2_ttf` / `sudo apt install libsdl2-ttf-dev`

---

## Models & Meshes

> **Planned** — not yet implemented.

```rust
mk.load_model(path: str) -> !Model       # .obj .gltf .glb etc.
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

> **Planned** — not yet implemented.

```rust
mk.load_shader(vs_path: str, fs_path: str) -> !Shader
mk.load_shader_src(vs_src: str, fs_src: str) -> !Shader
mk.unload_shader(shader: Shader)

mk.begin_shader(shader: Shader)
mk.end_shader()

mk.shader_set_i32(shader: Shader, name: str, val: i32)
mk.shader_set_f32(shader: Shader, name: str, val: f32)
mk.shader_set_vec2(shader: Shader, name: str, val: Vec2)
mk.shader_set_vec3(shader: Shader, name: str, val: Vec3)
mk.shader_set_vec4(shader: Shader, name: str, val: Vec4)
mk.shader_set_mat4(shader: Shader, name: str, val: Mat4)
mk.shader_set_texture(shader: Shader, name: str, tex: Texture)
```

---

## Audio

SDL_mixer backend. Requires `sdl2_mixer` (Arch) / `libsdl2-mixer-dev` (apt).

```rust
# Init / shutdown — call init_audio once after init_window
mk.init_audio() -> !void
mk.close_audio()

# Sounds (short clips, loaded fully into memory — WAV, OGG, MP3)
type Sound = i64
mk.load_sound(path: str) -> !Sound
mk.unload_sound(s: Sound)
mk.play_sound(s: Sound)
mk.stop_sounds()                       # stop all active sound channels
mk.sound_set_volume(s: Sound, vol: f32)  # 0.0 - 1.0

# Music (streaming — MP3, OGG, WAV, FLAC, ...)
type Music = i64
mk.load_music(path: str) -> !Music
mk.unload_music(m: Music)
mk.play_music(m: Music, loops: i32)    # -1 = loop forever
mk.stop_music()
mk.pause_music()
mk.resume_music()
mk.music_playing() -> bool
mk.music_paused()  -> bool
mk.music_set_volume(vol: f32)          # 0.0 - 1.0 (global music volume)
```

**Example:**
```rust
try mk.init_audio()
defer mk.close_audio()

shoot  := try mk.load_sound("assets/shoot.wav")
bgm    := try mk.load_music("assets/bgm.ogg")
defer mk.unload_sound(shoot)
defer mk.unload_music(bgm)

mk.play_music(bgm, -1)   # loop forever

# in game loop:
if mk.key_pressed(mk.keys.SPACE) { mk.play_sound(shoot) }
```

---

## Math Types

`Vec2`, `Rect`, and `Color` come from the backend and are usable directly
(struct literals work: `Vec2{.x=1.0, .y=2.0}`).

```rust
struct Color { r: u8, g: u8, b: u8, a: u8 }
struct Vec2  { x: f32, y: f32 }
struct Rect  { x: f32, y: f32, w: f32, h: f32 }

# planned
struct Vec3 { x: f32, y: f32, z: f32 }
struct Vec4 { x: f32, y: f32, z: f32, w: f32 }
struct Ray  { pos: Vec3, dir: Vec3 }
struct Mat4 { m: [16]f32 }   # column-major

# constructors
mk.vec2(x: f32, y: f32)                -> Vec2
mk.vec2_zero()                         -> Vec2
mk.rect(x: f32, y: f32, w: f32, h: f32) -> Rect
mk.vec3(x: f32, y: f32, z: f32)        -> Vec3   # planned
mk.vec3_zero()                         -> Vec3   # planned

# Vec2 ops
mk.v2_add(a: Vec2, b: Vec2)  -> Vec2
mk.v2_sub(a: Vec2, b: Vec2)  -> Vec2
mk.v2_scale(v: Vec2, s: f32) -> Vec2
mk.v2_len(v: Vec2)           -> f32
mk.v2_norm(v: Vec2)          -> Vec2
mk.v2_dot(a: Vec2, b: Vec2)  -> f32
mk.v2_dist(a: Vec2, b: Vec2) -> f32
mk.v2_lerp(a: Vec2, b: Vec2, t: f32) -> Vec2

# Vec3 ops (planned; same pattern: mk.v3_*)
# Mat4 ops (planned)
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

# 3D (planned)
mk.check_ray_sphere(ray: Ray, center: Vec3, r: f32)  -> bool
mk.check_ray_box(ray: Ray, min: Vec3, max: Vec3)     -> bool
mk.check_boxes(min1: Vec3, max1: Vec3, min2: Vec3, max2: Vec3) -> bool
```

---

## Minimal Full Example

```rust
@import (
    mk = @std.malkur,
)

fn main() -> !void
{
    try mk.init_window(800, 600, "Hello Malkur")
    defer mk.close_window()
    mk.set_fps(60)

    tex := try mk.load_texture("assets/sprite.bmp")
    defer mk.unload_texture(tex)

    pos := mk.vec2(400.0, 300.0)

    while !mk.should_close() {
        dt :: mk.dt()

        if mk.key_down(mk.keys.RIGHT) { pos.x += 200.0 * dt }
        if mk.key_down(mk.keys.LEFT)  { pos.x -= 200.0 * dt }

        mk.begin_draw()
            mk.clear_bg(mk.BLACK)
            mk.draw_texture(tex, pos.x, pos.y, mk.WHITE)
            mk.draw_circle(pos.x, pos.y - 40.0, 12.0, mk.RED)
        mk.end_draw()
    }
}
```
