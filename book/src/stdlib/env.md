# std.env

`std.env` provides access to the process environment.

## Import

```olrn
@import ( env = @std.env )
```

## Functions

```olrn
fn env_get(name: str) -> str
```

Returns the value of the named environment variable. Returns an empty string if the variable is not set.

```olrn
@import ( env = @std.env, io = @std.io )

fn main() {
    home :: env.env_get("HOME")
    if home == "" {
        io.println("HOME not set")
    } else {
        io.println(home)
    }
}
```

## Checking for presence

Because unset variables return `""`, compare against the empty string:

```olrn
debug :: env.env_get("DEBUG")
if debug != "" {
    io.println("debug mode on")
}
```

## Difference from `@getenv`

The builtin `@getenv("VAR")` does the same thing without an import. Use `std.env` when you want it alongside other stdlib imports for consistency, or when you're storing the result in a typed local:

```olrn
# builtin — no import needed
path :str = @getenv("PATH")

# std.env — explicit import, same result
@import ( env = @std.env )
path :str = env.env_get("PATH")
```
