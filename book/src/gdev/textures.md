# Textures & Fonts

## Loading textures

Supports BMP, PNG, and JPG (PNG/JPG via SDL_image).

```olrn
tex :Texture = try mk.load_texture("assets/player.png")
defer mk.unload_texture(tex)
```

## Drawing textures

```olrn
# basic — position + tint (use mk.WHITE for no tint)
mk.draw_texture(tex, x, y, mk.WHITE)

# extended — position, rotation, scale, tint
mk.draw_texture_ex(tex, pos, rot, scale, tint)

# subrect — draw a region of the texture (sprite sheet)
src :Rect = mk.rect(0.0, 0.0, 32.0, 32.0)   # source region
dst :Rect = mk.rect(100.0, 100.0, 64.0, 64.0) # destination on screen
mk.draw_texture_rect(tex, src, dst, mk.WHITE)
```

## Bitmap font (built-in)

No loading required — always available.

```olrn
mk.draw_text("Score: 100", x, y, size, color)
sz :Vec2 = mk.measure_text("hello", 16.0)
```

`size` controls the pixel height. The built-in font is an 8×8 bitmap, scaled up.

## TTF fonts

Requires SDL_ttf. Load at a fixed pixel size:

```olrn
font :Font = try mk.load_font("assets/font.ttf", 24)
defer mk.unload_font(font)

pos :Vec2 = mk.vec2(10.0, 10.0)
mk.draw_text_ex(font, "Hello!", pos, 24.0, 1.0, mk.WHITE)

sz :Vec2 = mk.measure_text_ex(font, "Hello!", 24.0, 1.0)
```

`spacing` (4th param of `draw_text_ex`) controls extra pixels between characters.

## Sprite sheet example

```olrn
sheet := try mk.load_texture("assets/tileset.png")
defer mk.unload_texture(sheet)

tile_w :f32 = 16.0
tile_h :f32 = 16.0
tile_id :i32 = 3   # 4th tile in the first row

src := mk.rect(@f32(tile_id) * tile_w, 0.0, tile_w, tile_h)
dst := mk.rect(200.0, 100.0, 32.0, 32.0)
mk.draw_texture_rect(sheet, src, dst, mk.WHITE)
```
