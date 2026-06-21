# std.mem

Low-level memory utilities — arenas, alignment, and bulk operations.

```olrn
@import ( std = @std )
mem :: @std.mem
```

## Zeroing memory

```olrn
p :*i32 = @alo(i32)
mem.zero(p, @sizeof(i32))
```

## Copying and moving

```olrn
mem.copy(dst, src, n_bytes)
mem.move(dst, src, n_bytes)   # safe for overlapping regions
```

## Comparing

```olrn
eq :bool = mem.equal(a, b, n_bytes)
```

## Arena allocator

An arena allocates from a fixed-size backing buffer. All allocations are freed at once by destroying the arena. Useful for per-frame or per-request data.

```olrn
arena := mem.arena_new(1024 * 1024)    # 1 MB backing
defer mem.arena_free(arena)

buf :*u8 = mem.arena_alloc(arena, 256)
mem.zero(buf, 256)

mem.arena_reset(arena)    # free all, keep backing buffer
```

## Alignment

```olrn
aligned := mem.align_up(size, 16)    # round up to 16-byte boundary
```
