# std.math

Mathematical functions and constants.

```olrn
@import ( std = @std )
m :: @std.math
```

## Constants

```olrn
pi  :f64 = m.PI     # 3.14159265358979...
tau :f64 = m.TAU    # 2 * PI
e   :f64 = m.E      # 2.71828...
```

## Trigonometry

```olrn
s := m.sin(m.PI / 2.0)    # 1.0
c := m.cos(0.0)            # 1.0
t := m.tan(m.PI / 4.0)    # ~1.0

a := m.asin(1.0)
b := m.acos(1.0)
d := m.atan(1.0)
e := m.atan2(y, x)
```

## Exponential / logarithm

```olrn
p := m.pow(2.0, 10.0)    # 1024.0
r := m.sqrt(16.0)         # 4.0
l := m.log(m.E)           # 1.0
l2 := m.log2(8.0)         # 3.0
l10 := m.log10(100.0)     # 2.0
```

## Rounding

```olrn
fl := m.floor(3.7)    # 3.0
ce := m.ceil(3.2)     # 4.0
ro := m.round(3.5)    # 4.0
tr := m.trunc(3.9)    # 3.0
```

## Scalar helpers

```olrn
a   := m.abs(-5.0)           # 5.0 — also @abs for any numeric
mn  := m.min(3.0, 7.0)       # 3.0 — also @min
mx  := m.max(3.0, 7.0)       # 7.0 — also @max
cl  := m.clamp(15.0, 0.0, 10.0)  # 10.0 — also @clamp
sg  := m.sign(-3.0)          # -1.0
```

## Lerp / smoothstep

```olrn
v := m.lerp(0.0, 100.0, 0.25)      # 25.0
s := m.smoothstep(0.0, 1.0, 0.5)   # 0.5
```

## Degrees / radians

```olrn
r := m.to_rad(180.0)    # PI
d := m.to_deg(m.PI)     # 180.0
```
