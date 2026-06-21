# Operator Precedence

Operators are listed from highest to lowest precedence. Operators on the same row have equal precedence and are left-associative unless noted.

| Precedence | Operators | Description |
|---|---|---|
| 7 (highest) | `-` `!` `*` `&` `^` | Unary: negate, logical not, deref, address-of, smart-deref |
| 6 | `*` `/` `%` | Multiply, divide, remainder |
| 5 | `+` `-` | Add, subtract |
| 4 | `<<` `>>` | Bitwise shift |
| 3 | `<` `>` `<=` `>=` `==` `!=` | Comparison |
| 2 | `&` `^` `\|` | Bitwise AND, XOR, OR |
| 1 | `and` `or` | Logical AND, OR (keyword operators) |
| 0 (lowest) | `=` `:=` `::` `+=` `-=` `*=` `/=` `%=` | Assignment |

## Notes

- `and` and `or` are keywords, not symbols. They compile to `&&` and `||` in C++.
- Unary `*` (deref) and binary `*` (multiply) are disambiguated by position.
- `^` as a unary operator is smart-pointer dereference (postfix: `p.^`). As a binary operator it is bitwise XOR.
- All binary expressions in the emitted C++ are wrapped in parentheses, so operator precedence in generated code is always explicit.

## Assignment operators

```olrn
x = 5
x += 1    # x = x + 1
x -= 1
x *= 2
x /= 2
x %= 3
```

## Examples

```olrn
# precedence example — * binds tighter than +
result := 2 + 3 * 4      # 14, not 20

# explicit grouping with parentheses
result2 := (2 + 3) * 4   # 20

# logical operators
ok := x > 0 and y > 0
bad := a == 0 or b == 0
```
