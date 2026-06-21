# std.io

File and stream I/O.

```olrn
@import ( std = @std )
io :: @std.io
```

## Opening files

```olrn
f :File = io.open("data.txt", IOMode.Read)
defer io.close(f)
```

| Mode | Description |
|---|---|
| `IOMode.Read` | Read-only |
| `IOMode.Write` | Write, truncate |
| `IOMode.Append` | Write, append |

## Reading

```olrn
line :str = io.readline(f)        # one line (strips \n)
all  :str = io.read_all(f)        # entire file as str
chunk :[]u8 = io.read(f, 1024)    # up to N bytes
```

## Writing

```olrn
io.write(f, data)          # []u8
io.write_str(f, "hello\n") # str
```

## Seek / tell

```olrn
io.seek(f, 0, SeekFrom.Start)    # rewind
io.seek(f, 0, SeekFrom.End)      # go to end
pos :i64 = io.tell(f)
```

| SeekFrom | Description |
|---|---|
| `SeekFrom.Start` | From the beginning |
| `SeekFrom.Current` | From current position |
| `SeekFrom.End` | From the end |

## Flushing and closing

```olrn
io.flush(f)
io.close(f)
```

## Qualified vs flat access

Both forms work:

```olrn
# qualified (via alias)
f := std.io.open("a.txt", IOMode.Read)

# flat (via top-level bind)
io :: @std.io
f  := io.open("a.txt", IOMode.Read)
```
