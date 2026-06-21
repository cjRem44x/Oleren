# Structs

Structs group related fields into a named type with optional methods and static state.

## Declaration

```olrn
struct Vec2 {
    x: f32,
    y: f32,
}
```

## Instantiation

```olrn
v :Vec2 = Vec2{.x=1.0, .y=2.0}
```

Fields not listed are zero-initialized.

## Field access

```olrn
@pf("{} {}\n", v.x, v.y)
v.x = 10.0
```

## Methods

```olrn
struct Player {
    x:  f32,
    y:  f32,
    hp: i32,

    pub fn move(dx: f32, dy: f32)
    {
        @self.x += dx
        @self.y += dy
    }

    pub fn is_alive() -> bool
    {
        ret @self.hp > 0
    }

    fn clamp_pos()   # private — not callable outside the struct
    {
        if @self.x < 0.0 { @self.x = 0.0 }
        if @self.y < 0.0 { @self.y = 0.0 }
    }
}
```

- `pub fn` — publicly accessible
- bare `fn` — private; only callable from other methods in the same struct
- `@self` — the current instance

```olrn
p :Player = Player{.x=0.0, .y=0.0, .hp=100}
p.move(5.0, 3.0)
@pl(p.is_alive())   # true
```

## Static fields

```olrn
struct Config {
    pub MAX_SPEED : f32 : 500.0   # immutable static constant
    pub version   : i32 = 1       # mutable static
}

@pl(Config.MAX_SPEED)   # 500
Config.version = 2
```

## Nested structs

```olrn
struct Rect {
    pos:  Vec2,
    size: Vec2,
}

r :Rect = Rect{
    .pos  = Vec2{.x=10.0, .y=20.0},
    .size = Vec2{.x=100.0, .y=50.0},
}
@pl(r.pos.x)   # 10
```

## Passing structs

Structs are passed by value. Use a pointer for mutation across a function boundary:

```olrn
fn scale(v: *Vec2, s: f32)
{
    v.x *= s
    v.y *= s
}

scale(&v, 2.0)
```
