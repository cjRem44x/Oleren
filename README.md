# Oleren `0.2.0`

A thin, sugared frontend that compiles to C++ and then to a native binary.
Designed for fast media applications — games, audio/video pipelines, asset tools.
No runtime, no GC, no OOP.

---

## Quick Start

```sh
# build the compiler
make

# create a project
mkdir myGame
cd myGame
olrn init

# build & run
olrn build
olrn run

# or compile a single file directly
olrn sac main.olrn -o=main
./main
```

---

## Language at a Glance

```rust
@import (
    std = @std,
    mk  = @std.malkur,
)

err AppError { BadInput, NotFound }

fn greet(name: str) -> !str
{
    if name == "" { ret AppError.BadInput }
    ret "hello, " + name
}

fn main() -> !void
{
    msg := try greet("world")
    @pl(msg)
}
```

### Key features

| Feature | Syntax |
|---|---|
| Functions | `fn name(arg: T) -> R { ret val }` |
| Mutable var | `x := 42` / `x :i32 = 42` |
| Immutable var | `x :: 42` / `x :i32: 42` |
| Defer | `defer @free(ptr)` |
| Error union | `fn foo() -> !T` / `fn foo() -> ErrSet!T` (set enforced) |
| Propagate error | `val := try expr` |
| Handle error | `expr catch fallback` / `catch { }` / `catch \|e\| { }` |
| Error cleanup | `errdefer s.free()` |
| Pointers | `p :*T = &v` / `p :*T = @alo(T)` raw, `s :^T = @alo(T)` smart |
| Structs/enums | `struct P { x: i32 }` / `enum Dir { North }` / `unn U { ... }` |
| Length | `s.len` / `arr.len` — i64, on `str` and arrays |
| Loops | `for e => arr {}` / `while cond {}` / `loop i:=0, i<N, i+=1 {}` |
| Generics | `fn f(x: any)` + `when @type(x) { i64 => ... }` |
| Extern FFI | `extern fn SDL_Init(flags: u32) -> i32` |
| Fallible casts | `n := try @i32("42")` — string→numeric returns `!T` |
| Threading | `std.thread.spawn(fn)` / mutex / atomic i32 + CAS |

---

## CLI Commands

| Command | Description |
|---|---|
| `olrn init` | Scaffold a project (`src/`, `bin/`, `olrn_out/`, …) in cwd |
| `olrn build` | Compile the project entry point → `bin/<name>` |
| `olrn run` | Build then run the output binary |
| `olrn <file.olrn>` / `olrn emit <file.olrn>` | Emit generated C++ to stdout |
| `olrn build-src <file.olrn>` | Compile Oleren → C++ file (project-aware) |
| `olrn build-out <file.cpp>` | Compile existing C++ output → binary (project-aware) |
| `olrn check <file.olrn>` | Parse + semantic checks, no output |
| `olrn deps [file.olrn]` | Check system libs (SDL2, …) with per-OS install hints |
| `olrn sac <files> -o=<name>` | Stand-alone compiler — no project required |
| `olrn version` | Print compiler version |
| `olrn help` | Print usage |

---

## Error Handling

Errors are values, not exceptions. No hidden control flow.

```rust
err FileError { NotFound, PermDenied }

fn read_file(path: str) -> FileError!str
{
    if path == "" { ret FileError.NotFound }
    ret "data"
}

fn process() -> !void
{
    buf := read_file("") catch |e| {
        @pl("error: " + e.msg)
        ret err.Wrapped
    }

    data := try read_file("good.txt")  # propagate on failure

    res := read_file("maybe.txt") catch "default"  # fallback value

    errdefer @pl("cleanup on error")
    try do_more()
}

fn main() -> !void
{
    try process()  # uncaught errors print to stderr and exit 1
}
```

---

## Gamedev — Malkur

`@std.malkur` is the built-in gamedev library (Raylib-inspired flat API, SDL2
backend). `olrn deps` or `olrn build` auto-resolves `-lSDL2`.

```rust
@import ( mk = @std.malkur )

fn main() -> !void
{
    try mk.init_window(800, 600, "My Game")
    defer mk.close_window()
    mk.set_fps(60)

    cam := mk.camera2d(Vec2{.x=0.0, .y=0.0}, Vec2{.x=400.0, .y=300.0}, 1.0)
    pos := mk.vec2(0.0, 0.0)

    while !mk.should_close() {
        dt :: mk.dt()
        if mk.key_down(mk.keys.RIGHT) { pos.x += 200.0 * dt }
        if mk.key_down(mk.keys.LEFT)  { pos.x -= 200.0 * dt }
        if mk.pad_connected(0) and mk.pad_btn_pressed(0, mk.pad_btn.A) {
            @pl("jump!")
        }

        mk.begin_draw()
            mk.clear_bg(mk.DARKGRAY)
            mk.begin_camera2d(cam)
                mk.draw_rect(pos.x, pos.y, 40.0, 60.0, mk.GREEN)
            mk.end_camera2d()
            mk.draw_text("Oleren v0.2", 10.0, 10.0, 16.0, mk.WHITE)
        mk.end_draw()
    }
}
```

**v0.2 features:** window/loop, keyboard, mouse, gamepad (4 slots, hotplug),
2D shapes, `draw_rect_rot`, textures (BMP + subrect), camera 2D
(`begin_camera2d`/`end_camera2d`, world↔screen conversion), embedded 8×8 bitmap
font (`draw_text`/`measure_text`), colors + `hex()`, Vec2 math, 2D collision.

---

## Threading

```rust
@import ( std = @std )

fn worker(cnt: i64)
{
    std.thread.atomic_add(cnt, 1)
}

fn main() -> void
{
    cnt := std.thread.atomic_new(0)
    defer std.thread.atomic_free(cnt)

    t := std.thread.spawn_arg(worker, cnt)
    std.thread.join(t)

    @pl(std.thread.atomic_load(cnt))   # 1
}
```

---

## Project Layout

`olrn init` scaffolds the tree; `olrn build` compiles the entry point to
`olrn_out/<name>.cpp` and then to `bin/<name>` (named after the directory):

```
/myGame
    /bin          # compiled binary
    /src/main/olrn
        main.olrn # entry point
    /olrn_out     # generated C++
    olrn_pkg.toml # build config (scaffolded; not read by the build yet)
    README.md
```

A bare `main.olrn` in the current directory also works (flat mode) and
builds to `./<name>`.

---

## System Dependencies

Some stdlib modules need system libraries — `@std.malkur` needs SDL2.
The compiler resolves them per platform at build time (pkg-config first,
then Linux/macOS/Windows-MinGW fallbacks) and fails with install
instructions for your OS when one is missing. `olrn deps` reports the
status up front:

```
$ olrn deps
system deps for src/main/olrn/main.olrn
└─ SDL2 (@std.malkur)  found 2.32.70
```

---

## Docs

- [`docs/Notes.md`](docs/Notes.md) — full language reference
- [`docs/StdLib.md`](docs/StdLib.md) — standard library (`std.io`, `std.fs`, `std.math`, `std.mem`, `std.str`, `std.time`, `std.log`, `std.thread`)
- [`docs/Malkur.md`](docs/Malkur.md) — Malkur gamedev library (`@std.malkur`)
- [`docs/Roadmap.md`](docs/Roadmap.md) — design roadmap and implementation status
