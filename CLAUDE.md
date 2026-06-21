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
src/codegen/gdev_impl.h    Gdev SDL2 backend (injected C++ when @std.gdev imported)
src/codegen/stdlib_impl.h    stdlib C++ implementations
stdlib/std/gdev.olrn       Gdev Oleren wrappers (gdev_* → _olrn_gd_* bridge)
```

The naming convention for stdlib: `mk.pad_btn(0, btn)` → codegen strips alias prefix
→ `gdev_pad_btn(0, btn)` → calls `_olrn_gd_pad_btn()` from gdev_impl.h.

---

## Current Version: 0.1.6

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
- Module system — file-as-module, `pub fn` enforcement, recursive import resolver
- CLI: build, run, check, emit, sac, init, deps, view
- Gdev gamedev library v0.4 (SDL2 backend) — full feature list in Roadmap.md
- Crypt crypto library (libsodium backend)

### Recently fixed / added (v0.1.6)

- **Source spans** — `Token`/`AstNode` carry `col`; parser shows `error:line:col:` with source excerpt + caret
- **Recursive module graph** — transitive imports resolved depth-first; diamond deps deduplicated; import cycles silently broken; transitive aliases recognized in codegen
- **Expression type model** — `OlrnType` in symbol table; `type_of_expr()` for literals/idents/calls/casts/binary; `check_compat()` at var decl, assignment, return, call args; int out-of-range → error; computed narrowing → warning
- `-Wno-narrowing` passed to g++ (sema now owns all narrowing diagnostics)
- **System builtins** — `@exit(code)`, `@cmd("...") -> i32`, `@getenv("VAR") -> str`, `@pid() -> i32`, `@sleep(ms)`, `@args -> []str` (argv\[1..\]; supports `.len`, `[i]`, `for`)

---

## Sema Checks (`src/sema/check.c`)

Current checks: call arity, bare `ret` misuse, duplicate top-level fns, `null` restricted
to pointer locals, `@free` pointer kind, err set enforcement, unused imports,
import alias shadowing, `any` in struct fields rejected, `len` as struct field name rejected,
usize-to-signed assignment warning, expression type compatibility (var decl, assign, ret, call args).

The `Check` struct tracks: err sets, fn declarations, import aliases, a scoped symbol table
with ptr_kind (0=non-ptr, 1=raw, 2=smart) + `OlrnType sym_otype[]`, and separate `errors`/`warnings` counts.

---

## Known Gaps (see docs/Roadmap.md for full list)

1. **No struct field type registry** — `type_of_expr` returns `TY_UNKNOWN` for field access (`p.x`); a struct name→field→type table would enable field-level checks.
2. **Sema skips module functions** — only top-level `NODE_FN_DECL` nodes are checked; functions inside `NODE_MODULE` wrappers are not walked.
3. **Generic collection element types** — `@ls(T)`, `@map(K,V)`, `@set(T)` element types not tracked in `type_of_expr`.

---

## Style Notes

- Fix compiler gaps in the compiler, not with user-code workarounds.
- When writing test Oleren code hits a gap, fix the parser/sema/codegen first.
- Responses should be tight — no trailing summaries the user can read from the diff.

---

## Active Companion Project

`~/Prog/Games/Olengard` — see `~/Prog/Games/Olengard/CLAUDE.md` for full game brief.
The game stress-tests the compiler and drives most feature requests.
