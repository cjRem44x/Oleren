# Variables

## Declaration

```olrn
x := 42          # mutable, inferred type (i64)
y :i32 = 10      # mutable, explicit type
z :: 3.14        # immutable (constant), inferred (f64)
w :f32 : 1.6     # immutable, explicit type
```

- `:=` — mutable, inferred
- `:T =` — mutable, explicit
- `::` — immutable, inferred
- `:T :` — immutable, explicit

## Reassignment

```olrn
x := 0
x = 5      # ok — x is mutable
z :: 10
z = 20     # error — z is immutable
```

## Shadowing

Variables can be re-declared in inner scopes:

```olrn
x := 1
if true {
    x := "hello"   # new x, shadows outer
    @pl(x)         # hello
}
@pl(x)             # 1
```

## Unused variables

The compiler warns on declared-but-never-read locals.
Use `_` to explicitly discard a value:

```olrn
_ = some_call()
```
