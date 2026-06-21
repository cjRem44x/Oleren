# The Compiler CLI

```sh
olrn <command> [options]
```

| Command | Description |
|---|---|
| `olrn build` | Compile project in cwd (`olrn_pkg.toml` required) |
| `olrn run` | Build then execute |
| `olrn sac <file> -o=<name>` | Standalone compile — single file to binary |
| `olrn check <file>` | Parse + sema only, no codegen |
| `olrn emit <file>` | Write generated C++ to `olrn_out/` |
| `olrn init <name>` | Scaffold a new project |
| `olrn deps` | Print system dependency status |
| `olrn view <file>` | SDL2 viewer for images/GIFs/video |
| `olrn version` | Print compiler version |

## Flags

| Flag | Effect |
|---|---|
| `--release` | Compile with `-O2` (default is `-O0`) |

## Examples

```sh
# single-file compile
olrn sac main.olrn -o=myapp

# project build + run
olrn run

# check for errors only
olrn check src/main/olrn/main.olrn

# inspect generated C++
olrn emit src/main/olrn/main.olrn
cat olrn_out/main.cpp
```
