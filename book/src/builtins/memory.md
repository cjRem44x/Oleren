# Memory Builtins

## @alo(T)

Heap-allocates a value of type `T` and returns a raw pointer `*T`.

```olrn
p :*i32 = @alo(i32)
*p = 42
@free(p)
```

## @free(p)

Frees a raw pointer previously allocated with `@alo`.

```olrn
p :*Node = @alo(Node)
defer @free(p)
```

Only raw pointers (`*T`) can be passed to `@free`. Smart pointers (`^T`) are freed automatically.

## @memcpy(dst, src, n)

Copies `n` bytes from `src` to `dst`.

```olrn
@memcpy(dst_ptr, src_ptr, 64)
```

## @memset(ptr, val, n)

Sets `n` bytes starting at `ptr` to `val`.

```olrn
@memset(buf, 0, 1024)
```

## @sizeof(T) -> i64

Returns the size of type `T` in bytes.

```olrn
sz := @sizeof(i32)     # 4
```

## @alignof(T) -> i64

Returns the alignment of type `T` in bytes.

```olrn
al := @alignof(f64)    # 8
```

## @assert(cond, msg)

Aborts with `msg` if `cond` is false.

```olrn
@assert(x > 0, "x must be positive")
```

## @panic(msg)

Prints `msg` to stderr and aborts immediately.

```olrn
@panic("unreachable state reached")
```

## @unreachable()

Marks a code path that must never execute. Compiles to `__builtin_unreachable()`.

```olrn
when dir {
    Dir.North => move_up(),
    Dir.South => move_down(),
    _         => @unreachable(),
}
```
