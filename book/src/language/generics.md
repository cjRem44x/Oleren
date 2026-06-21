# Generics

Oleren generics use `any` parameters and `when @type(x)` dispatch. The compiler monomorphizes via C++ templates — no boxing, no virtual dispatch.

## Generic functions

```olrn
fn identity(x: any) -> any
{
    ret x
}

n := identity(42)
s := identity("hello")
```

## Type dispatch with when

```olrn
fn describe(x: any) -> str
{
    ret when @type(x) {
        "i32"  => "a 32-bit integer",
        "f64"  => "a double",
        "str"  => "a string",
        _      => "something else",
    }
}

@pl(describe(42))       # a 32-bit integer
@pl(describe(3.14))     # a double
@pl(describe("hi"))     # a string
```

## Generic structs

Structs can hold `any`-typed fields via methods, but field declarations cannot use `any` directly. The idiomatic pattern is to use a method that accepts `any`:

```olrn
struct Stack {
    items: @ls(any),

    pub fn push(val: any)
    {
        @self.items.add(val)
    }

    pub fn pop() -> any
    {
        ret @self.items.pop()
    }
}
```

## Type-aware builtins

`@type(x)` returns the demangled Oleren type name as a `str`:

```olrn
@pl(@type(42))       # i64
@pl(@type(3.14))     # f64
@pl(@type("hi"))     # str
```

## Generic collection iteration

```olrn
fn sum(nums: @ls(i32)) -> i32
{
    total :i32 = 0
    for n => nums { total += n }
    ret total
}
```

## Limitations

- `any` in struct field declarations is rejected by sema (use a concrete type or a typed collection).
- Generic collection element types (`@ls(T)`) are not yet tracked in the type model — element-type mismatches on `add`/`get` are caught by g++ rather than sema.
