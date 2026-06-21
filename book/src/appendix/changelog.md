# Changelog

## v0.1.6

- **System builtins** — `@exit`, `@cmd`, `@sleep`, `@pid`, `@getenv`, `@args`
- **Expression type model** — sema tracks `OlrnType` for all locals/params; enforces compatibility at var decl, assignment, return, and call args; int out-of-range → error; narrowing → warning
- **Recursive module graph** — transitive imports resolved depth-first; diamond deps deduplicated; import cycles silently broken
- **Source spans in diagnostics** — all errors now show `error:line:col:` with a source excerpt and caret

## v0.1.5

- `else`/`elif` on the next line after `}` (parser gap fixed)
- `draw_orb` scanline health orb in Malkur
- `SDL_WINDOW_RESIZABLE` on all Malkur windows
- Gamepad: `pad_btn_hit`, `pad_btn_press`, `pad_btn_release`, `pad_count`, `pad_name`, `pad_axis_dz`, `pad_rumble`
- Sema warning when `.len`/`.cap` (usize) assigned to a signed integer

## v0.1.0 — v0.1.4

Initial implementation: full language core, Malkur v0.4, Pelentar v0.3,
file-as-module system, `@ls`/`@map`/`@set` collections, multi-return tuples,
`mstr` multiline strings, `olrn view`, struct methods and privacy.
