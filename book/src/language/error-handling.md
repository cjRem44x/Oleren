# Error Handling

Errors are values. No exceptions, no hidden control flow.

## Error sets

```olrn
err FileError { NotFound, PermDenied, TooLarge }
```

## Fallible return types

`!T` means "T or any error". `ErrSet!T` restricts to a declared set:

```olrn
fn read(path: str) -> FileError!str
{
    if path == "" { ret FileError.NotFound }
    ret "data"
}

fn write(path: str, data: str) -> !void
{
    if path == "" { ret err.Wrapped }
}
```

## try — propagate on failure

```olrn
fn process(path: str) -> !void
{
    data := try read(path)   # propagates the error upward on failure
    @pl(data)
}
```

## catch — handle the error inline

```olrn
# fallback value
data := read("file.txt") catch "default"

# inspect the error
data := read("file.txt") catch |e| {
    @pf("error: {}\n", e.msg)
    ret err.Wrapped
}

# branch without value
read("file.txt") catch {
    @pl("read failed, continuing")
}
```

## errdefer — cleanup only on failure

Runs when the enclosing scope exits via an error. Skipped on normal exit.

```olrn
fn load() -> !void
{
    f := io.open("data.txt", IOMode.Read)
    errdefer io.close(f)     # runs only if we return an error below

    data := try io.read_all(f)
    io.close(f)              # normal close path
}
```

## Error sets enforcement

```olrn
err NetError { Timeout, Refused }

fn connect(host: str) -> NetError!void
{
    ret NetError.Timeout     # ok
    # ret FileError.NotFound # compile error — not in NetError
}
```

## main with errors

```olrn
fn main() -> !void
{
    try process("input.txt")
}
```

Uncaught errors print `error: <msg>` to stderr and exit with code 1.

## err.Wrapped

Use `err.Wrapped` to forward an error across error-set boundaries:

```olrn
fn outer() -> !void
{
    inner() catch { ret err.Wrapped }
}
```
