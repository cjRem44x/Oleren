# Oleren — Design Roadmap

## Mission

A thin, sugared frontend that compiles to explicit C++ and then to a native binary.
Target use: fast media applications — games, audio/video pipelines, asset tools.
No runtime, no GC, no OOP. Close to the metal; readable by default.

---

## Already Specced (in Notes.md)

- [x] Project manager (`olrn init / build / run / sac`)
- [x] Project directory layout
- [x] Functions (`fn`, `ret`, `-> type`, void shorthand)
- [x] Variables — mutable/immutable, explicit/implicit inference
- [x] Primitive types (`i8`–`i64`, `u8`–`u64`, `f32`, `f64`, `chr`, `bool`)
- [x] Arrays (`[]T`, `[N]T`, `[]imu T`, `undef`)
- [x] Strings (`chr`, `[]chr`, `str`, `istr`) and `str` method surface
- [x] Loops (`for`, `loop`, `while`)
- [x] `if / elif / else`
- [x] `when` (switch/match statement)
- [x] Heap allocation (`@alo`, `@free`, `defer`)
- [x] Raw pointers (`*`) and smart pointers (`^`)
- [x] Enums (plain and typed `=> T`)
- [x] Unions (`unn`, `unn(enum)`)
- [x] Structs (data fields, static vars, `pub fn`, instance `@self` fns)
- [x] Expression rules (no `;` by default, `;` for multi-expr lines)
- [x] Console I/O (`@pl`, `@pf`, `@cout`, `@cin`)

---

## Remaining Language Design

### 1. Expressions & Evaluation

**Binary expressions and operator precedence.**
Currently the parser handles no binary ops. Need a proper Pratt parser or
precedence-climbing pass so things like `x * 2 + y / z` evaluate correctly.

Standard precedence (high → low):
```
unary:   - ! * & ^
factor:  * / %
term:    + -
shift:   << >>
compare: < > <= >= == !=
bitwise: & ^ |
logical: and or
assign:  = := ::
```

**if / when as expressions.**
Noted in the spec (`y := if W {5} else {6}`) but not fully defined.
Rules needed:
- Both branches must produce the same type.
- `when` arms must all produce the same type, or `_` arm must exist.
- Result is the value of the taken branch.
```rust
label := when code {
    200 => "ok",
    404 => "not found",
    _   => "other",
}
```

**Unary expressions.**
`-x`, `!x`, `*p` (deref), `&x` (address-of), `^x` (smart-deref).

---

### 2. Type System Gaps

**Explicit casting.**
No cast syntax defined. Options — pick one:
```rust
x := @cast(i32, my_f64)   # builtin cast (Zig-style)
x := my_f64 as i32        # keyword cast (Rust-style)
```
Recommendation: `@cast(T, val)` — consistent with the `@` builtin pattern.

**`bool` and `void` in the type table.**
Both are used throughout but not listed in the primitive type table in Notes.md.
Add them for completeness.

**Type aliases.**
Useful for readability in media code (`Vec2 :: [2]f32`).
Syntax to decide:
```rust
type Vec2 = [2]f32
type Color = u32
```

**Multi-return / out-values.**
No tuple or multi-return defined. Options:
```rust
fn min_max(arr: []f32) -> (f32, f32) { ... }   # tuple return
x, y := min_max(data)

# OR just use a struct — simpler, no new syntax
```
Recommendation: struct return for now; revisit tuples later.

---

### 3. Module & Import System — RESOLVED

Single `import` block at the top of each file. All imports aliased. See Notes.md § Imports.

```rust
import (
    x  = "file.olrn",       # local file by path
    y  = "../other/file",    # relative path
    mk = @malkur,            # builtin/stdlib lib via @name
    io = @file,
)
```

---

### 4. Error Handling — RESOLVED

Zig-style error unions. See Notes.md § Error Handling for the full spec.

Summary:
- `err Name { FOO, BAR }` defines a named error set (`err` is the keyword)
- `!T` generic union, `MyErrors!T` named set, `err.NAME!T` single error
- `ret err.NAME` / `ret MyErrors.NAME` returns an error value
- `try expr` propagates an error to the caller
- `catch` handles an error inline (fallback value or `|e|` block)
- `if expr |val| { } else |e| { }` branches on success/error
- `errdefer` runs cleanup only on the error path
- `main -> void` or `main -> !void`; the `!void` form is the top-level handler (runtime prints error and exits)

---

### 5. Missing Builtins

| Builtin | Purpose |
|---|---|
| `@cast(T, val)` | Explicit type coercion |
| `@sizeof(T)` | Size of type in bytes |
| `@alignof(T)` | Alignment of type |
| `@assert(cond, msg)` | Debug assertion; no-op in release |
| `@panic(msg)` | Immediate abort with message |
| `@type(val)` | Already noted; return type as string |
| `@min(a,b)` / `@max(a,b)` | Scalar min/max |
| `@clamp(v,lo,hi)` | Clamp value to range |
| `@memcpy(dst,src,n)` | Raw memory copy |
| `@memset(ptr,val,n)` | Raw memory set |
| `@sqrt(x)` / `@abs(x)` | Scalar math |

---

### 6. C / C++ Interop (FFI)

Required for using SDL, Vulkan, OpenAL, platform APIs, etc.

Proposed:
```rust
# declare an external C function
extern fn SDL_Init(flags: u32) -> i32
extern fn SDL_Quit()

# use as normal
SDL_Init(0x00000020)
defer SDL_Quit()
```

`extern` functions bypass codegen and emit a raw C-style declaration.
Header inclusion handled via a `cc_include` directive in `olrn_pkg.toml`.

---

### 7. Standard Library Scope (Media Focus)

These are the modules the stdlib needs to cover for v1.
Implementation comes later; design the surface now.

**`std.io`** — file and stream I/O
```rust
use std.io

f := io.open("data.bin", .Read)
defer io.close(f)

buf :[]u8 = io.read(f, 1024)
io.write(f, buf)

io.seek(f, 0, .Start)
pos :: io.tell(f)
```

**`std.math`** — scalar and vector math
```rust
use std.math

x := math.sqrt(2.0)
x := math.sin(math.PI)
x := math.clamp(v, 0.0, 1.0)

# vectors
v2 := math.Vec2{.x=1.0, .y=0.0}
v3 := math.Vec3{.x=0, .y=1, .z=0}
v4 := math.Vec4{...}

# matrix (column-major, matches GPU conventions)
m := math.Mat4.identity()
m  = math.Mat4.translate(m, v3)
m  = math.Mat4.rotate(m, angle, axis)
```

**`std.mem`** — memory utilities
```rust
use std.mem

arena := mem.Arena.init(mem.MB(64))
defer arena.deinit()

ptr := arena.alloc(MyStruct)   # bump-alloc from arena
mem.copy(dst, src, n)
mem.set(ptr, 0, n)
```

**`std.str`** — string utilities (beyond built-in str methods)
```rust
use std.str

n   := str.parse_int("42")
s   := str.fmt("x = {}", x)      # heap-alloc formatted string
arr := str.split(s, ",")
```

**`std.fs`** — filesystem (paths, dir listing)
```rust
use std.fs

exists := fs.exists("assets/tex.png")
fs.mkdir("out/")
entries := fs.ls("assets/")
```

**`std.time`** — timing (critical for game loops)

**`@malkur` (Malkur)** — built-in gamedev library. See `docs/Malkur.md` for the full API surface. Covers window, input, 2D/3D drawing, textures, fonts, models, shaders, audio, math types, and collision. Raylib-inspired API; raw OpenGL/SDL2/Vulkan backend.
```rust
use std.time

t0 := time.now()          # nanosecond timestamp
dt := time.since(t0)      # elapsed as f64 seconds
time.sleep(0.016)         # sleep for ~1 frame at 60fps
```

**`std.thread`** (v2, not v1) — basic threading

---

### 8. `olrn_pkg.toml` Format

Needs a spec. Proposed minimal schema:

```toml
[project]
name    = "my_game"
version = "0.1.0"
entry   = "src/main/olrn/main.olrn"

[build]
std     = "c++17"
opt     = "release"          # debug | release
cc_flags = ["-O2"]

[deps]
sdl2 = { path = "../vendor/SDL2", headers = ["SDL.h"] }

[cc_include]
headers = ["SDL2/SDL.h", "vulkan/vulkan.h"]
libs    = ["-lSDL2", "-lvulkan"]
```

---

### 9. `mod` — Deferred

`mod` will be Oleren's flavor of a class-like module. Not OOP — no inheritance.
Closer to a namespace with encapsulated state and associated functions.
Design deferred; spec when needed.

---

### 10. Syntax Gaps to Resolve

**Multi-declaration inconsistency.**
Two styles appear in Notes.md — pick one and remove the other:
```rust
# Style A (used in var section)
mut i32: x = 0, y = 44, z = 543

# Style B (used everywhere else)
x :i32 = 0
y :i32 = 44
z :i32 = 543
```
Recommendation: **Style B only**. Style A's `mut` keyword conflicts with the
`:=` / `:type=` system and adds a second declaration grammar for no gain.

**Anonymous struct init (`.{}`).**
Used throughout but never formally defined.
Rule: `.{...}` expands to the type expected by the enclosing context.
```rust
foo : Foo = .{.x = 1, .y = 2.0}   # type known from decl
arr : []Foo = { .{.x=0, .y=0}, .{.x=1, .y=1} }
fn take_foo(f: Foo) {}
take_foo(.{.x=0, .y=0})           # type inferred from param
```

**`defer` full semantics.**
Currently only shown with `@free`. Full rules needed:
- Executes at end of enclosing *block* scope, not function scope.
- Multiple defers in the same block execute in LIFO order.
- Defer captures the expression at the point of declaration.

---

### 10. Generics — RESOLVED

No angle-bracket generics. Uses `any` parameter type + `@type(val)` + `when T { }` dispatch.
Compiler monomorphizes per unique concrete type — no runtime overhead.
See Notes.md § Generics.

---

## Compiler Implementation Order

Once the above is designed, implementation should proceed in this order:

1. Binary expressions + operator precedence (Pratt parser)
2. `if / when` as expressions
3. Variable declarations (`x := ...`, `x :T = ...`, `:T:`)
4. Type casting (`@cast`)
5. `any` type + `@type()` + `when T` dispatch
6. Import system (`import ( x = "path", lib = @name )`)
7. `extern` FFI declarations
8. `std.io` (file open/read/write/close)
9. `std.math` (scalar + Vec2/3/4, Mat4)
10. `std.mem` (arena allocator)
11. `std.time` (game loop timer)
