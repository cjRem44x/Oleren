# The Oleren Language

## Goal

The goal is to provide a more reasonable frontend for the C++ industry standard within Game Dev. A new paint job that simplifies and modernizes old school performance without the headache. This language does NOT plan to support OOP, Classes, or Objects Functions, Structs, and other basic types are included as Oleren aims to provide a more Modern C approach.

## Compiling

The Oleren Project Manager allows for a more modern approach to building code, managing assets, and better project organization.

To create a new oleren project `mkdir myOlrnDir` create a new project directory, `cd myOlrnDir` move into it, then run `olrn init` to create a fresh Oleren Project.

The project structure should look like,
```
/myOlrnDir
    /bin
        # bin build
    /src
        /main
            /olrn
                main.olrn # a pre-made entry
    /olrn_out
        # compile C++ project code
    olrn_pkg.toml # where build assets are managed
    README.md # blank readme
```

To build my Oleren code, I run `olrn build`. If I want to run the binary output I use `olrn run`.

If I want to just build to C++, I run `olrn build-src` which just compile Oleren -> C++. And `olrn build-out` compile C++ -> Oleren bin, which allows for changes to be made to output C++ if needed.

Stand-Alone-Compiler to compile Oleren code directly in a binaries,`olrn sac main.olrn -o=main`

This allows for Oleren code outside of the project manager.
`olrn sac path/*.olrn ../path/*.olrn ... -o=name` allows for multiple paths of source code to be compiled.

## Imports

All imports live in a single `import` block at the top of the file.
Every import is given an alias — that alias is how you access the module.

Builtin libraries use the `@libs.name` path:

```rust
import (
    x   = "file.olrn",          # local file, relative to current file
    y   = "../path/to/file",     # relative path (no extension needed)

    std = @libs.std,             # full standard library
    mk  = @libs.malkur,          # Malkur gamedev library
)
```

`@libs.std` gives you the entire stdlib namespace. Submodules are accessed
via dot notation on the alias: `std.file`, `std.enc`, `std.hash`, etc.

Rules:
- File imports use a quoted path string. The alias is required.
- Builtin/stdlib imports use `@libs.name`. The alias is required.
- The alias is how you call into that module: `mk.init_window(...)`, `std.file.file_rd(...)`
- Only one `import` block per file; it must appear before any declarations.
- Unused imports are a compile error.

```rust
import (
    std = @libs.std,
    mk  = @libs.malkur,
)

fn main() -> !void
{
    win := try mk.init_window(800, 600, "Game")
    defer win.close()

    f := try std.file.file_rd("data.txt")
    defer f.close()
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

Using multi-declaration,

```rust
mut i32: x = 0. y = 44, z = 543 # oneline, multi mutable def

imu f32: pi = 3.14, w = -5443.45 # online, multi immutable def
```

### Arrays

```rust

nums :[]u32 = {1,2,3,4,5}
len :: nums.len
nums[i] = ...

final_arr :[]imu u8 = {1,0,0,1,1} # create an array where the contents are immutable
len :: final_arr.len
final_arr[i] = ... # not allowed

empnums :[N]i32 = undef # `undef` or undefined reps a empty (not null) value, for arrays this create a empty array of N elems
empnums[i] = ...
# empty valuues: 0, 0.0, '0', "0", false
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

chr is our Primitve C Char reps.
name : []chr = "john"
```

We also have the `str` and `istr` types which are more advanced strings handling done with automatic heap alloc, whereas are `chr and []chr` are all manual and primitive.

```rust
name : str = "john" # by init the str value the mem gets alloc
defer @free(free) # str|istr have to be freed

word : str # this is a null pointer that has not reserved any mem
if word.ptr == NULL {...} # .ptr is the internal field that holds the heap pointer.
```

## String VS Chars Arrays

```rust
grade : chr = 'A'

name : []chr = "John" # very primitive C char arr
name[i] = 'B'
len :: name.len
# thats about it, no more. Very primitive, no builtin funcs like str

final_name :[]imu chr = "Jack"
len :: final_name.len
final_name[i] ... # nope! contents are immut

# Chars are stack values, and arrays, that can be alloc if need be.

# str is a wrapper of []chr with modern handling
word: str = "hello" # modern string view handle

# str funcs
_ := word.has("foo") # if contains substr, ret bool
_ := word.starts("foo") # if it starts with substr, ret bool
_ := word.ends("foo") # if ends with substr, ret bool

_ := word.sub(pos, len) # return substr staring at pos and ending at pos+len, ret str

word.add("gh") # append str
word.ins(pos, "gh") # insert substr at pos
word.rep(pos, "gh") # starting at pos, replace existing chars/append if longer then str with substr
word.del(pos, len) # delete (erase) chars starting at pos and ending at pos+len

word.upper() # set chars to uppercase
word.lower() # set chars to lowercase
word.trim() # trim str

word.eql("dfds") # is equal to other str, ret bool

arr := word.spl("regrex") # split on regrex and return []str

word[i] = 'A'
len :: word.len

# and most important
@free(word) # all str's and istr's are heap alloc strings and MUST be freed

imustr: istr = "foo" #@ the value can be cahned as is ais mut, but the string array (contents) is immutable
imustr[i] = 'F' # cannot do this, istr => immut arr, elems are init then const
@free(imustr)
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

## Heap Allocation and Pointers

```rust
# using builtin alo
_ = @alo(<size_in_bytes>) # retuns pointer array to heap

x : *i32 = @alo(i32) # alloc space for i32, alo knows size
defer @free(x) # dfr is defer and it works just like it does in Zig: places instruction0 to end of scope...
x[0].* ...

# ONLY structs can be allocated or pointerd too, dats are prohibited
struct Foo {
    x: i32, y: f32
}

foo : *Foo = @alo(Foo) # just like C, where alo gets size of Foo

# alo always rets an array, this one is just and array of len 1
foo[0]->x = ... # all pointers (Stack|Heap) use -> like C to ref fields (p.*).e
@free(foo)

# ALL pointers and there value assignments must be explicit,

V : type = value
P : *type = &V
P = <new_ref> can be ref or another pointer holding a ref
P.* = <new_value>

x :i32 = 5 # explicit
p_x :*i32 = &x # explicit
# GOOD

y := 3.145 # implicit
p_y :*f32 = &y # explicit
# WRONG

z :str = "Hello"
p_z := &z # pointers MUST always be explicit!

# immutable pointers
PI :f32: 3.145
p_PI :*imu f32 = &PI # this is equ to *const f32, the underlinging value is immutable

p_PI2 :*imu f32: &PI # this is a `const P : *const f32` the value is immut and the pointer is immut

W :[]chr = "word"
p_W :*[]chr: &W # a immut pointer to a mut value

# on the topic of array pointers
nums :[]f32 = ...
p_nums :*[]f32 = &nums
p_nums[i].* = ...

struct Y {z: f32, x: u8, p: *chr}

ys :[]Y = { .{.z=0, .x=0, .p = ...}, .{...}, ...} # the . here expands to known type, in this case Y is the known type
p_ys :*[]Y = &ys

p_ys[i].* = .{...} # set to new Y
p_ys[i]->z ... # access field

p_ys[i]->p.* = ... # set value to pointer field p

# Implicit inference
x := undef OR x :: undef # this, in all cases, is illegal and should fail
                         # undef reqs explicit

# default to largest size and most basic type
# same case for :: immuts
# for nums
x := 0 # always => i64
x := 3.45 # always => f64

x := 'F' # always => chr
x := "Jack" # always => []chr, never imply heap alloc with str|istr

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

    p.* = .{.name="john", .age=23}

    p2 : ^Person = @alo(Person) # no free is needed as this is wrapped in a smart pointer that manaages itself
}
```

## More on Str's
```rust
name : str = "john" # by default `str` is a raw pointer
defer @free(name)

word : ^str = "hello" # by using `^` we are directing the compiler to wrap str with a smart pointer
```

## Console I/O
```rust
fn main() -> void
{
    # Console Builtins
    @pl("hello world") # PrintLine
    @pf("\\{\\}  the value is {x}\n") # PrintF where {} is for formatting and \\{ \\} are literals

    @cout << "Hello World is " << x << @endl # we also offer a C++ stream style

    input := @cin("enter prompt: ") # console input func that takes a string prompt and returns a `str` value
    defer @free(input) # free bec input is `str`

    X :: 3.14546 # => f64
    @pl("X = "+x) # pl is Java styled
    @pf("X = {X}\n") # prints all digits
    @pf("X = {X:.3}\n") # prints only 3 digits, the formatt is `<var>:n.nT`, just lie %0.0T
    @cout << "X = " << x:.3 << @endl
}
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

Three forms for the return type:

```rust
fn foo() -> !type           # generic — any error (anyerror union)
fn foo() -> err.ONEERR!type # specific error value
fn foo() -> MyErrors!type   # all errors in a named set
```

A function that calls another `!T` function must itself return an error
union, unless it handles the error with `catch`.

```rust
fn read_file(path: []chr) -> ![]u8             # any error
fn open(path: []chr) -> FileError!File         # named set
fn get_val() -> err.NotFound!i32               # single specific error
fn init() -> !void                             # can fail, no success value
```

### Returning Errors

Use `err.NAME` to return a value from the global namespace, or
`MyErrors.NAME` to return from a specific set.

```rust
fn validate(x: i32) -> err!void
{
    if x < 0    { ret err.InvalidInput }
    if x > 1000 { ret err.OutOfRange   }
}

fn open_file(path: []chr) -> FileError!File
{
    if !fs.exists(path) { ret FileError.NotFound }
    ret fs.open(path)
}
```

### `try` — Propagate Up the Call Stack

`try expr` evaluates `expr`. If it is an error it is immediately returned
from the current function (which must itself return an error union).

```rust
fn load_config(path: []chr) -> err!Config
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
```

The catch block must either produce a value of the success type or exit the
scope (`ret`, `@panic`, etc.).

### `if` with Error Union Capture

```rust
if open_file(path) |f| {
    defer io.close(f)
    # f is the unwrapped File value
} else |e| {
    @pl("open failed")
}
```

`|f|` captures the success value. `|e|` captures the error.
`else {}` is valid if you want to silently ignore the error.

### `errdefer` — Cleanup on Error Path Only

Runs at scope exit only if the scope exits with an error. LIFO like `defer`.

```rust
fn init_system() -> err!System
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

fn run() -> err!void
{
    _ := try parse_args(1)

    f := io.open("data.bin", .Read) catch |e| {
        ret AppError.ConfigMissing
    }
    defer io.close(f)

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
