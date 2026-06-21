# Project Layout

`olrn init <name>` scaffolds:

```
my_project/
  src/
    main/
      olrn/
        main.olrn     ← entry point
  bin/                ← compiled binary lands here
  olrn_out/           ← generated C++ (inspect with olrn emit)
  olrn_pkg.toml       ← project manifest
```

## olrn_pkg.toml

```toml
[project]
name    = "my_project"
version = "0.1.0"
entry   = "src/main/olrn/main.olrn"
```

## Importing local files

Any `.olrn` file can be imported as a module:

```olrn
@import (
    utils = "utils.olrn",
)

fn main() -> void
{
    utils.hello()
}
```

Private functions (bare `fn`) are only accessible within their file.
Public functions (`pub fn`) are accessible via the module alias.
