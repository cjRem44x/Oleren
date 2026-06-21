# Loops

## for (range)

```olrn
for i in 0..10 {
    @pf("{}\n", i)     # 0 through 9
}

for i in 0..=10 {
    @pf("{}\n", i)     # 0 through 10 inclusive
}
```

## for (collection iteration)

```olrn
names := @ls(str)
names.add("Alice")
names.add("Bob")

for name => names {
    @pl(name)
}
```

Map iteration:

```olrn
for k, v => my_map {
    @pf("{}: {}\n", k, v)
}
```

## while

```olrn
x := 0
while x < 10 {
    x = x + 1
}
```

## loop (infinite)

```olrn
loop {
    line := @cin()
    if line == "quit" { break }
    @pl(line)
}
```

## break / continue

```olrn
for i in 0..100 {
    if i == 50 { break }
    if i % 2 == 0 { continue }
    @pf("{}\n", i)
}
```

## defer

Runs a statement when the enclosing scope exits, regardless of how it exits:

```olrn
f := io.open("data.txt", IOMode.Read)
defer io.close(f)

# f is closed automatically here
```

## errdefer

Runs only if the scope exits via an error:

```olrn
fn risky() -> !void
{
    f := io.open("data.txt", IOMode.Read)
    errdefer io.close(f)

    try some_fallible_op()
    io.close(f)   # normal close if no error
}
```
