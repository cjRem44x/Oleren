# The Oleren Language

## Goal

The goal is to provide a more reasonable frontend for the C++ industry standard within Game Dev. A new paint job that simplifies and modernizes old school performance without the headache. This language does NOT plan to support OOP, Classes, or Objects Functions, Structs, and other basic types are included as Oleren aims to provide a more Modern C approach.

## Compiling

The Oleren Project Manager allows for a more modern approach to building code, managing assets, and better project organization.

To create a new Oleren project, `mkdir myGame` and `cd` into it, then run `olrn init`. It scaffolds inside the current directory, using the directory name as the project name.

The project structure looks like:
```
/myOlrnDir
    /bin
        # compiled binary (bin/<name>, named after the directory)
    /src
        /main
            /olrn
                main.olrn # entry point
    /olrn_out
        # generated C++ output (<name>.cpp)
    olrn_pkg.toml # build config (scaffolded; not read by the build yet)
    README.md
```

`olrn init` scaffolds this tree (never overwriting existing files), and
`olrn build` compiles `src/main/olrn/main.olrn` → `olrn_out/<name>.cpp` →
`bin/<name>`. A bare `main.olrn` in the current directory still works
(flat mode) and builds to `./<name>`.

| Command | Description |
|---|---|
| `olrn build` | Compile `main.olrn` → C++ → binary |
| `olrn run` | Build then run the binary |
| `olrn <file.olrn>` / `olrn emit <file.olrn>` | Emit generated C++ to stdout |
| `olrn build-src <file.olrn>` | Compile Oleren → C++ file (no binary) |
| `olrn build-out <file.cpp>` | Compile existing C++ → binary (skip frontend) |
| `olrn check <file.olrn>` | Parse + semantic checks, no output |
| `olrn sac <files> [-o=name]` | Stand-alone: compile one or more files to binary |
| `olrn --version` / `-V` | Print version |
| `olrn --help` / `-h` | Print usage |

`build-src` + `build-out` are useful together: generate the C++, hand-edit it, then recompile without touching the Oleren source.

`sac` (Stand-Alone Compiler) works outside a project directory and accepts multiple source files:
```sh
olrn sac main.olrn util.olrn -o=myprog
```

## Imports

All imports live in a single `@import` block at the top of the file.
Every import is given an alias — that alias is how you access the module.

The standard library (including builtin libs like Malkur) lives under
`@std`. External dependencies declared in `olrn_pkg.toml` come in under
`@pkg.libname` (planned — pkg manager not built yet).

```rust
@import (
    x   = "file.olrn",       # local file, relative to current file
    y   = "../path/to/file", # relative path (no extension needed)

    std = @std,              # full standard library
    mk  = @std.malkur,       # one stdlib submodule, Zig-style alias
    sdl = @pkg.sdl2,         # external lib from olrn_pkg.toml (planned)
)
```

`@std` gives you the entire stdlib namespace; submodules are accessed
via dot notation on the alias: `std.io`, `std.fs`, `std.math`, etc.
`@std.module` binds a single submodule to the alias.

Submodules can also be bound at the top level, after the `@import` block:

```rust
io :: @std.io
mk :: @std.malkur

f := io.open("data.bin", IOMode.Read)
```

Rules:
- File imports use a quoted path string. The alias is required.
- Stdlib imports use `@std` or `@std.module`; external toml-managed
  libs use `@pkg.libname` (planned). The alias is required.
- The alias is how you call into that module: `mk.init_window(...)`, `std.io.open(...)`
- Only one `@import` block per file; it must appear before any declarations.
- Unused imports are a compile error.

```rust
@import (
    std = @std,
    mk  = @std.malkur,
)

fn main() -> !void
{
    try mk.init_window(800, 600, "Game")
    defer mk.close_window()

    f :File = std.io.open("data.txt", IOMode.Read)
    defer std.io.close(f)
}
```

## Hello World

```rust
fn main() -> void # main func signature
{
    @pl("Hello World") # a builtin for printLn
}
```

## Functions

The signature of a simple return function.

```rust
fn my_func(arg1: type, arg2: type, ...) -> ret_type
{
    ret value
}
```

For voids, or subroutines, the `-> type` is not required.

```rust
fn subrout(args...)
{
    ret # void return is optional
}
```

## Type Aliases

`type` creates a named alias for any existing type.

```rust
type Vec2   = [2]f32
type Color  = u32
type Index  = i32
type Name   = str
type Matrix = [16]f32

# aliases are fully interchangeable with their underlying type
pos  :Vec2  = .{0.0, 0.0}
bg   :Color = 0xFF000000
idx  :Index = 0
```

Aliases can reference other aliases:
```rust
type EntityId = Index
type Velocity = Vec2
```

## Variables

We use a very Golang/Odinlang approach to vars.

```rust
fn main() -> void
{
    VAR : type = value # explicit mutable type
    VAR := value       # implicit mutable type

    VAR :type: value   # explicit immutable type, like const|constexpr|final
    VAR :: value       # impicit immutable type

    x := 32 # this infers to the largest possible signed int, i64
    @pl( @type(x) ) # i64

    x: i32 = 43445 # because we use explicit decl here x is a 32-bit int
}
```

### Single declaration (one variable)

`:T=` and `:=` are for one variable per expression.

```rust
x :i32 = 0       # explicit mutable
x := 0           # implicit mutable (infers i64)

x :i32: 0        # explicit immutable
x :: 0           # implicit immutable (infers i64)
```

### Multi-declaration (multiple variables, same type)

When you need several variables of the same type, use `mut T:` or `imu T:`.
This is one expression defining a group — cleaner than repeating the type.

```rust
mut i32: x=0, y=0, height=0, width=0, vx=0, vy=0

imu f32: pi=3.14, e=2.71, tau=6.28
```

Rule: all variables in the group share the same type and the same
mutability (`mut` = mutable, `imu` = immutable). Each gets its own
initializer. The group is one expression — no `;` needed between vars.

```rust
# real usage
mut i32: x=0, y=0, width=800, height=600
myColor :mk.Color = ...       # single, different type — back to :T= form
```

### Arrays

```rust

nums :[]u32 = {1,2,3,4,5}
len :: nums.len # i64 length — works on []T, [N]T, str
nums[i] = ...

final_arr :[]imu u8 = {1,0,0,1,1} # create an array where the contents are immutable
final_arr[i] = ... # not allowed

# 'len' is reserved: structs cannot declare a field named len

empnums :[N]i32 = undef # `undef` or undefined reps a empty (not null) value, for arrays this create a empty array of N elems
empnums[i] = ...
# empty values: 0, 0.0, "", false
```

This language offers these primitive types:

```rust
Ints
---------------
byte   | i8
ubyte  | u8
short  | i16
ushort | u16
int    | i32
uint   | u32
long   | i64
ulong  | u64

Floats
---------------
float  | f32
double | f64
```

Text is handled by exactly two types — there is no primitive char type
(`chr` and `[]chr` were removed):

- `str`  — managed string, mutable
- `istr` — managed string, immutable contents

## Strings

```rust
# str is a managed string (std::string in the C++ output) — value
# semantics, automatic cleanup, no manual @free, no null states
word :str = "hello"

word = word + " world"         # concatenation with +
same := word == "hello world"  # comparison with ==

# indexing reads/writes single characters; character literals exist
# only for this — there is no standalone char type
c := word[0]        # 'h'
word[0] = 'H'
if word[0] == 'H' { @pl("capitalized") }

# istr — contents are immutable; mutation is a compile error
name :istr = "john"
copy :str = name    # istr -> str copies fine
# name = "jane"     # error
# name[0] = 'J'     # error

# length is the .len property (i64); also on arrays
n := word.len

# string functions live in std.str (and the @str/@i32 cast builtins):
up   := std.str.to_upper(word)
has  := std.str.contains(word, "wor")
st   := std.str.starts_with(word, "He")
en   := std.str.ends_with(word, "ld")
trim := std.str.trim("  pad  ")
s    := @str(42)                     # number -> str
num  := try @i32("42")               # str -> number, fallible (!i32)

# planned (not implemented yet):
#   method-style calls (word.has(...)), sub/ins/rep/del/spl
```

## Loops

```rust
for e => array { # elem in array
    e = ...
}

for e, i => array { # elem and index in array
    e = ...
    array[i] = ...
}

for _, i => array { # just want index
    ...
}

for e => array, 0..N {} # continue until range is < N
# OR
for e => array, 0..=N {} # include upper bound

# simple counting loop
for 0..N { # or 0..=N
}

loop i := 0, i < N, i += 1 {} # traditional loop style

while <condition> {} # basic

while <condition> => my_func() {} # while do's point to (=>) a func call that is called on every loop
```

## Statements

### Logical Operators

`and` and `or` are keywords that compile to `&&` and `||`.
`!` is logical NOT and stays as-is.

```rust
if x > 0 and y > 0 {}
if done or failed {}
if !active {}
if x > 0 and !colliding or y == 0 {}
```

No `&&` / `||` symbol forms — use `and` / `or` exclusively.

```rust
if X {}
elif X and Y or B and X <= Y or X != 0 ... {}
else {}

when X {
    Y => foo +=1, # one line
    Z => { # block of code
        ...block
    },
    4 => func(), # func call

    _ => ... # default
}

```

## Defer

`defer` works identically to Zig.

- Runs at the end of the **enclosing block** (not function exit)
- Multiple defers in the same block run **LIFO** (last declared, first run)
- The expression is **evaluated when it runs** (end of scope), not at declaration
- Both single-expression and block forms are valid
- `errdefer` runs only on the error path — see § Error Handling

```rust
# single expression
defer @free(ptr)
defer std.io.close(f)

# block form
defer {
    conn.flush()
    conn.close()
}

# LIFO order
{
    a := @alo(Foo)
    defer @free(a)     # runs second
    b := @alo(Bar)
    defer @free(b)     # runs first
}                      # @free(b) then @free(a)

# block scope — defer runs when its block ends, not at function exit
fn foo()
{
    {
        buf := @alo(u8)
        defer @free(buf)   # freed here when inner block exits
    }
    # buf is already freed here
}

# expression evaluated at execution time, not declaration
x :i32 = 1
defer @pl(x)    # prints 2, not 1
x = 2
```

## Heap Allocation and Pointers

```rust
# @alo(T) allocates space for one T on the heap and returns a plain *T
x :*i32 = @alo(i32)
defer @free(x)       # raw pointers must be freed; defer is the idiom
x.* = 42             # deref with .*
@pl(x.*)

struct Foo {
    x: i32, y: f32
}

foo :*Foo = @alo(Foo)
defer @free(foo)
foo->x = 1           # pointers use -> to reach fields, like C
foo->y = 2.0
whole :Foo = foo.*   # .* reads/writes the whole pointee

# smart pointers: ^T — reference-counted, frees itself, no @free
p2 :^Foo = @alo(Foo) # compiles to a shared_ptr + make_shared
p2->x = 7
alias :^Foo = p2     # shared: writes through one handle visible in the other

# ALL pointers and their value assignments must be explicit

V : type = value
P : *type = &V       # & takes an address
P.* = <new_value>

x :i32 = 5     # explicit
p_x :*i32 = &x # explicit
# GOOD

y := 3.145     # implicit
p_y :*f32 = &y # explicit
# WRONG — pointee must be explicitly typed

z :str = "Hello"
p_z := &z # pointers MUST always be explicit!

# pointers work through function params too
fn bump(n: *i32) { n.* = n.* + 1 }
bump(&x)

# planned (not implemented yet):
#   immutable pointers (*imu T), pointer-to-array indexing (p_arr[i].*)

# Implicit inference
x := undef OR x :: undef # this, in all cases, is illegal and should fail
                         # undef reqs explicit

# default to largest size and most basic type
# same case for :: immuts
# for nums
x := 0 # always => i64
x := 3.45 # always => f64

x := "Jack" # always => str
x := word[0] # a single character (no nameable type — used with string indexing)

x := false # always => bool

x := {1,2,3,4} # always => []i64
x := {32.1, 343.5, ...} # always => []f64
```

## Generics — `any` and `@type`

Oleren does not use angle-bracket generics. Instead, `any` allows a parameter
to accept any type, and `@type(val)` returns the compile-time type of a value.
The compiler monomorphizes — a separate instantiation is generated for each
unique concrete type the function is called with.

### `any` parameter type

```rust
fn print_val(x: any)
{
    T :: @type(x)     # T is an immutable compile-time type constant
    when T {
        i64    => @pf("{}\n", x),
        f64    => @pf("{:.4}\n", x),
        str    => @pf("{}\n", x),
        bool   => @pf("{}\n", x),
        _      => @pl("unsupported type"),
    }
}

print_val(42)          # T = i64
print_val(3.14)        # T = f64
print_val("hello")     # T = str
```

### Multiple `any` parameters

Each `any` parameter can be a different type at the call site.

```rust
fn eq(a: any, b: any) -> bool
{
    TA :: @type(a)
    TB :: @type(b)
    if TA != TB { ret false }
    ret a == b
}
```

### `any` return type

A function can return `any` when the output type depends on the input.

```rust
fn clamp_val(val: any, lo: any, hi: any) -> any
{
    T :: @type(val)
    when T {
        i32 => ret @clamp(val, lo, hi),
        f32 => ret @clamp(val, lo, hi),
        f64 => ret @clamp(val, lo, hi),
        _   => @panic("clamp_val: unsupported type"),
    }
}
```

### `any` in structs

Struct fields can be `any` for type-flexible containers. The type is fixed
per instance at the point of initialization.

```rust
struct Pair {
    first:  any,
    second: any,
}

p := Pair{.first = 42, .second = "hello"}
T :: @type(p.first)    # i64
```

### `@type` comparison

`@type(x)` values can be compared directly:

```rust
fn same_type(a: any, b: any) -> bool
{
    ret @type(a) == @type(b)
}
```

### Note on compile-time dispatch

Because `when T { ... }` dispatches on a compile-time constant, branches for
unreachable types are stripped at compile time — there is no runtime overhead.
If a `when T` has no `_` default and an unsupported type is passed, it is a
compile error.

```rust
# Basic
enum X {ONE, TWO, THREE, FOUR}

x: X = X.ONE

enum Y => f32 {
    ONE=3.435, TWO=4.53, THREE=-43.53,
}

y: Y = Y.ONE
```

## Unions

```rust
unn X {
    a: i32, b: i32, c: f32,
}

x : X = X{.a = 55}
x.a ...
x.b = 67

unn(enum) Y {
    a: i32 b: i32,
}

y := Y{.a=1}

when y {
    .a => ...,
    .b => ...,
    _ => ...,
}
```

## Structs

```rust
struct Foo {
    a: i32, b: f32, ... # these are instancable data fields

    static_var : type = value # this var is static accessed

    pub fn init(...) { # this is a pub accessable static memeber function
        ...
    }

    pub fn deinit(self: @self) {} # the `var: @self` in param marks this func as a instance member

    fn priv_func(this: @self) {} # this instance func is private to struct
}

foo := Foo.init(...) # bec init is static, it is static acccessed
defer foo.deinit() # the @self does not need to passed, it is inferred as instance `this`
```

If a C styled struct is wanted,

```rust
# just data, no funcs
struct C_Struct {
    var1: type,
    var2: type,
    ...
}

my_struct := C_Struct{.var1=45, .var2=45, ...} # standard means of init data fields in struct
```

### Anonymous Struct Init — `.{}`

Works identically to Zig. When the type is known from context, `.{}`
expands to a full struct init without repeating the type name.
If the type cannot be determined, the compiler emits a warning.

```rust
# type known from declaration
foo :Foo = .{.x=1, .y=2.0}

# type known from array element type
arr :[]Foo = { .{.x=0, .y=0}, .{.x=1, .y=1} }

# type known from function parameter
fn take_foo(f: Foo) {}
take_foo(.{.x=0, .y=0})

# type known from pointer target
p.* = .{.name="john", .age=23}

# no context — compiler warns, use explicit form instead
x := .{.x=1, .y=2}        # warn: type unknown
x := Foo{.x=1, .y=2}      # explicit, always safe
```

Fields are named (`.field=val`), any order, all fields must be specified.

## Expressions
Oleren does not use `;` because expr are only suppose to use one line, unless you have a series of `,` in the declaration.
```rust
x : type = value # this is an expr
y := if W {5} else {6} # this is also a expr
```

However, similar to Go, we have `;` for when multiple exprs are pushed onto the same line.
```rust
x := 5; y :: 3.245; z := "word" # expr; expr; expr
```

Yet it is ideal for code in Oleren to be structured as,
```rust
expr
expr

fn foo() ...
{
    expr
    expr

    if ... {
        expr
        expr
    } else {
        expr
        ...
    }

    ret ...
}
```

## Pointers
A stated above, Oleren utils the C flavor of handling pointers.
`x : *type = ...`

However, `*` is a raw pointer. Oleren also offers Smart Pointers done with `^`.

```rust
fn main() -> void
{
    raw_ptr : *type = ...

    smart_ptr : ^type = ...
}
```

Because Oleren compiles into C++, we have the capability to use both Raw and Smart Pointers. And similarly, smart pointers are managed by runtime.

```rust
struct Person {
    name: str,
    age: i32,
}

fn main() -> void
{
    p : *Person = @alo(Person) # alloc space for Person and return raw pointer
    defer @free(p) # needed for raw ptr

    p.* = .{.name="john", .age=23}  # .{} expands to the known type (Person)

    # prefer ^T for structs with str fields — @alo is malloc, which does
    # not run constructors; make_shared (what ^T = @alo(T) compiles to) does
    p2 : ^Person = @alo(Person) # no free needed — manages itself
}
```

## More on Str's

`str` is a managed string (`std::string` in the C++ output): value
semantics, automatic cleanup, no `@free`, no pointer wrapping needed.
See § String VS Chars Arrays for the function surface (`std.str.*`).

## Console I/O
```rust
fn main() -> void
{
    # Console Builtins
    @pl("hello world")       # PrintLine; multiple args are streamed:
    x :: 3.14546             # => f64
    @pl("X = ", x)           # X = 3.14546

    @pf("X = %.3f\n", x)     # PrintF — C printf formatting (%d %s %.3f ...)

    @cout << "X = " << x << @endl  # C++ stream style

    input := @cin("enter prompt: ") # prompt + read a line; returns str (no free)
}
```

## Builtins

All builtins use the `@` prefix and are resolved at compile time.

```rust
# ── Output ────────────────────────────────────────────
@pl(val)                   # print line; multiple args stream: @pl("x = ", x)
@pf("x = %.3f\n", x)       # printf-style — C format specifiers (%d %s %f)
@cout << val << @endl      # C++ stream style

# ── Input ─────────────────────────────────────────────
input := @cin("prompt: ")  # read line from stdin, returns str (no free needed)

# ── Memory ────────────────────────────────────────────
@alo(T)                    # allocate heap space for type T, returns *T
@free(ptr)                 # free heap allocation
@sizeof(T)                 # size of type in bytes -> i64
@alignof(T)                # alignment of type -> i64

# ── Types ─────────────────────────────────────────────
@type(val)                 # compile-time type of val (used with when T {})

# ── Casting ───────────────────────────────────────────
# @T(val) — cast val to type T. Uses the type name as the builtin.
# Numeric-to-numeric always succeeds. String-to-numeric returns !T (use try/catch).

@i32(3.145)                # f64 -> i32  (truncates)         => i32
@f32(someInt)              # i64 -> f32                      => f32
@str(42)                   # numeric -> str                  => str
@str(3.14)                 # f64 -> str                      => str
@bool(x)                   # any numeric -> bool (0 = false) => bool

n  := try @i32("42")       # str -> i32, can fail            => !i32
f  := try @f64("3.14")     # str -> f64, can fail            => !f64
n  := @i32("42") catch 0   # with fallback

# ── Random ────────────────────────────────────────────
@rng(T, low, high)         # random value of type T in range [low, high] inclusive

n  := @rng(i32, 0, 100)    # random i32 from 0 to 100
f  := @rng(f32, 0.0, 1.0)  # random f32 from 0.0 to 1.0
b  := @rng(i64, -50, 50)   # random i64 from -50 to 50

# ── Assertions / Safety ───────────────────────────────
@assert(cond, "msg")       # assertion — prints and aborts on failure (always on)
@panic("msg")              # immediate abort with message
@unreachable()             # marks a code path that must never be reached

# ── Math ──────────────────────────────────────────────
@sqrt(x)                   # square root
@abs(x)                    # absolute value
@min(a, b)                 # minimum of two values
@max(a, b)                 # maximum of two values
@clamp(v, lo, hi)          # clamp v to [lo, hi]

# ── Memory ops ────────────────────────────────────────
@memcpy(dst, src, n)       # copy n bytes from src to dst
@memset(ptr, val, n)       # set n bytes at ptr to val
```

## Error Handling

Oleren uses Zig's error union model. Errors are values, not exceptions.
No runtime overhead, no hidden control flow.

### Error Sets

`err` is both the keyword to define a named error set and the global error
namespace for ad-hoc errors.

```rust
err FileError { NotFound, PermDenied, ReadFail }
err NetError  { Timeout, Refused, BadAddr }
```

### Error Union Return Types

Two forms for the return type:

```rust
fn foo() -> !type          # generic — any error
fn foo() -> MyErrors!type  # all errors in a named set
```

A function that calls another `!T` function must itself return an error
union, unless it handles the error with `catch`.

```rust
fn read_file(path: str) -> ![]u8         # any error
fn open(path: str) -> FileError!File     # named set
fn init() -> !void                         # can fail, no success value
```

### Returning Errors

Use `err.NAME` to return a value from the global namespace, or
`MyErrors.NAME` to return from a specific set.

```rust
fn validate(x: i32) -> !void
{
    if x < 0    { ret err.InvalidInput }
    if x > 1000 { ret err.OutOfRange   }
}

fn open_file(path: str) -> FileError!File
{
    if !fs.exists(path) { ret FileError.NotFound }
    ret fs.open(path)
}
```

### `try` — Propagate Up the Call Stack

`try expr` evaluates `expr`. If it is an error it is immediately returned
from the current function (which must itself return an error union).

```rust
fn load_config(path: str) -> !Config
{
    data := try read_file(path)     # propagate on failure
    cfg  := try parse_config(data)  # propagate on failure
    ret cfg
}
```

`try` desugars to:
```rust
val := expr catch |e| { ret e }
```

### `catch` — Handle the Error

`catch` is a postfix operator on any error union expression.

```rust
# fallback value — used on error, must match the success type
count := parse_int(s) catch 0
name  := read_name()  catch "unknown"

# block with error capture
data := read_file(path) catch |e| {
    @pl("read failed")
    ret undef
}

# capture-less block — when the error value itself doesn't matter
n := parse_int(s) catch { ret err.BadInput }
cleanup() catch { @pl("cleanup failed, ignoring") }
```

The catch block must either produce a value of the success type or exit the
scope (`ret`, `@panic`, etc.). `catch {` always starts a block — an
array-literal fallback needs parens: `catch ({1, 2})`.

### `errdefer` — Cleanup on Error Path Only

Runs at scope exit only if the scope exits with an error. LIFO like `defer`.

```rust
fn init_system() -> !System
{
    s := try System.alloc()
    errdefer s.free()       # skipped on success, runs on any error below

    try s.load_config()
    try s.connect()
    ret s
}
```

### Full Example

```rust
err AppError { ConfigMissing, BadInput }

fn parse_args(argc: i32) -> AppError!i32
{
    if argc < 2 { ret AppError.BadInput }
    ret argc - 1
}

fn run() -> !void
{
    _ := try parse_args(1)

    f := std.io.open("data.bin", .Read) catch |e| {
        ret AppError.ConfigMissing
    }
    defer std.io.close(f)

    @pl("ok")
}

fn main() -> !void
{
    try run()   # error propagates to runtime, which prints it and exits
}
```

### `main` and Top-Level Error Handling

`main` can return either `void` or `!void`.

```rust
fn main() -> void  { ... }   # errors must be caught before reaching main
fn main() -> !void { ... }   # main IS the top-level handler; runtime prints the error and exits
```

When `main` returns `!void`, any uncaught error that reaches it is printed
by the runtime and the process exits with a non-zero code — identical to
Zig's behaviour. This is the recommended form for programs that want clean
top-level error propagation.

```rust
fn main() -> !void
{
    try run()   # if run() fails, error is printed and process exits
}
```
