# std.time

Clocks, timestamps, and timers.

```olrn
@import ( std = @std )
t :: @std.time
```

## Current time

```olrn
now  :i64 = t.now_ms()     # milliseconds since Unix epoch
now_s :i64 = t.now_s()     # seconds since Unix epoch
mono  :i64 = t.mono_ms()   # monotonic milliseconds (for measuring elapsed time)
```

## Measuring elapsed time

```olrn
start :i64 = t.mono_ms()

# ... do work ...

elapsed :i64 = t.mono_ms() - start
@pf("took {} ms\n", elapsed)
```

## Sleeping

For millisecond-precision sleep, use the built-in `@sleep(ms)`.

```olrn
@sleep(16)     # ~60 fps
@sleep(1000)   # 1 second
```

## Formatting timestamps

```olrn
ts  :i64 = t.now_s()
fmt :str = t.fmt(ts, "%Y-%m-%d %H:%M:%S")
@pl(fmt)   # e.g. 2026-06-21 14:30:00
```

## Timer helper

```olrn
timer := t.timer_new()
t.timer_start(timer)

# ... do work ...

ms :f64 = t.timer_elapsed_ms(timer)
@pf("elapsed: {}\n", ms)
t.timer_free(timer)
```
