# std.thread

Multithreading — spawn, join, mutex, and atomics.

```olrn
@import ( std = @std )
th :: @std.thread
```

## Spawning threads

```olrn
fn worker()
{
    @pl("running in thread")
}

t := th.spawn(worker)
th.join(t)
```

Pass data with a closure-like pattern using `spawn_arg`:

```olrn
fn worker_arg(n: i64)
{
    @pf("got {}\n", n)
}

t := th.spawn_arg(worker_arg, 42)
th.join(t)
```

## Detaching

```olrn
t := th.spawn(background_task)
th.detach(t)    # fire and forget — do not join
```

## Mutex

```olrn
mu := th.mutex_new()
defer th.mutex_free(mu)

th.lock(mu)
# critical section
th.unlock(mu)
```

## Atomic i32

```olrn
counter := th.atomic_new(0)
defer th.atomic_free(counter)

th.atomic_add(counter, 1)
n :i32 = th.atomic_load(counter)
th.atomic_store(counter, 0)

# compare-and-swap
ok :bool = th.atomic_cas(counter, 0, 1)
```

## Thread ID

```olrn
id :i64 = th.current_id()
```

## Example — parallel work

```olrn
@import ( std = @std )

fn square(n: i64)
{
    @pf("{} ^ 2 = {}\n", n, n * n)
}

fn main() -> void
{
    threads := @ls(Thread)
    for i in 0..4 {
        t := std.thread.spawn_arg(square, @i64(i))
        threads.add(t)
    }
    for t => threads { std.thread.join(t) }
    threads.deinit()
}
```
