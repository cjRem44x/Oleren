# Control Flow

## if / elif / else

```olrn
x := 5

if x > 10 {
    @pl("big")
} elif x > 0 {
    @pl("small")
} else {
    @pl("zero or negative")
}
```

`else`/`elif` can follow the closing `}` on the same line or the next:

```olrn
if cond {
    do_something()
}
else {
    do_other()
}
```

## if as an expression

```olrn
label := if x > 0 { "positive" } else { "non-positive" }
```

## when (switch/match)

```olrn
when code {
    200 => @pl("ok"),
    404 => @pl("not found"),
    _   => @pl("other"),
}
```

With a typed enum:

```olrn
when dir {
    Dir.North => move_up(),
    Dir.South => move_down(),
    _         => {},
}
```

## when as an expression

```olrn
msg := when code {
    200 => "ok",
    404 => "not found",
    _   => "unknown",
}
```

## when T (generic dispatch)

```olrn
fn describe(x: any) -> str
{
    ret when @type(x) {
        "i32" => "an integer",
        "str" => "a string",
        _     => "something else",
    }
}
```
