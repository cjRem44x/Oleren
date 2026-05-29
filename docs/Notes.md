# The Oleren Language

## Goal

The goal is to provide a more reasonable frontend for the C++ industry standard within Game Dev. A new paint job that simplifies and modernizes old school performance without the headache. This language does NOT plan to support OOP, Classes, or Objects; Functions, Structs, and other basic types are included as Oleren aims to provide a more Modern C approach.

## Hello World

```rust
fn main() -> void # main func signature
{
    @pl("Hello World"); # a builtin for printLn
}
```

## Functions

The signature of a simple return function.

```rust
fn my_func(arg1: type, arg2: type, ...) -> ret_type
{
    ret value;
}
```

For voids, or subroutines, the `-> type` is not required.

```rust
fn subrout(args...)
{
    ret; # void return is optional
}
```

## Variables

We use a very Golang/Odinlang approach to vars.

```rust
fn main() -> void
{
    VAR : type = value; # explicit mutable type
    VAR := value;       # implicit mutable type

    VAR :type: value;   # explicit immutable type, like const|constexpr|final
    VAR :: value;       # impicit immutable type

    x := 32; # this infers to the largest possible signed int, i64
    @pl( @type(x) ); # i64

    x: i32 = 43445; # because we use explicit decl here x is a 32-bit int
}
```

Using multi-declaration,

```rust
mut i32: x = 0. y = 44, z = 543; # oneline, multi mutable def

imu f32: pi = 3.14, w = -5443.45; # online, multi immutable def
```

### Arrays

```rust

nums :[]u32 = {1,2,3,4,5};
len :: nums.len;
nums[i] = ...

final_arr :[]imu u8 = {1,0,0,1,1}; # create an array where the contents are immutable
len :: final_arr.len;
final_arr[i] = ... # not allowed

empnums :[N]i32 = undef; # `undef` or undefined reps a empty (not null) value, for arrays this create a empty array of N elems
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
name : []chr = "john";
```

We also have the `str` and `istr` types which are more advanced strings handling done with automatic heap alloc, whereas are `chr and []chr` are all manual and primitive.

```rust
name : str = "john"; # by init the str value the mem gets alloc
defer @free(free); # str|istr have to be freed

word : str; # this is a null pointer that has not reserved any mem
if word.ptr == NULL {...} # .ptr is the internal field that holds the heap pointer.
```

## String VS Chars Arrays

```rust
grade : chr = 'A';

name : []chr = "John"; # very primitive C char arr
name[i] = 'B';
len :: name.len;
# thats about it, no more. Very primitive, no builtin funcs like str

final_name :[]imu chr = "Jack";
len :: final_name.len;
final_name[i] ... # nope! contents are immut

# Chars are stack values, and arrays, that can be alloc if need be.

# str is a wrapper of []chr with modern handling
word: str = "hello"; # modern string view handle

# str funcs
_ := word.has("foo"); # if contains substr, ret bool
_ := word.starts("foo"); # if it starts with substr, ret bool
_ := word.ends("foo"); # if ends with substr, ret bool

_ := word.sub(pos, len) # return substr staring at pos and ending at pos+len, ret str

word.add("gh"); # append str
word.ins(pos, "gh"); # insert substr at pos
word.rep(pos, "gh"); # starting at pos, replace existing chars/append if longer then str with substr
word.del(pos, len); # delete (erase) chars starting at pos and ending at pos+len

word.upper(); # set chars to uppercase
word.lower(); # set chars to lowercase
word.trim(); # trim str

word.eql("dfds"); # is equal to other str, ret bool

arr := word.spl("regrex"); # split on regrex and return []str

word[i] = 'A';
len :: word.len;

# and most important;
@free(word) # all str's and istr's are heap alloc strings and MUST be freed

imustr: istr = "foo"; #@ the value can be cahned as is ais mut, but the string array (contents) is immutable
imustr[i] = 'F' # cannot do this, istr => immut arr, elems are init then const
@free(imustr);
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
_ = @alo(<size_in_bytes>); # retuns pointer array to heap

x : *i32 = @alo(@size(i32)); # alloc space for i32
defer @free(x); # dfr is defer and it works just like it does in Zig: places instruction0 to end of scope...
x[0].* ...

# ONLY structs can be allocated or pointerd too, dats are prohibited
struct Foo {
    x: i32, y: f32
}

foo : *Foo = @alo(Foo); # just like C, where alo gets size of Foo

# alo always rets an array, this one is just and array of len 1
foo[0]->x = ... # all pointers (Stack|Heap) use -> like C to ref fields (p.*).e
@free(foo)

# ALL pointers and there value assignments must be explicit,

V : type = value;
P : *type = &V;
P = <new_ref>; can be ref or another pointer holding a ref
P.* = <new_value>;

x :i32 = 5; # explicit
p_x :*i32 = &x; # explicit
# GOOD

y := 3.145; # implicit
p_y :*f32 = &y; # explicit
# WRONG

z :str = "Hello";
p_z := &z; # pointers MUST always be explicit!

# immutable pointers
PI :f32: 3.145;
p_PI :*imu f32 = &PI; # this is equ to *const f32, the underlinging value is immutable

p_PI2 :*imu f32: &PI; # this is a `const P : *const f32` the value is immut and the pointer is immut

W :[]chr = "word";
p_W :*[]chr: &W; # a immut pointer to a mut value

# on the topic of array pointers
nums :[]f32 = ...
p_nums :*[]f32 = &nums;
p_nums[i].* = ...

struct Y {z: f32, x: u8, p: *chr};

ys :[]Y = { .{.z=0, .x=0, .p = ...}, .{...}, ...}; # the . here expands to known type, in this case Y is the known type
p_ys :*[]Y = &ys;

p_ys[i].* = .{...}; # set to new Y
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

## Enums

```rust
# Basic
enum X {ONE, TWO, THREE, FOUR}

x: X = X.ONE;

enum Y => f32 {
    ONE=3.435, TWO=4.53, THREE=-43.53,
}

y: Y = Y.ONE;
```

## Unions

```rust
unn X {
    a: i32; b: i32; c: f32;
}

x : X = X{.a = 55};
x.a ...
x.b = 67;

unn(enum) Y {
    a: i32; b: i32;
}

y := Y{.a=1};

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

    static_var : type = value; # this var is static accessed

    pub fn init(...) { # this is a pub accessable static memeber function
        ...
    }

    pub fn deinit(self: @self) {} # the `var: @self` in param marks this func as a instance member

    fn priv_func(this: @self) {} # this instance func is private to struct
}

foo := Foo.init(...); # bec init is static, it is static acccessed
defer foo.deinit(); # the @self does not need to passed, it is inferred as instance `this`
```

If a C styled struct is wanted,

```rust
# just data, no funcs
struct C_Struct {
    var1: type,
    var2: type,
    ...
}

my_struct := C_Struct{.var1=45, .var2=45, ...}; # standard means of init data fields in struct
```
