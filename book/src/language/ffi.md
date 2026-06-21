# FFI — Foreign Function Interface

Oleren can call C functions directly via `extern` declarations. No wrapper library needed.

## Declaring extern functions

```olrn
extern fn SDL_Init(flags: u32) -> i32
extern fn SDL_Quit()
extern fn SDL_GetError() -> str
```

`extern` functions emit a raw C-style declaration in the generated C++. They bypass codegen type-checking and map directly to the C ABI.

## Calling extern functions

```olrn
rc := SDL_Init(0x00000020)   # SDL_INIT_VIDEO
if rc != 0 {
    @pl(SDL_GetError())
    @exit(1)
}
defer SDL_Quit()
```

## Type mapping

| Oleren | C / C++ |
|---|---|
| `i8` `i16` `i32` `i64` | `int8_t` `int16_t` `int32_t` `int64_t` |
| `u8` `u16` `u32` `u64` | `uint8_t` `uint16_t` `uint32_t` `uint64_t` |
| `f32` `f64` | `float` `double` |
| `bool` | `bool` |
| `str` | `std::string` (pass `.c_str()` for `const char*` params via FFI) |
| `*T` | `T*` |
| `void` | `void` |

## Including C headers

Headers are included via the generated C++ preamble. For libraries outside the stdlib, add the include via an `extern` block comment or by ensuring the system library is linked through `olrn deps` / `olrn_pkg.toml`.

## System libraries

System libraries (SDL2, libsodium, etc.) are resolved automatically when you import the corresponding stdlib module (`@std.gdev`, `@std.crypt`). For custom C libraries, link flags can be added to `olrn_pkg.toml` once the package manager is implemented.

## Example — calling libc

```olrn
extern fn getpid() -> i32
extern fn puts(s: str) -> i32

fn main() -> void
{
    puts("hello from libc")
    @pf("pid={}\n", getpid())
}
```

> Note: `@pid()` and `@cmd()` are built-in wrappers for common system calls. Use `extern` for everything else.
