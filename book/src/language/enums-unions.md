# Enums & Unions

## Enums

Plain enums compile to `enum class` in C++:

```olrn
enum Dir { North, South, East, West }

d :Dir = Dir.North

when d {
    Dir.North => @pl("up"),
    Dir.South => @pl("down"),
    _         => @pl("sideways"),
}
```

### Typed enums

Typed enums (`enum Name => T`) give each variant an associated value:

```olrn
enum Key => i32 {
    UP    = 273,
    DOWN  = 274,
    LEFT  = 276,
    RIGHT = 275,
    ESC   = 27,
}

k :i32 = Key.UP
if k == Key.ESC { @exit(0) }
```

Variants are `static constexpr T` inside a C++ namespace — zero runtime overhead.

## Unions

`unn` declares a C-style union where all fields share the same memory:

```olrn
unn Data {
    as_int:   i32,
    as_float: f32,
    as_bytes: [4]u8,
}

d :Data
d.as_int = 0x3F800000
@pl(d.as_float)   # 1.0
```

### Tagged unions

Pair a `unn` with an `enum` for a discriminated union:

```olrn
enum Shape { Circle, Rect }

unn ShapeData {
    circle: f32,    # radius
    rect:   Vec2,   # width, height
}

struct Entity {
    tag:  Shape,
    data: ShapeData,
}

e :Entity = Entity{.tag=Shape.Circle, .data=ShapeData{.circle=5.0}}

when e.tag {
    Shape.Circle => @pf("circle r={}\n", e.data.circle),
    Shape.Rect   => @pf("rect {}x{}\n", e.data.rect.x, e.data.rect.y),
}
```
