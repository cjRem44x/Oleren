# The Oleren Language

## Contents

- [Goal](#goal)
- [Compiling](#compiling)
- [Imports](#imports)
- [Variables & Types](#variables--types)
- [Strings](#strings) — `str`, `istr`, `mstr` (`"""..."""` multiline)
- [Arrays](#arrays)
- [Lists (`@ls`)](#lists-ls)
- [Collections (`@map`, `@set`)](#collections-map-set)
- [Functions](#functions)
- [Structs](#structs)
- [Enums](#enums)
- [Unions](#unions)
- [Loops](#loops)
- [Control Flow](#control-flow)
- [Pointers](#pointers)
- [Error Handling](#error-handling)
- [Generics](#generics)
- [Defer](#defer)
- [Builtins](#builtins)
- [Threading](#threading)
- [Malkur](#malkur)
- [Examples](#examples)

---

## Goal

Oleren is a more reasonable frontend for the C++ industry standard within game dev — a new paint job that simplifies and modernizes old-school performance without the headache. No OOP, no classes, no inheritance. Functions, structs with methods, and basic types cover everything; the goal is a modern-C feel that compiles to fast, correct C++.

---

## Compiling

The Oleren project manager handles building, assets, and organization.

Create a new project: `mkdir myGame && cd myGame && olrn init`. It scaffolds inside the current directory using the directory name as the project name.

```
/myOlrnDir
    /bin
        # compiled binary (bin/<name>)
    /src
        /main
            /olrn
                main.olrn   # entry point
    /olrn_out
        # generated C++ output (<name>.cpp)
    olrn_pkg.toml
    README.md
```

`olrn build` compiles `src/main/olrn/main.olrn` → `olrn_out/<name>.cpp` → `bin/<name>`. A bare `main.olrn` in the current directory (flat mode) still works.

| Command | Description |
|---|---|
| `olrn build` | Compile `main.olrn` → C++ → binary |
| `olrn run` | Build then run the binary |
| `olrn <file.olrn>` / `olrn emit <file.olrn>` | Emit generated C++ to stdout |
| `olrn build-src <file.olrn>` | Compile Oleren → C++ file (no binary) |
| `olrn build-out <file.cpp>` | Compile existing C++ → binary (skip frontend) |
| `olrn check <file.olrn>` | Parse + semantic checks, no output |
| `olrn deps [file.olrn]` | Check system libs (SDL2, …) with per-OS install hints |
| `olrn sac <files> [-o=name]` | Stand-alone: compile one or more files to binary |
| `olrn view <file>` | Open an image, GIF, or video in a built-in SDL2 viewer |
| `olrn version` | Print version |
| `olrn help` | Print usage |

`sac` works outside a project directory and accepts multiple source files:
```sh
olrn sac main.olrn util.olrn -o=myprog
```

### `olrn view`

Built-in media viewer — opens images, animated GIFs, and videos in an SDL2 window without writing any Oleren code:

```sh
olrn view photo.png
olrn view animation.gif
olrn view clip.mp4
```

Controls:

| Key / Action | Effect |
|---|---|
| Scroll wheel | Zoom in / out |
| `+` / `-` | Zoom in / out |
| `0` | Reset zoom and pan to 1:1 center |
| Click + drag | Pan |
| `E` | Toggle enhance (images and GIFs only): denoise → sharpen → S-curve levels |
| `Space` | Pause / resume (video only) |
| `Q` / `Esc` | Quit |

Supported formats: PNG, JPG, BMP, WebP, GIF (animated), MP4, MKV, AVI, MOV, WebM, and more.  
Video playback requires `ffmpeg` to be installed. Images and GIFs only need SDL2 + SDL2\_image.

---

## Imports

All imports live in a single `@import` block at the top of the file. Every import requires an alias — that alias is how you call into the module.

```rust
@import (
    x   = "file.olrn",       # local file, relative to current file
    y   = "../path/to/file", # relative path (no extension needed)

    std = @std,              # full standard library
    mk  = @std.malkur,       # one stdlib submodule
    ma  = @std.math,
)
```

Submodules can also be bound at the top level after the `@import` block:

```rust
io :: @std.io
mk :: @std.malkur
```

Rules:
- Only one `@import` block per file; it must appear before any declarations.
- Unused imports are a compile error.
- The alias is how you call in: `mk.init_window(...)`, `std.io.open(...)`

### File-as-module

Every `.olrn` file other than the main driver is a **module** — a self-contained namespace. Members follow the same `pub` / bare rule as struct methods:

| Declaration | Access |
|---|---|
| `pub fn name(...)` | Public — callable as `alias.name()` from any file |
| `fn name(...)` | Private — only callable from within the same file |

```rust
# util.olrn
pub fn ask(text: mstr) -> str { ... }   # accessible as util.ask()
fn internal_helper() { ... }            # private — util.internal_helper() errors
```

```rust
# main.olrn
@import( util = "util.olrn" )

fn main() -> void {
    util.ask("...")          # ok — pub
    util.internal_helper()   # ERROR: 'internal_helper' is private to module 'util'
}
```

The main driver (the file containing `fn main`) is exempt — its functions live in the global namespace and are not subject to module access rules.

---

## Variables & Types

Oleren uses a Go/Odin style for declarations.

```rust
x :i32 = 5      # explicit mutable
x := 5          # implicit mutable  (infers i64 for integers)

x :i32: 5       # explicit immutable (like const/constexpr)
x :: 5          # implicit immutable (infers i64)
```

### Multi-declaration

When several variables share the same type, use `mut T:` or `imu T:`:

```rust
mut i32: x=0, y=0, width=800, height=600
imu f32: pi=3.14, e=2.71, tau=6.28
```

### Type inference rules

```rust
x := 0          # => i64   (default integer)
x := 3.45       # => f64   (default float)
x := "hello"    # => str
x := false      # => bool
x := {1,2,3}    # => []i64
```

### Integer literals

Decimal, hexadecimal, and binary are all supported:

```rust
dec := 255          # decimal
hex := 0xFF         # hexadecimal — 255
bin := 0b11111111   # binary      — 255
```

`undef` requires an explicit type — `x := undef` is always a compile error.

### Primitive types

```
Integers              Floats
─────────────         ──────────
i8   u8   (byte)      f32
i16  u16              f64
i32  u32
i64  u64
```

### Type aliases

`type` creates a named alias for any existing type:

```rust
type EntityId = i32
type Velocity = f32
type Matrix   = [16]f32
```

---

## Strings

`str` is a managed string (`std::string` under the hood) — value semantics, automatic cleanup, no manual `@free`.

```rust
word :str = "hello"
word = word + " world"          # concatenation
same := word == "hello world"   # comparison

c := word[0]       # read a character
word[0] = 'H'      # write a character

n := word.len      # length (i64)
```

`istr` — immutable contents, mutable binding:

```rust
name :istr = "john"
copy :str  = name   # copies fine
# name[0] = 'J'    # compile error
```

`mstr` — multiline string, written with triple-quote `"""..."""` literals. The raw content (newlines, indentation) is preserved verbatim. Same underlying type as `str` (`std::string`), interchangeable at runtime:

```rust
bio :mstr = """
Name: Alice
Role: Engineer
"""

query := """
SELECT *
FROM users
WHERE active = 1;
"""

# immutable mstr
banner :mstr: """
╔══════════╗
║  Oleren  ║
╚══════════╝
"""

# inline concat with str
lang :str = "Oleren"
msg := """Hello from """ + lang + """!"""
```

All string forms (`str`, `istr`, `mstr`) share the same methods and operators — `+`, `==`, `.len`, `[]`, `std.str.*`.

String functions live in `std.str`:

```rust
up   := std.str.to_upper(word)
has  := std.str.contains(word, "wor")
st   := std.str.starts_with(word, "He")
en   := std.str.ends_with(word, "ld")
trim := std.str.trim("  pad  ")
s    := @str(42)              # number → str
num  := try @i32("42")        # str → number, fallible
num  := @i32("42") catch 0    # with fallback
```

---

## Arrays

```rust
nums :[]i32 = {1, 2, 3, 4, 5}
n    := nums.len        # i64 length

nums[0] = 99            # index write
v := nums[2]            # index read

# fixed-size array
fixed :[8]f32 = undef   # 8 floats, uninitialized

# immutable elements
data :[]imu u8 = {1, 0, 1, 1}
# data[0] = 0   # compile error

# 'len' is reserved — structs cannot have a field named len
```

---

## Lists (`@ls`)

`@ls(T)` is a growable list. The type is inferred from `@ls.init`.

```rust
nums := @ls.init(i32, 16)   # start cap 16; type is @ls(i32)
defer nums.deinit()          # release memory when done

nums.add(10)
nums.add(20)
nums.add(30)

@pf("len={} cap={}\n", nums.len, nums.cap)

v := nums[0]            # index read
nums[1] = 99            # index write

last := nums.pop()      # remove and return last element
nums.remove(0)          # remove by index
nums.insert(1, 42)      # insert before index

nums.clear()            # empty the list

for n => nums { @pf("{} ", n) }   # range-for works

@pf("type={}\n", @type(nums))     # "@ls(i32)"
```

**Method summary:**

| Method | Description |
|---|---|
| `ls.add(val)` | Append element |
| `ls.pop()` | Remove and return last element |
| `ls.remove(i)` | Erase element at index `i` |
| `ls.insert(i, val)` | Insert before index `i` |
| `ls.clear()` | Empty without releasing memory |
| `ls.deinit()` | Empty and release backing memory |
| `ls.len` | Current length (i64) |
| `ls.cap` | Current capacity (i64) |
| `ls[i]` | Index access (read/write) |

---

## Collections (`@map`, `@set`)

### `@map(K, V)` — hash map

```rust
scores := @map.init(str, i32, 16)   # initial bucket count 16; type is @map(str, i32)
defer scores.deinit()

scores.set("alice", 100)            # insert or update
scores.set("bob", 85)

ok  := scores.has("alice")          # bool
val := scores.get("alice")          # i32 — default V{} if missing
scores.del("bob")
scores.clear()
scores.len                          # i64

# key-value iteration
for k, v => scores { @pf("{}: {}\n", k, v) }

# keys / values as lists
ks := scores.keys()                 # @ls(str)
vs := scores.vals()                 # @ls(i32)

@type(scores)                       # "@map(str, i32)"
```

**Method summary:**

| Method | Description |
|---|---|
| `m.set(k, v)` | Insert or update key |
| `m.get(k)` | Get value; returns `V{}` if missing |
| `m.has(k)` | Check key existence → bool |
| `m.del(k)` | Remove key |
| `m.clear()` | Empty without releasing |
| `m.deinit()` | Empty and release |
| `m.len` | Entry count (i64) |
| `m.keys()` | All keys as `@ls(K)` |
| `m.vals()` | All values as `@ls(V)` |

### `@set(T)` — hash set

```rust
tags := @set.init(str, 8)          # type is @set(str)
defer tags.deinit()

tags.add("fast")
tags.add("fast")                   # duplicate — silently ignored
ok := tags.has("fast")             # bool
tags.del("fast")
tags.clear()
tags.len                           # i64

for t => tags { @pf("{}\n", t) }   # order unspecified

@type(tags)                        # "@set(str)"
```

**Method summary:**

| Method | Description |
|---|---|
| `s.add(v)` | Insert element |
| `s.has(v)` | Test membership → bool |
| `s.del(v)` | Remove element |
| `s.clear()` | Empty without releasing |
| `s.deinit()` | Empty and release |
| `s.len` | Element count (i64) |

---

## Functions

```rust
fn add(a: i32, b: i32) -> i32
{
    ret a + b
}
```

Void functions omit `-> type`:

```rust
fn greet(name: str)
{
    @pl("hello, " + name)
}
```

Functions are top-level only — no lambdas or nested functions.

---

## Structs

Plain data struct (C-style):

```rust
struct Vec2 {
    x: f32,
    y: f32,
}

v := Vec2{.x=1.0, .y=2.0}
v.x = 3.0
```

Struct with static fields and member functions:

```rust
struct Player {
    x: f32, y: f32,
    hp: i32,

    pub MAX_HP : i32 : 100      # immutable static — Player.MAX_HP
    pub TAG    : str = "player" # mutable static   — Player.TAG

    pub fn init(x: f32, y: f32) -> Player
    {
        ret Player{.x=x, .y=y, .hp=Player.MAX_HP}
    }

    pub fn move(self: @self, dx: f32, dy: f32)
    {
        self.x += dx
        self.y += dy
        self.clamp_to_world()   # private helper called from inside
    }

    fn clamp_to_world(self: @self)  # no pub — private to struct
    {
        if self.x < 0.0 { self.x = 0.0 }
        if self.y < 0.0 { self.y = 0.0 }
    }
}

p := Player.init(0.0, 0.0)
p.move(1.0, 0.0)
@pl(Player.MAX_HP)        # 100
@pl(Player.TAG)           # player
# p.clamp_to_world()      # ERROR — private
```

`@self` in a param marks it as the instance receiver — no need to pass it at the call site.

### Method visibility

| Declaration | Access |
|---|---|
| `pub fn name(...)` | Public — callable from anywhere |
| `fn name(...)` | Private — only callable from within the struct's own methods |

Only methods need `pub` / no-`pub`. Data fields are always accessible (structs are transparent data containers). Static fields (`pub NAME : T`) are always public.

### Static fields

`pub` fields are static members of the struct, not per-instance:

```rust
pub NAME : T : val   # immutable static (const) — declared with ::
pub NAME : T = val   # mutable static            — declared with =
```

Both are accessed via `Struct.NAME`. Static fields can hold any type — numbers, strings, arrays. Immutable statics are good for constants; mutable statics are shared across all uses (like C++ `static inline`).

### Anonymous struct init — `.{}`

When the type is known from context, `.{}` expands without repeating the name:

```rust
v :Vec2 = .{.x=1.0, .y=2.0}          # from declaration type
arr :[]Vec2 = { .{.x=0.0, .y=0.0}, .{.x=1.0, .y=1.0} }  # from array element type
fn take(v: Vec2) {}
take(.{.x=0.0, .y=0.0})              # from parameter type
```

---

## Enums

```rust
enum Direction { North, South, East, West }

d :Direction = Direction.North

# typed enum (values are the underlying type)
enum Key => i32 {
    SPACE=44, ENTER=40, ESCAPE=41,
}

k :Key = Key.SPACE
@pl(@i32(k))   # 44
```

---

## Unions

```rust
unn Variant {
    i: i32,
    f: f32,
    s: str,
}

v := Variant{.i=42}
v.f = 3.14

# tagged union — use when for safe dispatch
unn(enum) Shape {
    rect: Rect,
    circle: f32,
}

s := Shape{.circle=5.0}
when s {
    .rect   => @pl("rect"),
    .circle => @pl("circle"),
}
```

---

## Loops

```rust
# for-each over array
for e => nums {
    @pl(e)
}

# for-each with index
for e, i => nums {
    @pl(i)
    @pl(e)
}

# range (exclusive upper bound)
for 0..10 { @pl("tick") }

# range (inclusive)
for 0..=10 { @pl("tick") }

# C-style loop
loop i := 0, i < 10, i += 1 {
    @pl(i)
}

# while
while !done {
    done = check()
}
```

---

## Control Flow

```rust
if x > 0 {
    @pl("positive")
} elif x == 0 {
    @pl("zero")
} else {
    @pl("negative")
}

# if as an expression
label := if score > 100 { "high" } else { "low" }
```

`and`, `or`, `!` are the logical operators (`&&`/`||` are not used):

```rust
if x > 0 and y > 0 { }
if done or failed  { }
if !active         { }
```

`when` is pattern-matched dispatch (like `switch` but exhaustive):

```rust
when direction {
    Direction.North => move_up(),
    Direction.South => move_down(),
    Direction.East  => move_right(),
    Direction.West  => move_left(),
    _ => @panic("unknown direction"),
}

# one-liner arms or blocks
when key {
    Key.SPACE => jump(),
    Key.ENTER => {
        confirm()
        advance()
    },
    _ => {},
}
```

---

## Pointers

Oleren has two pointer kinds: raw (`*T`) and smart (`^T`).

### Raw pointers — `*T`

Raw pointers work like C. You manage the lifetime manually.

```rust
x :i32 = 41
p :*i32 = &x        # & takes address
p.* += 1            # .* dereferences
@pl(x)              # 42

# heap allocation — always pair with defer @free
buf :*i32 = @alo(i32)
defer @free(buf)
buf.* = 99
```

Struct fields through a raw pointer use `->`:

```rust
struct Point { x: i32, y: i32 }

pt :*Point = @alo(Point)
defer @free(pt)
pt->x = 3
pt->y = 4
cp :Point = pt.*    # whole-struct deref copy
```

Raw pointers as function parameters:

```rust
fn bump(n: *i32)
{
    n.* = n.* + 1
}

v :i32 = 10
bump(&v)
@pl(v)   # 11
```

### Smart pointers — `^T`

`^T` is reference-counted (`std::shared_ptr` under the hood) — frees itself when the last handle goes out of scope. No `@free` needed or allowed.

```rust
p :^Point = @alo(Point)   # make_shared; no @free
p->x = 7
p->y = 1

alias :^Point = p         # shared — both handles see the same data
alias->y = 24
@pl(p->y)                 # 24

cp :Point = p.^           # whole-struct deref copy (.^ for smart, .* for raw)
```

### Pointer-to-pointer — `**T`

Multiple `*` levels are supported in types, parameters, and return values:

```rust
x  :i32   = 42
p  :*i32  = &x
pp :**i32 = &p

pp.*.* = 99     # double-deref write
@pl(x)          # 99

fn write_through(pp: **i32, v: i32)
{
    pp.*.* = v
}
```

### Explicit typing rule

All pointer declarations must be explicitly typed — the pointee type cannot be inferred:

```rust
x :i32 = 5
p :*i32 = &x    # correct

y := 3.14
# p := &y       # error — pointee must be explicitly typed
p :*f64 = &y    # correct
```

---

## Error Handling

Oleren uses Zig's error union model. Errors are values — no exceptions, no hidden control flow.

### Error sets

```rust
err FileError { NotFound, PermDenied, ReadFail }
err NetError  { Timeout, Refused, BadAddr }
```

### Return types

```rust
fn foo() -> !i32              # generic — any error
fn open() -> FileError!File   # named set — only FileError variants
fn init() -> !void            # can fail, no success value
```

### Returning errors

```rust
fn validate(x: i32) -> !void
{
    if x < 0    { ret err.InvalidInput }
    if x > 1000 { ret err.OutOfRange }
}

fn open_file(path: str) -> FileError!File
{
    if !fs.exists(path) { ret FileError.NotFound }
    ret fs.open(path)
}
```

### `try` — propagate up

`try expr` returns the error immediately if `expr` failed:

```rust
fn load_config(path: str) -> !Config
{
    data := try read_file(path)
    cfg  := try parse_config(data)
    ret cfg
}
```

### `catch` — handle the error

```rust
count := parse_int(s) catch 0          # fallback value
name  := read_name()  catch "unknown"

data := read_file(path) catch |e| {    # capture + block
    @pl("read failed: " + @str(e))
    ret undef
}

n := parse_int(s) catch { ret err.BadInput }  # block, no capture
```

### `errdefer` — cleanup on error path only

```rust
fn init_system() -> !System
{
    s := try System.alloc()
    errdefer s.free()   # skipped on success; runs on any error below

    try s.load_config()
    try s.connect()
    ret s
}
```

### `main` and top-level errors

```rust
fn main() -> void  { }    # errors must be caught before reaching main
fn main() -> !void { }    # runtime prints any uncaught error and exits non-zero
```

---

## Generics

Oleren has no angle-bracket generics. `any` accepts any type; the compiler monomorphizes a separate instantiation per concrete type.

```rust
fn print_val(x: any)
{
    T :: @type(x)
    when T {
        i64  => @pf("{}\n", x),
        f64  => @pf("{:.4}\n", x),
        str  => @pf("{}\n", x),
        bool => @pf("{}\n", x),
        _    => @pl("unsupported type"),
    }
}

print_val(42)       # T = i64
print_val(3.14)     # T = f64
print_val("hello")  # T = str
```

Multiple `any` params can each be different types at the call site:

```rust
fn eq(a: any, b: any) -> bool
{
    if @type(a) != @type(b) { ret false }
    ret a == b
}
```

`any` in structs fixes the type per instance at init:

```rust
struct Pair { first: any, second: any }

p := Pair{.first=42, .second="hello"}
T :: @type(p.first)   # i64
```

---

## Defer

`defer` runs at the end of the **enclosing block** (not function exit). Multiple defers in the same block run LIFO. The expression is evaluated when it runs, not when it is declared.

```rust
buf :*u8 = @alo(u8)
defer @free(buf)        # runs at end of enclosing block

# LIFO order
{
    a :*Foo = @alo(Foo)
    defer @free(a)      # runs second
    b :*Bar = @alo(Bar)
    defer @free(b)      # runs first
}

# expression evaluated at run time
x :i32 = 1
defer @pl(x)    # prints 2, not 1
x = 2
```

`errdefer` runs only on the error path — see [Error Handling](#error-handling).

---

## Builtins

All builtins use the `@` prefix and are resolved at compile time.

```rust
# ── Output ──────────────────────────────────────────
@pl(val)                    # print line — multiple args: @pl("x = ", x)
@pf("x = %.3f\n", x)        # printf-style format
@cout << val << @endl       # C++ stream style

# ── Input ───────────────────────────────────────────
input := @cin("prompt: ")   # read line from stdin, returns str

# ── Memory ──────────────────────────────────────────
@alo(T)                     # heap alloc for type T → *T
@free(ptr)                  # free raw heap allocation (not for ^T)
@sizeof(T)                  # size of type in bytes → i64
@alignof(T)                 # alignment of type → i64

# ── Types ───────────────────────────────────────────
@type(val)                  # compile-time type of val

# ── Casting ─────────────────────────────────────────
@i32(3.145)                 # f64 → i32 (truncates)
@f32(someInt)               # i64 → f32
@str(42)                    # numeric → str
@hex(255)                   # integer → lowercase hex string ("ff")
@bool(x)                    # any numeric → bool (0 = false)
n := try @i32("42")         # str → i32, fallible
n := @i32("42") catch 0     # with fallback

# ── Collections ─────────────────────────────────────
@ls.init(T, cap)            # create @ls(T) with initial capacity
@map.init(K, V, cap)        # create @map(K, V) with initial bucket count
@set.init(T, cap)           # create @set(T) with initial bucket count

# ── Random ──────────────────────────────────────────
@rng(T, lo, hi)             # random T in [lo, hi] inclusive
n := @rng(i32, 0, 100)
f := @rng(f32, 0.0, 1.0)

# ── Math ────────────────────────────────────────────
@sqrt(x)
@abs(x)
@min(a, b)
@max(a, b)
@clamp(v, lo, hi)

# ── Assertions / Safety ─────────────────────────────
@assert(cond, "msg")        # aborts on failure
@panic("msg")               # immediate abort
@unreachable()              # marks unreachable code paths

# ── Memory ops ──────────────────────────────────────
@memcpy(dst, src, n)
@memset(ptr, val, n)
```

---

## Threading

`std.thread` provides POSIX-style threads, a mutex, and an atomic `i32`. All handles are opaque `i64` values.

### Spawn

Workers take no arguments (`spawn`) or a single `i64` (`spawn_arg`). Pass raw pointers encoded as `i64` to share state:

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

Always call `join` or `detach` — both free the thread handle.

### Mutex

```rust
mtx := std.thread.mutex_new()
defer std.thread.mutex_free(mtx)

std.thread.mutex_lock(mtx)
# ... critical section
std.thread.mutex_unlock(mtx)
```

### Atomic i32

```rust
cnt := std.thread.atomic_new(0)
defer std.thread.atomic_free(cnt)

std.thread.atomic_store(cnt, 5)
val := std.thread.atomic_load(cnt)        # → i32
old := std.thread.atomic_add(cnt, 1)      # fetch-add; returns value before add
won := std.thread.atomic_cas(cnt, 5, 10)  # compare-and-swap → bool
```

### Utility

```rust
std.thread.yield()
id    := std.thread.id()     # → i64
cores := std.thread.cores()  # → i32
```

---

## Malkur

`@std.malkur` is the built-in gamedev library. See [`Malkur.md`](Malkur.md) for the full API.

### Core game loop

```rust
@import ( mk = @std.malkur )

fn main() -> !void
{
    try mk.init_window(1280, 720, "My Game")
    defer mk.close_window()
    mk.set_fps(60)

    while !mk.should_close() {
        dt :: mk.dt()

        mk.begin_draw()
            mk.clear_bg(mk.BLACK)
            # ... draw calls here
        mk.end_draw()
    }
}
```

### Camera 2D

```rust
cam := mk.camera2d(
    mk.vec2(player.x, player.y),   # target (world point to center on)
    mk.vec2(640.0, 360.0),          # offset (screen anchor, usually center)
    1.5,                            # zoom
)

mk.begin_draw()
    mk.clear_bg(mk.BLACK)
    mk.begin_camera2d(cam)
        # draw calls here are in world space
        mk.draw_rect(0.0, 0.0, 100.0, 60.0, mk.GREEN)
    mk.end_camera2d()
    # UI here — screen space, no camera
    mk.draw_text("score: 42", 10.0, 10.0, 16.0, mk.WHITE)
mk.end_draw()

world_pt  := mk.screen_to_world2d(mk.mouse_pos(), cam)
screen_pt := mk.world_to_screen2d(mk.vec2(player.x, player.y), cam)
```

### Gamepad

```rust
if mk.pad_connected(0) {
    if mk.pad_btn_pressed(0, mk.pad_btn.A) { jump() }
    lx :: mk.pad_axis(0, mk.pad_axis.LEFTX)   # -1.0 to 1.0
    lt :: mk.pad_axis(0, mk.pad_axis.LT)       # trigger, 0.0 to 1.0
}
```

### Drawing

```rust
mk.draw_rect(x, y, w, h, mk.RED)
mk.draw_rect_rot(x, y, w, h, mk.vec2(w/2.0, h/2.0), angle, mk.ORANGE)
mk.draw_circle(cx, cy, r, mk.BLUE)
mk.draw_line(x1, y1, x2, y2, 2.0, mk.WHITE)
mk.draw_text("hello", 10.0, 10.0, 16.0, mk.WHITE)

# texture
tex := try mk.load_texture("assets/sprite.png")
defer mk.unload_texture(tex)
mk.draw_texture(tex, x, y, mk.WHITE)

src := mk.rect(0.0, 0.0, 32.0, 32.0)    # source rect in texture
dst := mk.rect(200.0, 100.0, 64.0, 64.0) # destination on screen
mk.draw_texture_rect(tex, src, dst, mk.WHITE)

# color helpers
coral := mk.hex(0xFF7F50FF)             # RRGGBBAA packed u32
faded := mk.color_fade(mk.RED, 0.5)
```

### Audio

```rust
try mk.init_audio()
defer mk.close_audio()

shoot := try mk.load_sound("assets/shoot.wav")
defer mk.unload_sound(shoot)
bgm := try mk.load_music("assets/bgm.ogg")
defer mk.unload_music(bgm)

mk.play_music(bgm, -1)   # -1 = loop forever

# in game loop:
if mk.key_pressed(mk.keys.SPACE) { mk.play_sound(shoot) }
```

---

## Examples

Complete runnable programs for each core feature area.

### Hello World

```rust
fn main() -> void
{
    @pl("Hello, World!")
}
```

### Variables, Types, and Casting

```rust
fn main() -> void
{
    # mutable and immutable
    x :i32 = 10
    y :: 3.14      # immutable f64

    # multi-declaration
    mut i32: a=1, b=2, c=3

    # casting
    f := @f64(x)       # i32 → f64
    s := @str(x)       # i32 → str
    @pl("x = " + s)

    # string → numeric (fallible)
    n := @i32("42") catch -1
    @pl(n)   # 42
}
```

### Functions and Multiple Returns

```rust
fn clamp_score(score: i32, lo: i32, hi: i32) -> i32
{
    if score < lo { ret lo }
    if score > hi { ret hi }
    ret score
}

fn min_max(a: f32, b: f32) -> (f32, f32)
{
    if a < b { ret (a, b) }
    ret (b, a)
}

fn main() -> void
{
    @pl(clamp_score(150, 0, 100))   # 100

    lo, hi := min_max(7.5, 3.2)
    @pl(lo)   # 3.2
    @pl(hi)   # 7.5
}
```

### Structs and Methods

```rust
struct Rect {
    x: f32, y: f32,
    w: f32, h: f32,

    pub UNIT : Rect : .{.x=0.0, .y=0.0, .w=1.0, .h=1.0}  # immutable static

    pub fn new(x: f32, y: f32, w: f32, h: f32) -> Rect
    {
        ret Rect{.x=x, .y=y, .w=w, .h=h}
    }

    pub fn area(self: @self) -> f32
    {
        ret self.w * self.h
    }

    pub fn overlaps(self: @self, other: Rect) -> bool
    {
        ret self.x < other.x + other.w and self.x + self.w > other.x
           and self.y < other.y + other.h and self.y + self.h > other.y
    }
}

fn main() -> void
{
    a := Rect.new(0.0, 0.0, 10.0, 5.0)
    b := Rect.new(8.0, 3.0, 10.0, 5.0)

    @pl(a.area())           # 50
    @pl(a.overlaps(b))      # true
    @pl(Rect.UNIT.w)        # 1
}
```

### Loops and Arrays

```rust
fn main() -> void
{
    nums :[]i32 = {10, 20, 30, 40, 50}

    # sum with for-each
    sum :i32 = 0
    for v => nums { sum += v }
    @pl(sum)   # 150

    # index + element
    for v, i => nums {
        @pl(@str(i) + ": " + @str(v))
    }

    # range loop — FizzBuzz
    for i := 1, i <= 20, i += 1 {
        if i % 15 == 0      { @pl("FizzBuzz") }
        elif i % 3 == 0     { @pl("Fizz") }
        elif i % 5 == 0     { @pl("Buzz") }
        else                { @pl(i) }
    }
}
```

### Pointers — Raw and Smart

```rust
struct Node {
    val:  i32,
    next: *Node,
}

fn bump(p: *i32)
{
    p.* += 1
}

fn main() -> void
{
    # raw pointer — manual lifetime
    x :i32 = 41
    bump(&x)
    @pl(x)   # 42

    # heap allocation — defer handles cleanup
    n :*Node = @alo(Node)
    defer @free(n)
    n->val = 99
    n->next = 0   # null
    @pl(n->val)   # 99

    # smart pointer — reference-counted, no @free
    struct Point { x: f32, y: f32 }

    p :^Point = @alo(Point)
    p->x = 3.0
    p->y = 4.0

    alias :^Point = p     # shared — same object
    alias->x = 10.0
    @pl(p->x)   # 10.0 — change visible through original handle

    cp :Point = p.^     # deref copy (.^ for smart ptrs, .* for raw)
}
```

### Error Handling

```rust
err ParseError { Empty, Invalid, Overflow }

fn parse_positive(s: str) -> ParseError!i32
{
    if s.len == 0 { ret ParseError.Empty }
    n := @i32(s) catch { ret ParseError.Invalid }
    if n < 0     { ret ParseError.Overflow }
    ret n
}

fn run() -> !void
{
    a := try parse_positive("42")
    @pl(a)   # 42

    b := parse_positive("") catch |_| { @i64(0) }
    @pl(b)   # 0

    c := parse_positive("bad") catch |_| { @i64(-1) }
    @pl(c)   # -1
}

fn main() -> !void
{
    try run()
}
```

### Generics with `any`

```rust
fn sum(arr: []any) -> any
{
    T :: @type(arr[0])
    acc :T = 0
    for v => arr { acc += v }
    ret acc
}

fn largest(a: any, b: any) -> any
{
    if a > b { ret a }
    ret b
}

fn main() -> void
{
    ints :[]i32 = {1, 2, 3, 4, 5}
    @pl(sum(ints))              # 15

    @pl(largest(3.14, 2.71))   # 3.14
    @pl(largest(10, 42))       # 42
}
```

### Defer and Resource Cleanup

```rust
@import ( io = @std.io )

fn process_file(path: str) -> !void
{
    f := try io.open(path, IOMode.Read)
    defer io.close(f)          # always runs, even on error below

    buf :*u8 = @alo(u8)
    defer @free(buf)           # LIFO: freed after io.close

    data := try io.read(f, buf, 1024)
    @pl(@str(data.len) + " bytes read")
}

fn main() -> !void
{
    try process_file("data.txt")
}
```

### Malkur — Simple Game

```rust
@import ( mk = @std.malkur )

struct Ball {
    x: f32, y: f32,
    vx: f32, vy: f32,
    r: f32,
}

fn main() -> !void
{
    try mk.init_window(800, 600, "Ball")
    defer mk.close_window()
    mk.set_fps(60)

    ball := Ball{
        .x=400.0, .y=300.0,
        .vx=200.0, .vy=150.0,
        .r=20.0,
    }

    while !mk.should_close() {
        dt :: mk.dt()

        ball.x += ball.vx * dt
        ball.y += ball.vy * dt

        if ball.x - ball.r < 0.0 or ball.x + ball.r > 800.0 { ball.vx = -ball.vx }
        if ball.y - ball.r < 0.0 or ball.y + ball.r > 600.0 { ball.vy = -ball.vy }

        if mk.key_pressed(mk.keys.ESCAPE) { break }

        mk.begin_draw()
            mk.clear_bg(mk.BLACK)
            mk.draw_circle(ball.x, ball.y, ball.r, mk.RED)
            mk.draw_text("ESC to quit", 10.0, 10.0, 16.0, mk.WHITE)
        mk.end_draw()
    }
}
```
