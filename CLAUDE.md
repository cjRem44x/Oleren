# Claude Session Brief — Oleren Compiler

You are working on **Oleren**, a custom programming language compiler written in C.
It compiles `.olrn` source files → C++ → native binary via `g++ -std=c++17 -O2`.
The language targets fast media applications (games, audio/video). No GC, no OOP, no runtime.

The companion game project built in Oleren is **Olengard** at `~/Prog/Games/Olengard`.
Both repos are on GitHub under `cjRem44x`.

---

## Quick Build

```sh
make              # builds the olrn compiler binary
./olrn check <file.olrn>   # parse + sema only
./olrn sac <file.olrn> -o=<name>   # standalone compile to binary
./olrn build      # compile project in cwd
```

---

## Compiler Pipeline

```
src/lexer/lexer.c       tokenizer
src/parser/parser.c     recursive descent
src/ast/ast.h + ast.c   AST node definitions
src/sema/check.c        semantic checks
src/codegen/codegen.c   C++ emitter
src/module/resolver.c   import resolver
src/codegen/malkur_impl.h    Malkur SDL2 backend (injected C++ when @std.malkur imported)
src/codegen/stdlib_impl.h    stdlib C++ implementations
stdlib/std/malkur.olrn       Malkur Oleren wrappers (malkur_* → _olrn_mk_* bridge)
```

The naming convention for stdlib: `mk.pad_btn(0, btn)` → codegen strips alias prefix
→ `malkur_pad_btn(0, btn)` → calls `_olrn_mk_pad_btn()` from malkur_impl.h.

---

## Current Version: 0.1.5

### What's implemented and passing

- Full Pratt parser (binary ops, precedence), if/elif/else, when
- Variable declarations (mutable/immutable, explicit/inferred)
- Structs, enums, unions with method privacy
- Error handling — `err`, `!T`, `ErrSet!T`, `try`, `catch`, `errdefer`
- Generics via `any` + `@type()` + `when T { }` dispatch
- `@ls(T)` growable list, `@map(K,V)` hash map, `@set(T)` hash set
- `usize` type → `size_t`; `.len`/`.cap` return usize
- Multi-return tuples, `mstr` multiline strings
- `extern` FFI declarations
- Module system — file-as-module, `pub fn` enforcement, import resolver
- CLI: build, run, check, emit, sac, init, deps, view
- Malkur gamedev library v0.4 (SDL2 backend) — full feature list in Roadmap.md
- Pelentar crypto library (libsodium backend)

### Recently fixed / added

- `else`/`elif` now work on the next line after `}` (was a parser gap — fixed with `skip_newlines()` after then-block in `parse_if_chain`)
- `draw_orb(cx, cy, r, fill, col_full, col_empty)` — scanline health orb with gloss highlight
- `SDL_WINDOW_RESIZABLE` on all Malkur windows
- Gamepad: `pad_btn_hit/press/release`, `pad_count`, `pad_name`, `pad_axis_dz`, `pad_rumble`
- Sema warning when `.len`/`.cap` (usize) is assigned to an explicitly-signed integer variable

---

## Sema Checks (`src/sema/check.c`)

Current checks: call arity, bare `ret` misuse, duplicate top-level fns, `null` restricted
to pointer locals, `@free` pointer kind, err set enforcement, unused imports,
import alias shadowing, `any` in struct fields rejected, `len` as struct field name rejected,
usize-to-signed assignment warning.

The `Check` struct tracks: err sets, fn declarations, import aliases, a scoped symbol table
with ptr_kind (0=non-ptr, 1=raw, 2=smart), and separate `errors`/`warnings` counts.

---

## Known Gaps (see docs/Roadmap.md for full list)

1. **No expression type model** — sema doesn't track types of locals/params; can't catch
   type mismatches, return type compatibility, or array element errors before C++ sees them.
   This is the highest priority next step.
2. **No recursive module graph** — imports are resolved one level deep; transitive imports,
   module alias tables, and cycle detection are not implemented.
3. **No source spans in diagnostics** — errors show line numbers but no column or source excerpt.
4. **`g++ -O2` with C++17 brace-init** produces `-Wnarrowing` warnings when Oleren integer
   literals (i64 by default) go into i32 struct fields, or when float arithmetic involves
   double literals. Not errors, but noisy. Root fix is the type model.

---

## Style Notes

- Fix compiler gaps in the compiler, not with user-code workarounds.
- When writing test Oleren code hits a gap, fix the parser/sema/codegen first.
- Responses should be tight — no trailing summaries the user can read from the diff.

---

## Active Companion Project

`~/Prog/Games/Olengard` — see `~/Prog/Games/Olengard/CLAUDE.md` for full game brief.
The game stress-tests the compiler and drives most feature requests.
