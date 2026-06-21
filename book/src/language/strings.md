# Strings

## str

`str` is a managed string backed by `std::string`. It is mutable and heap-allocated.

```olrn
s :str = "hello"
s += ", world"
@pl(s)           # hello, world
@pl(s.len)       # 12
```

## String literals

Double-quoted literals. Escape sequences follow C convention:

```olrn
tab  := "\t"
nl   := "\n"
quot := "\""
```

## mstr — multiline strings

Triple-quoted literals preserve newlines and backslashes without escaping:

```olrn
poem :mstr = """
roses are red
violets are blue
"""
@pl(poem)
```

`mstr` shares the same methods as `str`.

## Concatenation

```olrn
greeting := "Hello, " + name + "!"
```

## Length

`.len` returns `usize`:

```olrn
n :usize = s.len
```

## Interpolation with @pf

```olrn
name := "Alice"
age  :i32 = 30
@pf("name={} age={}\n", name, age)

# named interpolation
@pf("name={name} age={age}\n")
```

## Converting to/from numbers

```olrn
s := @str(42)           # int -> str
f := @str(3.14)         # float -> str
n := try @i32("42")     # str -> i32, fallible (!i32)
d := try @f64("3.14")   # str -> f64, fallible (!f64)
```

## istr — immutable contents

`istr` is the same underlying type as `str` but signals that the contents should not be modified. Use it for string constants and read-only parameters.

```olrn
greeting :istr = "hello"
```

## std.str

For splitting, trimming, searching, and other string operations see [std.str](../stdlib/str.md).
