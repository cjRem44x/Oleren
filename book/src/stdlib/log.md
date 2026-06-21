# std.log

Structured logging with levels and optional file output.

```olrn
@import ( std = @std )
log :: @std.log
```

## Log levels

Messages at or above the current level are printed; below it are suppressed.

```olrn
log.set_level(LogLevel.Debug)   # show everything
log.set_level(LogLevel.Info)    # default
log.set_level(LogLevel.Warn)
log.set_level(LogLevel.Error)
log.set_level(LogLevel.Off)     # silence all
```

## Logging

```olrn
log.debug("loading config")
log.info("server started on port {}", port)
log.warn("disk usage above 90%")
log.error("connection refused: {}", addr)
```

Output format: `[LEVEL] message`

## File output

```olrn
log.set_file("app.log")      # tee to file in addition to stderr
log.set_file("")             # stop file output
```

## Timestamps

```olrn
log.set_timestamps(true)    # prefix each line with HH:MM:SS
```

## Flushing

```olrn
log.flush()    # ensure all buffered output is written
```
