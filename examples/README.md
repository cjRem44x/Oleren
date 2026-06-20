# Oleren Examples

Standalone runnable programs. Each covers a core language feature.

## Running

```sh
olrn sac <file.olrn>      # compile to ./a.out
olrn sac <file.olrn> -o=name  # compile to ./name
```

## Examples

| File | What it covers |
|---|---|
| `01_hello.olrn` | `@pl`, `@pf`, basic output |
| `02_variables.olrn` | Mutable/immutable, type inference, arithmetic, casting |
| `03_strings.olrn` | `str` operations, concatenation, number conversions |
| `04_arrays.olrn` | `[]T` and `[N]T`, for-each, range loops, indexed iteration |
| `05_structs.olrn` | Structs, `@self` methods, nested structs, struct literals |
| `06_enums.olrn` | `enum`, typed enum (`=> i32`), `when` dispatch |
| `07_errors.olrn` | `err` sets, `!T` return types, `try`, `catch`, `errdefer` |
| `08_generics.olrn` | `any` params, `@type` dispatch, generic structs |
| `09_pointers.olrn` | Raw `*T` and smart `^T` pointers, `defer @free`, `->` |
| `10_crypto.olrn` | `@std.pelentar`: hashing, AEAD, password hashing, signing |
| `11_lists.olrn` | `@ls(T)` growable list: `add`, `pop`, `insert`, `remove`, `deinit` |
| `12_collections.olrn` | `@map(K, V)` hash map and `@set(T)` hash set |
| `13_mstr.olrn`        | `mstr` multiline strings, `"""..."""` literal syntax |
| `14_gamepad.olrn`     | Gamepad: `pad_connected`, `pad_name`, `btn_hit/press/release`, axes, deadzone, rumble |

## System deps

`10_crypto.olrn` requires libsodium. `14_gamepad.olrn` requires SDL2, SDL2_image, SDL2_mixer, SDL2_ttf (run `olrn deps` for status).

```sh
sudo pacman -S libsodium        # Arch / CachyOS
sudo apt install libsodium-dev  # Debian / Ubuntu
brew install libsodium          # macOS
```
