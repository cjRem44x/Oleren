# Keyword Reference

## Declaration keywords

| Keyword | Usage |
|---|---|
| `fn` | Function declaration |
| `pub fn` | Public function (exported from a module or struct) |
| `struct` | Struct type declaration |
| `enum` | Enum type declaration |
| `unn` | Union type declaration |
| `err` | Error set declaration |
| `type` | Type alias |
| `extern` | Declare an external C function |

## Variable keywords

| Keyword | Usage |
|---|---|
| `:=` | Mutable variable, inferred type |
| `:T =` | Mutable variable, explicit type |
| `::` | Immutable variable, inferred type |
| `:T :` | Immutable variable, explicit type |

## Control flow

| Keyword | Usage |
|---|---|
| `if` | Conditional |
| `elif` | Else-if branch |
| `else` | Else branch |
| `when` | Pattern match / switch |
| `for` | Range or collection iteration |
| `while` | Condition loop |
| `loop` | Infinite loop |
| `break` | Exit a loop |
| `continue` | Skip to next iteration |
| `ret` | Return a value from a function |

## Error handling

| Keyword | Usage |
|---|---|
| `try` | Propagate error on failure |
| `catch` | Handle an error inline |
| `defer` | Run on scope exit (always) |
| `errdefer` | Run on scope exit only on error |

## Import

| Keyword | Usage |
|---|---|
| `@import` | Import block |
| `@std` | Standard library root |

## Built-in specials

| Keyword | Usage |
|---|---|
| `@self` | Current struct instance inside a method |
| `null` | Null pointer sentinel |
| `true` `false` | Boolean literals |
| `_` | Discard (unused variable or tuple slot) |
| `and` `or` | Logical AND / OR |

## Type keywords

| Type | Description |
|---|---|
| `i8` `i16` `i32` `i64` | Signed integers |
| `u8` `u16` `u32` `u64` | Unsigned integers |
| `f32` `f64` | Floating point |
| `bool` | Boolean |
| `str` | Managed string |
| `istr` | Immutable-contents string |
| `mstr` | Multiline string |
| `usize` | Pointer-sized unsigned integer |
| `void` | No value |
| `any` | Generic type parameter |
