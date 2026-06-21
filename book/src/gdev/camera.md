# Camera

Gdev provides a 2D camera with pan, zoom, and world↔screen coordinate conversion.

## Creating a camera

```olrn
target :Vec2   = mk.vec2(0.0, 0.0)    # world point to center on
offset :Vec2   = mk.vec2(400.0, 300.0) # screen point that maps to target
zoom   :f32    = 1.0

cam :Camera2D = mk.camera2d(target, offset, zoom)
```

## Using the camera in the render loop

Wrap draw calls in `begin_camera2d` / `end_camera2d`. Everything inside is drawn in world space:

```olrn
mk.begin_draw()
    mk.clear_bg(mk.BLACK)

    mk.begin_camera2d(cam)
        # draw world objects here in world coordinates
        mk.draw_rect(player.x, player.y, 32.0, 32.0, mk.GREEN)
        mk.draw_texture(tilemap, 0.0, 0.0, mk.WHITE)
    mk.end_camera2d()

    # HUD drawn after end_camera2d — in screen coordinates
    mk.draw_text("HP: 100", 10.0, 10.0, 16.0, mk.WHITE)
mk.end_draw()
```

## Following a player

Update the camera target each frame to follow the player:

```olrn
cam = mk.camera2d(
    mk.vec2(player.x, player.y),
    mk.vec2(@f32(mk.width()) / 2.0, @f32(mk.height()) / 2.0),
    cam_zoom,
)
```

## Zooming

```olrn
scroll :f32 = mk.mouse_scroll()
cam_zoom += scroll * 0.1
if cam_zoom < 0.1 { cam_zoom = 0.1 }
```

## Coordinate conversion

Convert between screen and world space — useful for mouse picking:

```olrn
mouse_screen :Vec2 = mk.mouse_pos()
mouse_world  :Vec2 = mk.screen_to_world2d(mouse_screen, cam)

# and back
screen_pos :Vec2 = mk.world_to_screen2d(world_pos, cam)
```

## Full example

```olrn
@import ( mk = @std.gdev )

fn main() -> !void
{
    try mk.init_window(800, 600, "Camera Demo")
    defer mk.close_window()
    mk.set_fps(60)

    px :f32 = 400.0
    py :f32 = 300.0
    zoom :f32 = 1.0

    while !mk.should_close() {
        dt :: mk.dt()
        if mk.key_down(mk.keys.RIGHT) { px += 200.0 * dt }
        if mk.key_down(mk.keys.LEFT)  { px -= 200.0 * dt }
        if mk.key_down(mk.keys.UP)    { py -= 200.0 * dt }
        if mk.key_down(mk.keys.DOWN)  { py += 200.0 * dt }
        zoom += mk.mouse_scroll() * 0.1
        if zoom < 0.1 { zoom = 0.1 }

        cam := mk.camera2d(
            mk.vec2(px, py),
            mk.vec2(400.0, 300.0),
            zoom,
        )

        mk.begin_draw()
            mk.clear_bg(mk.DARKGRAY)
            mk.begin_camera2d(cam)
                mk.draw_rect(px - 16.0, py - 16.0, 32.0, 32.0, mk.RED)
            mk.end_camera2d()
            mk.draw_text("Arrow keys: move  Scroll: zoom", 10.0, 10.0, 14.0, mk.WHITE)
        mk.end_draw()
    }
}
```
