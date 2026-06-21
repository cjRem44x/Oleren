# Modules

Every `.olrn` file is a module. Importing it makes its `pub fn` functions accessible under an alias.

## Importing files

```olrn
@import (
    utils = "utils.olrn",
    math  = "lib/vec_math.olrn",
)

fn main() -> void
{
    utils.hello()
    v := math.normalize(Vec2{.x=3.0, .y=4.0})
}
```

Paths are relative to the importing file. Absolute paths and `..` are rejected.

## Importing stdlib

```olrn
@import (
    std = @std,
)

# bind a submodule top-level
io :: @std.io
fs :: @std.fs
```

Or import a single submodule directly:

```olrn
@import (
    mk = @std.gdev,
)
```

## pub fn vs bare fn

```olrn
# utils.olrn
pub fn hello()          # accessible as utils.hello()
{
    @pl("hello from utils")
}

fn _helper()            # private — compile error if called from outside
{
    @pl("internal")
}
```

Bare `fn` is `static` in the emitted C++ namespace — inaccessible from other files.

## Transitive imports

If `world.olrn` imports `shader.olrn`, and you import `world`, the compiler resolves the full graph depth-first. Transitive modules are emitted before their dependents in the generated C++ to ensure correct namespace ordering.

Diamond dependencies (the same file imported by two modules) are deduplicated — parsed and emitted exactly once under the first alias seen.

## Import cycles

Circular imports are silently broken. The first time a file is visited it's added to the resolved set; subsequent imports of the same canonical path are skipped.

## Module aliases in code

```olrn
@import (
    w  = "world.olrn",
    sh = "shader.olrn",
)

w.update()
sh.bind()
```

The alias becomes the C++ namespace name in generated code.
