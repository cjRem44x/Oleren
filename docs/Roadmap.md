# Oleren — Design Roadmap

## Mission

A thin, sugared frontend that compiles to explicit C++ and then to a native binary.
Target use: fast media applications — games, audio/video pipelines, asset tools.
No runtime, no GC, no OOP. Close to the metal; readable by default.

---

## Already Specced (in Language.md)

- [x] Project manager (`olrn init / build / run / sac`)
- [x] Project directory layout
- [x] Functions (`fn`, `ret`, `-> type`, void shorthand)
- [x] Variables — mutable/immutable, explicit/implicit inference
- [x] Primitive types (`i8`–`i64`, `u8`–`u64`, `f32`, `f64`, `bool`)
- [x] Arrays (`[]T`, `[N]T`, `[]imu T`, `undef`)
- [x] Strings — `str` (managed, std::string-backed) and `istr` (immutable contents); `chr`/`[]chr` removed
- [x] `.len` property on `str` and arrays (`len` is a reserved field name); type is `usize` (pointer-sized unsigned, like Zig)
- [x] Project layout — `init` scaffolds `src/main/olrn/`, `bin/`, `olrn_out/`; `build`/`run` use it (flat `main.olrn` still works)
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
- [x] `@pf` expression interpolation for literal formats and positional
      formatting for stored `str` formats
- [x] Resolver layer for direct imports: canonical local paths, duplicate-file
      import rejection, and self-import diagnostics
- [x] Focused sema hardening: call arity, bare return misuse, duplicate top-level
      functions, `null` restricted to pointer locals, and `@free` restricted to
      raw pointers

---

## Remaining Language Design

### Current Next Steps

1. **Struct field type registry.** — NEXT
   `type_of_expr` returns `TY_UNKNOWN` for any field access (`p.x`, `obj.count`).
   A struct name → field → type table would let sema check field reads/writes,
   catch struct literal field mismatches, and complete the narrowing story for
   brace-init.
2. **Sema for imported module functions.**
   The current `check_program` only walks top-level `NODE_FN_DECL` nodes;
   functions inside `NODE_MODULE` wrappers are skipped. Extend the walk to recurse
   into module decls so every function in the import graph is type-checked.
3. **Generic collection element types.**
   `@ls(T)`, `@map(K,V)`, `@set(T)` carry element types in the AST but `type_of_expr`
   returns `TY_UNKNOWN` for them. Representing `@ls(i32)` as a typed slot would catch
   element-type mismatches on `add`, `get`, and `for e => list`.
4. **Stdlib depth.**
   Prioritize `std.str.fmt`/builder, fallible buffered file I/O, `std.path`, and
   process/env helpers before adding new large domains.

### Completed

- ~~**Build a real expression type model in sema.**~~ **DONE (v0.1.6)**
  `OlrnType` (TyKind + name) lives in every symbol table slot. `type_of_expr()`
  infers types for literals, idents, direct calls, builtin casts (`@i32`, `@f64`, …),
  binary ops, and unary ops. `check_compat()` enforces compatibility at var decl,
  assignment, return, and call arguments. Int literals out of range → error; in-range
  narrowing → silent. Computed expression narrowing (e.g., i64 → i32) → warning.
  Float literals into `:f32` → silent; f64 expression into f32 → warning.
  `-Wno-narrowing` added to g++ flags; sema owns narrowing diagnostics now.

- ~~**Recursive module graph.**~~ **DONE (v0.1.6)**
  `ImportCtx` with a shared visited-set drives depth-first local import resolution.
  Transitive dependencies are emitted before their dependents (correct namespace
  ordering in C++). Diamond dependencies (same file imported by two modules)
  are deduplicated silently. Import cycles are silently broken via the visited-set.
  Codegen pre-registers all `NODE_MODULE` names before the emission pass so
  `is_module_alias()` recognizes transitive aliases inside namespace blocks.

- ~~**Diagnostics with source spans.**~~ **DONE (v0.1.6)**
  `Token` and `AstNode` now carry a `col` field. The lexer tracks the column via
  `adv()`. Parser errors report `error:line:col:` and print a source excerpt with a
  caret. Sema errors use the same `error:line:col:` format. All 68
  `ast_node_new()` call sites updated; resolver passes `(0, 0)` for synthetic nodes.

---

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
logical: and or        # keywords; compile to && ||
assign:  = := ::
```

**if / when as expressions — RESOLVED.**
Both compile to immediately-invoked C++ lambdas. Type is inferred from the
branch values; C++ enforces consistency.
```rust
msg   := if x > 0 { "pos" } elif x < 0 { "neg" } else { "zero" }
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

**Explicit casting — RESOLVED.** `@T(val)` syntax — type name is the builtin.
Numeric-to-numeric always succeeds. String-to-numeric returns `!T` (use `try`/`catch`).
```rust
x := @i32(3.145)           # f64 -> i32
s := @str(42)              # int -> str
n := try @i32("42")        # str -> i32, can fail
```

**`bool` and `void` in the type table.**
Both are used throughout but not listed in the primitive type table in Language.md.
Add them for completeness.

**Type aliases — RESOLVED.** `type T = B` syntax.
```rust
type Vec2  = [2]f32
type Color = u32
```

**Multi-return / out-values — RESOLVED.**
`(T1, T2, ...)` return type, `(a, b, ...)` tuple literal, `a, b := f()` /
`a, b :: f()` destructuring, `_` to discard a slot. Compiles to `std::tuple` +
C++17 structured bindings.
```rust
fn min_max(a: i32, b: i32) -> (i32, i32) { ret (a, b) }
lo, hi :: min_max(3, 7)
_, hi2 :: min_max(1, 9)   # discard first
```

---

### 3. Module & Import System — RESOLVED

Single `@import` block at the top of each file. All imports aliased.
Submodules can also be bound top-level: `io :: @std.io`. See Language.md § Imports.

```rust
@import (
    x   = "file.olrn",          # local file by path
    y   = "subdir/file.olrn",   # same-tree relative path
    std = @std,          # full stdlib — access as std.file, std.enc, std.hash ...
    mk  = @std.gdev,       # Gdev gamedev library
)
```

Implemented resolver behavior:
- Local file imports are relative to the importing file.
- Absolute imports and `..` path components are rejected.
- Direct self-imports and import cycles are silently broken via the visited-set.
- Transitive imports are resolved depth-first; dependencies are emitted before
  dependents so the generated C++ namespace order is always correct.
- Diamond dependencies (same file imported transitively by two modules) are
  deduplicated — the file is parsed and emitted exactly once under the first alias seen.

Still planned:
- Parse/check caching across the full graph (currently each import is re-parsed).
- Public/private export checks for non-function module members.
- Sema walk into imported module functions (currently only top-level fns are checked).

---

### 4. Error Handling — RESOLVED

Zig-style error unions. See Language.md § Error Handling for the full spec.

Summary:
- `err Name { FOO, BAR }` defines a named error set (`err` is the keyword)
- `!T` generic union, `MyErrors!T` named set union
- `ret err.NAME` / `ret MyErrors.NAME` returns an error value
- `try expr` propagates an error to the caller
- `catch` handles an error inline (fallback value, capture-less `{ }` block, or `|e|` block)
- `errdefer` runs cleanup only on the error path
- `MyErrors!T` is enforced by a sema pass: returned errors must belong to
  the declared set (and name a real variant), and `try` may not propagate
  a different named set (generic `!T` callees are the escape hatch)
- string→numeric casts are fallible: `@i32("42")` returns `!i32`
- `main -> void` or `main -> !void`; the `!void` form is the top-level handler (runtime prints error and exits)

---

### 5. Builtins — RESOLVED

All builtins documented in Language.md § Builtins.

**Memory / safety**
| Builtin | Purpose |
|---|---|
| `@alo(T)` | Heap-allocate T (raw `new T()`) |
| `@free(p)` | Free raw pointer |
| `@memcpy(dst,src,n)` | Raw memory copy |
| `@memset(ptr,val,n)` | Raw memory set |
| `@sizeof(T)` | Size of type in bytes |
| `@alignof(T)` | Alignment of type |

**Assertions / control flow**
| Builtin | Purpose |
|---|---|
| `@assert(cond, msg)` | Debug assertion; aborts on failure |
| `@panic(msg)` | Immediate abort with message |
| `@unreachable()` | Marks a path that must never be reached |
| `@exit(code)` | `std::exit(code)` — terminate immediately |

**I/O**
| Builtin | Purpose |
|---|---|
| `@pl(vals...)` | Print values + newline (`cout <<`) |
| `@pf("fmt {expr}", args...)` | Interpolated print |
| `@cout` | Raw `std::cout` handle |
| `@cin(prompt)` | Read line from stdin |

**Type / math**
| Builtin | Purpose |
|---|---|
| `@type(val)` | Demangled type name of val as `str` |
| `@T(val)` | Numeric/string cast (`@i32`, `@f64`, `@str`, …) |
| `@rng(T, low, high)` | Random value of type T in [low, high] |
| `@min(a,b)` / `@max(a,b)` | Scalar min/max |
| `@clamp(v,lo,hi)` | Clamp value to range |
| `@sqrt(x)` / `@abs(x)` | Scalar math |
| `@hex(v)` | Integer → lowercase hex string |

**System / process**
| Builtin | Purpose |
|---|---|
| `@args` | `[]str` — command-line args (argv\[1..\]); supports `.len`, `[i]`, `for` |
| `@cmd("...")` `-> i32` | `system()` — run shell command, return exit code |
| `@getenv("VAR")` `-> str` | Read env variable; `""` if unset |
| `@pid()` `-> i32` | Current process ID |
| `@sleep(ms)` | Sleep for N milliseconds |

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

All stdlib access goes through the import alias: `std.module.fn(...)`
compiles to the flat `module_fn(...)` name, which may also be called
directly. Constants and types are unprefixed (`PI`, `File`, `IOMode`).

**`std.io`** — file and stream I/O
```rust
@import ( std = @std )

f := std.io.open("data.bin", IOMode.Read)
defer std.io.close(f)

buf :[]u8 = std.io.read(f, 1024)
std.io.write(f, buf)

std.io.seek(f, 0, SeekFrom.Start)
pos :: std.io.tell(f)
```

**`std.math`** — scalar and vector math
```rust
@import ( std = @std )

x := std.math.sqrt(2.0)
x := std.math.sin(std.math.PI)
x := std.math.clamp(v, 0.0, 1.0)

# vectors (types are unprefixed, like all stdlib types)
v2 := Vec2{.x=1.0, .y=0.0}
v3 := Vec3{.x=0, .y=1, .z=0}
v4 := Vec4{...}

# matrix (column-major, matches GPU conventions)
m := Mat4.identity()
m  = Mat4.translate(m, v3)
m  = Mat4.rotate(m, angle, axis)
```

**`std.mem`** — memory utilities
```rust
arena := Arena.init(std.mem.MB(64))
defer arena.deinit()

ptr := arena.alloc(MyStruct)   # bump-alloc from arena
std.mem.copy(dst, src, n)
std.mem.set(ptr, 0, n)
```

**`std.str`** — string utilities (beyond built-in str methods)
```rust
n   := std.str.parse_int("42")
s   := std.str.fmt("x = {}", x)      # heap-alloc formatted string
arr := std.str.split(s, ",")
```

**`std.fs`** — filesystem (paths, dir listing) — IMPLEMENTED
```rust
exists := std.fs.exists("assets/tex.png")    # also: is_dir, is_file
std.fs.mkdir("out/")                          # recursive, like mkdir -p
entries :[]str = std.fs.ls("assets/")         # sorted file names
n := std.fs.size("a.bin")                     # -1 if missing
std.fs.copy(from, to); std.fs.rename(from, to)
std.fs.rm(path); std.fs.rm_all(path)          # rm_all returns count removed
cwd := std.fs.cwd()
```

**`std.time`** — timing (critical for game loops)

**`@std.gdev` (Gdev)** — built-in gamedev library. See `docs/Gdev.md` for the full API surface. Raylib-inspired flat API on an SDL2 backend; importing auto-links `-lSDL2`. v0.4 shipped: window/loop, keyboard, mouse, gamepad (4 slots, hotplug, `pad_btn_released`, `pad_count`, `pad_name`, `pad_rumble`), 2D shapes, `draw_rect_rot`, textures (BMP + PNG + JPG via SDL_image, subrect), camera 2D (pan/zoom, world↔screen), embedded 8×8 bitmap font (`draw_text`/`measure_text`), TTF fonts (`load_font`/`draw_text_ex`/`measure_text_ex` via SDL_ttf), audio (sounds + streaming music via SDL_mixer), colors + `hex()`, Vec2 math, 2D collision.
```rust
t0 := std.time.now()          # nanosecond timestamp
dt := std.time.since(t0)      # elapsed as f64 seconds
std.time.sleep(0.016)         # sleep for ~1 frame at 60fps
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

### 9. `mod` — Dropped

No `mod` construct. Structs (data fields, static vars, `pub fn`, instance
`@self` fns) cover the encapsulated-state use case; no separate class-like
module will be added.

---

### 10. Syntax Gaps to Resolve

**`else` / `elif` on their own line — RESOLVED.**
`skip_newlines()` is now called in `parse_if_chain` after parsing the then-block, so
`else` and `elif` are recognised correctly whether they appear on the same line as `}`
or on the next line.

**Multi-declaration — RESOLVED.**
Both forms are intentional and serve different purposes:
- `:T=` / `:=` — single variable declaration
- `mut T: a=v, b=v, ...` / `imu T: a=v, b=v, ...` — group of same-type vars in one expression

```rust
mut i32: x=0, y=0, width=800, height=600   # group, same type
x :i32 = 0                                  # single
```

**Anonymous struct init (`.{}`) — RESOLVED.** Works identically to Zig.
Type inferred from context; compiler warns if type cannot be determined.

**`defer` semantics — RESOLVED.** Works identically to Zig:
block scope, LIFO, expression evaluated at execution time, block form supported.

---

### 10. Generics — RESOLVED

No angle-bracket generics. Uses `any` parameter type + `@type(val)` + `when T { }` dispatch.
Compiler monomorphizes per unique concrete type — no runtime overhead.
See Language.md § Generics.

---

## Compiler Status — v0.1.6

- [x] Binary expressions + Pratt parser (full operator precedence)
- [x] `if / elif / else`, `when` (switch/match)
- [x] Variable declarations — mutable/immutable, explicit/implicit
- [x] Type casting (`@T(val)`)
- [x] `any` type + `@type()` + `when T` dispatch (monomorphized generics)
- [x] Import system (`@import ( alias = "path" / @std / @std.module )`, top-level `io :: @std.io` binds; `@pkg.libname` reserved for toml deps)
- [x] `extern` FFI declarations
- [x] Loops — `for`, `loop`, `while`
- [x] `defer` / `errdefer`
- [x] Error handling — `err`, `!T`, `ErrSet!T`, `try`, `catch`, `errdefer`
- [x] Structs, enums, unions
- [x] Heap allocation (`@alo`, `@free`), raw and smart pointers
- [x] Standard library — `std.io`, `std.fs`, `std.math`, `std.mem`, `std.str`, `std.time`, `std.log`, `std.thread`
- [x] CLI — `build`, `run`, `build-src`, `build-out`, `check`, `emit`, `sac`, `init`
- [x] Gdev gamedev library v0.4 — gamepad, camera 2D, rotated/subrect draws, embedded + TTF fonts, hex(), PNG/JPG textures (SDL_image), audio sounds + music (SDL_mixer) via `@std.gdev` (SDL2 backend)
- [x] Sema pass — `ErrSet!T` enforcement, unused-import + alias-shadowing errors
- [x] System deps — `olrn deps` + build-time resolution via pkg-config with per-OS fallbacks
- [x] Multi-return / tuples — `fn f() -> (T1, T2)`, `a, b := f()`, `_` discard
- [x] **`@ls(T)`** — built-in growable list; method-style API (`add`, `pop`, `insert`, `remove`, `clear`, `deinit`, `len`, `cap`, `[i]`); `@type` aware
- [x] **`@map(K, V)`** — built-in hash map; method-style API (`set`, `get`, `has`, `del`, `clear`, `deinit`, `keys()`, `vals()`, `len`); `for k, v => map` key-value iteration; `@type` aware
- [x] **`@set(T)`** — built-in hash set; method-style API (`add`, `has`, `del`, `clear`, `deinit`, `len`); range-for; `@type` aware
- [x] Struct static fields (`pub NAME : T : val` / `pub NAME : T = val`) and struct methods (`pub fn`)
- [x] Smart pointer postfix deref (`p.^`) and pointer-to-pointer types (`**T`, `***T`)
- [x] Integer literal formats — hex (`0xFF`) and binary (`0b1010`)
- [x] `@hex(v)` builtin — integer to lowercase hex string
- [x] **Crypt** (`@std.crypt`) — cryptography library (libsodium backend)
- [x] **File-as-module** — each imported `.olrn` file is a namespace; `pub fn` is accessible as `alias.func()`, bare `fn` is private with compile-time enforcement
- [x] `mstr` multiline string type (`"""..."""` triple-quote literals, shared `str` methods)
- [x] `olrn view <file>` — SDL2 viewer for images, animated GIFs, and video (zoom, pan, enhance)
- [x] Struct method privacy — bare `fn` in a struct is `private:` in C++; `pub fn` is public
- [x] **Source spans in diagnostics** — `Token` and `AstNode` carry `col`; parser errors show `error:line:col:` with source excerpt + caret; sema errors use the same format
- [x] **Recursive module graph** — transitive imports resolved depth-first; diamond deps deduplicated; import cycles silently broken; transitive module aliases recognized in codegen
- [x] **Expression type model** — `OlrnType` in symbol table; `type_of_expr()` for literals/idents/calls/casts/binary; `check_compat()` at var decl, assignment, return, call args; int out-of-range → error; computed narrowing → warning; `-Wno-narrowing` on g++ (sema owns narrowing)
- [x] **System builtins** — `@exit(code)`, `@cmd("...") -> i32`, `@getenv("VAR") -> str`, `@pid() -> i32`, `@sleep(ms)`, `@args -> []str` (argv\[1..\] with `.len`/index/for)

## Next (v0.6.0 candidates)

- [ ] Windows/macOS validation — dep layer and compiler are written portably
  (MinGW shims in place) but only Linux is exercised; needs CI + testing
- [ ] `olrn_pkg.toml` — deferred; only needed for *outside* resources
  (vendored C/C++ deps, link flags). Builtin libs don't need it.
- [ ] **Extended stdlib** — all tiers specced in `docs/StdLib.md`; Tier 1: `std.env`, `std.path`, `std.json`, `std.net`, `std.http`, `std.compress`, `std.regex`; Tier 2: `std.proc`, `std.bytes`, `std.date`, `std.uuid`, `std.toml`, `std.ws`; Tier 3: `std.csv`, `std.xml`, `std.test`, `std.rand`
- [ ] **Guix** (`@std.guix`) — native UI library (Qt6 backend); designed, not yet implemented.
  See `docs/Guix.md` for full API spec. Flat procedural API: windows, layouts, widgets, media
  (jpg/png/gif/webp/svg images, video, audio), menus, dialogs, canvas, system tray, timers, styling.
