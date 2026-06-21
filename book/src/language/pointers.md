# Pointers & Memory

## Raw pointers

`*T` is a raw pointer. Use `&` to take the address of a value, `*` to dereference:

```olrn
x :i32 = 42
p :*i32 = &x
@pl(*p)    # 42
*p = 99
@pl(x)     # 99
```

## Heap allocation

`@alo(T)` allocates a T on the heap and returns `*T`. Always pair with `@free` or `defer @free`:

```olrn
p :*i32 = @alo(i32)
*p = 10
defer @free(p)
```

Only raw pointers can be passed to `@free`.

## Smart pointers

`^T` is a reference-counted smart pointer. Use `@alo` with a smart pointer declaration:

```olrn
s :^i32 = @alo(i32)
s.^ = 42       # postfix .^ dereferences
@pl(s.^)       # 42
```

Smart pointers free themselves when the last reference drops. Do not call `@free` on them.

## Null

`null` is only valid for pointer-typed locals:

```olrn
p :*i32 = null
if p != null {
    @pl(*p)
}
```

## Pointer-to-pointer

Multiple levels of indirection:

```olrn
a :i32  = 1
p :*i32 = &a
q :**i32 = &p
@pl(**q)   # 1
```

## Passing pointers

```olrn
fn increment(p: *i32)
{
    *p += 1
}

n :i32 = 0
increment(&n)
@pl(n)   # 1
```

## memcpy / memset

For raw byte operations see [@memcpy and @memset](../builtins/memory.md).
