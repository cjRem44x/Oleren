# System & Process Builtins

## @args

Returns command-line arguments (excluding `argv[0]`) as a `[]str`.

```olrn
args := @args
@pf("got {} args\n", args.len)

for a => args {
    @pf("  {}\n", a)
}

if args.len > 0 {
    first := args[0]
    @pf("first: {}\n", first)
}
```

```sh
./myapp foo bar baz
# got 3 args
#   foo
#   bar
#   baz
# first: foo
```

## @exit(code)

Terminates the process immediately with the given exit code.

```olrn
if bad_state {
    @exit(1)
}
```

Any code after `@exit` is unreachable. `defer` blocks do **not** run on `@exit`.

## @cmd(str) -> i32

Runs a shell command via `system()`. Returns the exit code (0 = success).

```olrn
rc := @cmd("mkdir -p /tmp/mydir")
if rc != 0 {
    @panic("mkdir failed")
}

# as a statement (exit code discarded)
@cmd("echo building...")
```

> **Warning:** the command string is passed directly to the shell. Do not
> interpolate untrusted user input.

## @getenv(name) -> str

Reads an environment variable. Returns `""` if the variable is unset.

```olrn
home := @getenv("HOME")
@pf("home={}\n", home)

tok := @getenv("API_TOKEN")
if tok.len == 0 {
    @panic("API_TOKEN not set")
}
```

## @pid() -> i32

Returns the current process ID.

```olrn
pid := @pid()
@pf("running as pid {}\n", pid)
```

## @sleep(ms)

Suspends the current thread for the given number of milliseconds.

```olrn
@sleep(1000)    # sleep 1 second
@sleep(16)      # ~60 fps frame cap
```
