# Oleren `0.1.0`

A thin, sugared frontend that compiles to C++ and then to a native binary.
Designed for fast media applications — games, audio/video pipelines, asset tools.
No runtime, no GC, no OOP.

---

## Quick Start

```sh
# build the compiler
make

# create a project
mkdir myGame && cd myGame
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
import (
    std = @libs.std,
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
| Error union | `fn foo() -> !T` / `fn foo() -> ErrSet!T` |
| Propagate error | `val := try expr` |
| Handle error | `val := expr catch fallback` |
| Error cleanup | `errdefer s.free()` |
| Loops | `for e => arr {}` / `while cond {}` / `loop i:=0, i<N, i+=1 {}` |
| Generics | `fn f(x: any)` + `when @type(x) { i64 => ... }` |
| Extern FFI | `extern fn SDL_Init(flags: u32) -> i32` |

---

## CLI Commands

| Command | Description |
|---|---|
| `olrn init` | Create a new project in the current directory |
| `olrn build` | Compile Oleren → C++ → binary |
| `olrn run` | Build then run the output binary |
| `olrn build-src` | Compile Oleren → C++ only |
| `olrn build-out` | Compile existing C++ output → binary |
| `olrn check` | Parse and type-check without emitting |
| `olrn emit` | Print generated C++ to stdout |
| `olrn sac <files> -o=<name>` | Stand-alone compiler — no project required |
| `olrn version` | Print compiler version |
| `olrn --help` | Print usage |

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

## Project Layout

```
/myGame
    /bin          # compiled binary
    /src/main/olrn
        main.olrn # entry point
    /olrn_out     # generated C++
    olrn_pkg.toml # build config
    README.md
```

---

## Docs

- [`docs/Notes.md`](docs/Notes.md) — full language reference
- [`docs/StdLib.md`](docs/StdLib.md) — standard library (`std.math`, `std.io`, `std.mem`, `std.str`, `std.time`)
- [`docs/Malkur.md`](docs/Malkur.md) — Malkur gamedev library (`@libs.malkur`)
- [`docs/Roadmap.md`](docs/Roadmap.md) — design roadmap and implementation status
