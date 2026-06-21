# std.str

String manipulation utilities.

```olrn
@import ( std = @std )
s :: @std.str
```

## Trimming

```olrn
trimmed := s.trim("  hello  ")        # "hello"
tl      := s.trim_left("  hi")        # "hi"
tr      := s.trim_right("hi  ")       # "hi"
```

## Splitting and joining

```olrn
parts :@ls(str) = s.split("a,b,c", ",")
for p => parts { @pl(p) }
parts.deinit()

joined := s.join(parts, "-")   # "a-b-c"
```

## Searching

```olrn
has  :bool = s.contains("hello world", "world")   # true
idx  :i32  = s.index_of("hello", "ll")           # 2
sidx :i32  = s.last_index_of("abcabc", "bc")     # 4
starts :bool = s.starts_with("hello", "he")      # true
ends   :bool = s.ends_with("hello", "lo")        # true
```

## Replacing and transforming

```olrn
r  := s.replace("hello world", "world", "Oleren")
up := s.upper("hello")    # "HELLO"
lo := s.lower("HELLO")    # "hello"
```

## Padding and repeating

```olrn
padded  := s.pad_left("42", 5, "0")    # "00042"
paddedr := s.pad_right("hi", 5, ".")   # "hi..."
rep     := s.repeat("ab", 3)           # "ababab"
```

## Parsing

```olrn
n := try s.parse_i32("42")      # !i32
f := try s.parse_f64("3.14")    # !f64
```

## Substrings

```olrn
sub := s.slice("hello world", 6, 11)   # "world"
```
