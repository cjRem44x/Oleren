# Window & Rendering

## Window lifecycle

```olrn
try mk.init_window(800, 600, "Title")
defer mk.close_window()

mk.set_fps(60)
mk.set_vsync(true)
mk.set_title("New Title")
mk.fullscreen(true)
```

## Frame loop

```olrn
while !mk.should_close() {
    dt :: mk.dt()         # delta time in seconds (f32)
    fps :: mk.fps()       # current FPS (i32)
    w   :: mk.width()     # window width  (i32)
    h   :: mk.height()    # window height (i32)

    mk.begin_draw()
        mk.clear_bg(mk.BLACK)
        # ... draw calls here ...
    mk.end_draw()
}
```

All draw calls must be between `begin_draw()` and `end_draw()`.

## Colors

```olrn
mk.clear_bg(mk.DARKGRAY)

# built-in named colors
mk.WHITE  mk.BLACK  mk.RED  mk.GREEN  mk.BLUE
mk.YELLOW mk.ORANGE mk.PURPLE mk.DARKGRAY mk.GRAY

# custom RGBA (0–255 each)
red :Color = mk.rgba(255, 0, 0, 255)

# from 0xRRGGBBAA hex
col :Color = mk.hex(0xFF8800FF)

# fade (adjust alpha)
faded := mk.color_fade(mk.RED, 0.5)

# lerp between two colors
mid := mk.color_lerp(mk.RED, mk.BLUE, 0.5)
```

## 2D shapes

```olrn
mk.draw_rect(x, y, w, h, color)
mk.draw_rect_lines(x, y, w, h, thick, color)
mk.draw_rect_rot(x, y, w, h, origin, rot, color)   # rotated rect

mk.draw_circle(cx, cy, r, color)
mk.draw_circle_lines(cx, cy, r, color)

mk.draw_line(x1, y1, x2, y2, thick, color)

mk.draw_triangle(v1, v2, v3, color)
mk.draw_poly(center, sides, r, rot, color)
```

## Health orb

```olrn
mk.draw_orb(cx, cy, r, fill, col_full, col_empty)
# fill: 0.0 (empty) to 1.0 (full)
```

## Vec2 and Rect helpers

```olrn
v := mk.vec2(1.0, 2.0)
z := mk.vec2_zero()
r := mk.rect(x, y, w, h)

sum  := mk.v2_add(a, b)
diff := mk.v2_sub(a, b)
sc   := mk.v2_scale(v, 2.0)
dot  := mk.v2_dot(a, b)
len  := mk.v2_len(v)
dist := mk.v2_dist(a, b)
norm := mk.v2_norm(v)
lerp := mk.v2_lerp(a, b, 0.5)
```

## Collision

```olrn
hit   :bool = mk.check_rects(a, b)
hit2  :bool = mk.check_circles(c1, r1, c2, r2)
hit3  :bool = mk.check_point_rect(pt, r)
hit4  :bool = mk.check_point_circle(pt, center, r)
inter :Rect  = mk.rect_intersect(a, b)
```
