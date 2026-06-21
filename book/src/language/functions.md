# Functions

## Declaration

```olrn
fn add(a: i32, b: i32) -> i32
{
    ret a + b
}
```

Void functions omit the return type or use `-> void`:

```olrn
fn greet(name: str)
{
    @pf("Hello, {name}!\n")
}
```

## Calling

```olrn
result := add(1, 2)
greet("Alice")
```

## Multiple return values

```olrn
fn divmod(a: i32, b: i32) -> (i32, i32)
{
    ret (a / b, a % b)
}

q, r := divmod(10, 3)
@pf("{} remainder {}\n", q, r)
```

Use `_` to discard a return value:

```olrn
q, _ = divmod(10, 3)
```

## Generics

Use `any` for type-generic parameters:

```olrn
fn identity(x: any) -> any
{
    ret x
}

n := identity(42)
s := identity("hi")
```

At the call site, the compiler monomorphizes via C++ templates.

## Forward declarations

Functions can be declared in any order — the compiler does a forward-declaration pass before emitting code.

## pub fn

Functions inside a module file are private by default. Mark them `pub fn` to export:

```olrn
pub fn visible() -> void { }
fn private_helper() -> void { }
```
