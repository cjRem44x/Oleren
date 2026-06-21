# I/O Builtins

## @pl(values...)

Prints one or more values to stdout followed by a newline.

```olrn
@pl("hello")
@pl(42)
@pl("x =", x)         # space-separated
```

## @pf(fmt, args...)

Interpolated print. `{expr}` in the format string is replaced by the expression value.
`{}` is a positional placeholder filled left-to-right from the remaining arguments.

```olrn
name := "Alice"
@pf("Hello, {name}!\n")

@pf("sum of {} and {} is {}\n", 1, 2, 3)
```

Literal braces: `{{` and `}}`.

## @cout

Raw `std::cout` handle — useful when chaining with `<<` via FFI or inline C++.

```olrn
@cout << "raw " << 42 << "\n"
```

## @cin(prompt) -> str

Reads a line from stdin. Prints `prompt` before blocking (optional).

```olrn
line := @cin("enter name: ")
@pf("you typed: {}\n", line)
```
