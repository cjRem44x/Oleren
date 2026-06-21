# Gdev — Gamedev Library

Gdev is the built-in gamedev library. Raylib-inspired flat API, SDL2 backend. Import it with:

```olrn
@import ( mk = @std.gdev )
```

`mk` is the conventional alias. All Gdev calls go through it: `mk.init_window(...)`, `mk.draw_rect(...)`, etc.

## System requirements

`olrn deps` checks and reports status:

```sh
$ olrn deps
└─ SDL2        (@std.gdev)  found 2.32.70
└─ SDL2_image  (@std.gdev)  found 2.8.4
└─ SDL2_mixer  (@std.gdev)  found 2.8.1
└─ SDL2_ttf    (@std.gdev)  found 2.20.2
```

Install on common platforms:

```sh
# Arch / CachyOS
pacman -S sdl2 sdl2_image sdl2_mixer sdl2_ttf

# Ubuntu / Debian
apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
```

## Minimal game loop

```olrn
@import ( mk = @std.gdev )

fn main() -> !void
{
    try mk.init_window(800, 600, "My Game")
    defer mk.close_window()
    mk.set_fps(60)

    while !mk.should_close() {
        dt :: mk.dt()

        mk.begin_draw()
            mk.clear_bg(mk.DARKGRAY)
            mk.draw_text("Hello, Gdev!", 300.0, 280.0, 24.0, mk.WHITE)
        mk.end_draw()
    }
}
```

## Feature overview

| Area | What's included |
|---|---|
| [Window & Rendering](rendering.md) | Window, frame loop, 2D shapes, colors |
| [Input](input.md) | Keyboard, mouse, gamepad |
| [Textures & Fonts](textures.md) | PNG/JPG loading, bitmap + TTF text |
| [Audio](audio.md) | Sounds, streaming music |
| [Camera](camera.md) | 2D camera with pan/zoom |

## Error type

Gdev functions that can fail return `GdevError!T`:

```olrn
err GdevError { InitFailed, LoadFailed, AudioFailed }
```
