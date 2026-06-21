# Tuples & Multi-Return

Functions can return multiple values via tuples. Compiles to `std::tuple` + C++17 structured bindings.

## Returning multiple values

```olrn
fn min_max(a: i32, b: i32) -> (i32, i32)
{
    if a < b { ret (a, b) }
    ret (b, a)
}
```

## Destructuring

```olrn
lo, hi := min_max(7, 3)
@pf("lo={} hi={}\n", lo, hi)   # lo=3 hi=7
```

Use `::` for immutable bindings:

```olrn
lo, hi :: min_max(1, 9)
```

## Discarding with _

```olrn
_, hi := min_max(1, 9)    # only care about the max
```

## Practical example

```olrn
fn parse_point(s: str) -> (f32, f32, bool)
{
    # ... parse "x,y" format
    ret (1.0, 2.0, true)
}

x, y, ok := parse_point("1.0,2.0")
if !ok { @panic("bad point") }
@pf("point: {} {}\n", x, y)
```

## Tuple size

Oleren supports any number of return values. The compiler generates the corresponding `std::tuple<T1, T2, ...>` type automatically.
