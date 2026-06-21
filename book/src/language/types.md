# Types

## Primitives

| Type | C++ | Description |
|---|---|---|
| `i8` `i16` `i32` `i64` | `int8_t` … `int64_t` | Signed integers |
| `u8` `u16` `u32` `u64` | `uint8_t` … `uint64_t` | Unsigned integers |
| `f32` `f64` | `float` `double` | Floating point |
| `bool` | `bool` | `true` / `false` |
| `str` | `std::string` | Managed string |
| `istr` | `std::string` | Immutable string contents |
| `usize` | `size_t` | Pointer-sized unsigned (`.len`, `.cap`) |
| `void` | `void` | No value |

## Integer literals

```olrn
dec  := 255
hex  := 0xFF
bin  := 0b11111111
```

## Casting

```olrn
x :i32 = @i32(3.14)       # f64 -> i32
s :str = @str(42)          # int -> str
n := try @i32("42")        # str -> i32, fallible
```

## Type aliases

```olrn
type Pixels = i32
type Name   = str
```

## Arrays

```olrn
fixed  :[]i32  = {1, 2, 3}        # fixed-size (std::array)
sized  :[3]i32 = {1, 2, 3}        # explicit size
imuarr :[]imu i32 = {1, 2, 3}    # immutable elements
```

## usize

`.len` and `.cap` on strings, arrays, and collections return `usize`.
Assigning `usize` to a signed integer produces a warning.

```olrn
s := "hello"
n :usize = s.len      # ok
m :i32   = s.len      # warning: usize -> i32
```
