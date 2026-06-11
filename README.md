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

---

## CLI Commands

| Command | Description |
|---|---|
| `olrn init` | Scaffold a project (`src/`, `bin/`, `olrn_out/`, …) in cwd |
| `olrn build` | Compile the project entry point → `bin/<name>` |
| `olrn run` | Build then run the output binary |
| `olrn <file.olrn>` / `olrn emit <file.olrn>` | Emit generated C++ to stdout |
| `olrn build-src <file.olrn>` | Compile Oleren → C++ file |
| `olrn build-out <file.cpp>` | Compile existing C++ output → binary |
| `olrn check <file.olrn>` | Parse + semantic checks, no output |
| `olrn sac <files> -o=<name>` | Stand-alone compiler — no project required |
| `olrn --version` / `-V` | Print compiler version |
| `olrn --help` / `-h` | Print usage |

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

## Docs

- [`docs/Notes.md`](docs/Notes.md) — full language reference
- [`docs/StdLib.md`](docs/StdLib.md) — standard library (`std.io`, `std.fs`, `std.math`, `std.mem`, `std.str`, `std.time`, `std.log`)
- [`docs/Malkur.md`](docs/Malkur.md) — Malkur gamedev library (`@std.malkur`)
- [`docs/Roadmap.md`](docs/Roadmap.md) — design roadmap and implementation status
