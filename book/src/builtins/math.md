# Math & Casting Builtins

## Numeric casts

`@T(val)` casts `val` to type `T`. All numeric-to-numeric casts always succeed.
String-to-numeric casts return `!T` (fallible).

```olrn
x :i32  = @i32(3.14)        # 3
y :f64  = @f64(42)          # 42.0
s :str  = @str(255)         # "255"
h :str  = @hex(255)         # "ff"
n := try @i32("42")         # !i32 — can fail
```

## @min / @max

```olrn
lo := @min(a, b)
hi := @max(a, b)
```

## @clamp

```olrn
v := @clamp(value, 0, 255)
```

## @sqrt / @abs

```olrn
root := @sqrt(2.0)       # f64
dist := @abs(-5)         # i64
```

## @rng(T, low, high)

Returns a random value of type `T` in the inclusive range `[low, high]`.

```olrn
n := @rng(i32, 1, 6)       # dice roll
f := @rng(f64, 0.0, 1.0)   # random float
```

## @type(val) -> str

Returns the demangled Oleren type name of `val` as a `str`.

```olrn
@pl(@type(42))        # i64
@pl(@type(3.14))      # f64
@pl(@type("hello"))   # str
```

Useful with `when` for generic dispatch:

```olrn
fn show(x: any) -> void
{
    when @type(x) {
        "i32" => @pf("int: {}\n", x),
        "str" => @pf("str: {}\n", x),
        _     => @pl("?"),
    }
}
```

## @hex(v) -> str

Converts an integer to a lowercase hex string.

```olrn
@pl(@hex(255))    # ff
@pl(@hex(0))      # 0
```

## @sizeof / @alignof

See [Memory Builtins](memory.md).
