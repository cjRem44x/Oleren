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
defer name.free(); # str|istr have free funcs

word : str; # this is a null pointer that has not reserved any mem
if word.ptr == NULL {...} # .ptr is the internal field that holds the heap pointer.
```
