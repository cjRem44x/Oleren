# The Oleren Programming Language

Oleren is a thin, sugared frontend that compiles to C++17 and then to a native binary via `g++`.
It targets fast media applications — games, audio/video pipelines, asset tools — where
you want modern ergonomics without a runtime, garbage collector, or object-oriented overhead.

```olrn
fn main() -> void
{
    name := "world"
    @pf("Hello, {name}!\n")
}
```

## Design goals

- **Close to the metal.** No hidden allocations, no GC pauses, no runtime.
- **Readable by default.** Snake_case, explicit types where they matter, no sigil soup.
- **Batteries for media.** First-class gamedev library (Malkur/SDL2), crypto (Pelentar/libsodium), full stdlib.
- **Fast iteration.** One compiler binary, `olrn build` / `olrn run`, no package server required.

## Status

Current version: **0.1.6**

Oleren is in active development. The language and standard library are stable enough to build
real programs (see [Olengard](https://github.com/cjRem44x/EscapeFromOleren)) but APIs may
change between minor versions until 1.0.

## How to read this book

- **[Getting Started](getting-started/installation.md)** — install the compiler and build your first program.
- **[Language Reference](language/variables.md)** — every language construct with examples.
- **[Builtins](builtins/io.md)** — `@`-prefixed compiler intrinsics.
- **[Standard Library](stdlib/io.md)** — `@std.*` modules.
- **[Malkur](malkur/overview.md)** — the gamedev library.
