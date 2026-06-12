# Oleren Standard Library — Core APIs

Naming convention across all stdlib:
- Functions: `snake_case`
- Variables: `camelCase`
- When a func references a camelCase var, the var name keeps its casing: `set_fooBarThing`
- Long names use readable abbreviations: `get_file_wr`, `buf_rd`, `init`, `deinit`

Import the whole stdlib or bind one module; the alias is how you call in.
Qualified calls map to flat function names (`std.io.open` → `io_open`):

```rust
@import (
    std = @std,        # everything: std.io.*, std.fs.*, ...
    io  = @std.io,     # or bind one module: io.open(...)
)
# or a top-level bind, no block needed:
io :: @std.io
```

Implemented today: `std.io`, `std.fs`, `std.time`, `std.math`, `std.mem`,
`std.str`, `std.log`, `std.thread`, and `std.malkur` (see [`Malkur.md`](Malkur.md)).
Everything under **Planned Modules** further down is design spec.

---

## `std.io` — File I/O

```rust
type File = i64
enum IOMode   { Read, Write, ReadWrite, Append }
enum SeekFrom { Start, Current, End }

f :File = std.io.open("data.bin", IOMode.Write)
defer std.io.close(f)

std.io.write(f, data)               # data: []u8
buf  :[]u8 = std.io.read(f, n)      # read n bytes
line :str  = std.io.readline(f)
std.io.seek(f, pos, SeekFrom.Start)
pos := std.io.tell(f)               # -> i64
done := std.io.eof(f)               # -> bool
```

---

## `std.fs` — Filesystem

```rust
std.fs.exists(path: str)        -> bool
std.fs.is_dir(path: str)        -> bool
std.fs.is_file(path: str)       -> bool
std.fs.size(path: str)          -> i64    # -1 if missing
std.fs.mkdir(path: str)         -> bool   # creates parents too
std.fs.rm(path: str)            -> bool
std.fs.rm_all(path: str)        -> i64    # recursive; count removed
std.fs.ls(path: str)            -> []str  # sorted names
std.fs.rename(from: str, to: str) -> bool
std.fs.copy(from: str, to: str)   -> bool
std.fs.cwd()                    -> str
```

---

## `std.time` — Timing

```rust
now  := std.time.now()          # -> i64, nanoseconds since epoch
mono := std.time.mono()         # -> i64, monotonic ns (intervals, not wall time)
std.time.sleep(secs: f64)
dt   := std.time.since(t0)      # -> f64 seconds since an earlier mono/now stamp

# simple stopwatch
sw := std.time.sw_start()
# ... do work
dt := std.time.sw_elapsed(sw)   # -> f64 seconds

# conversions
std.time.ns_to_ms(ns: i64)  -> f64
std.time.ns_to_sec(ns: i64) -> f64
```

---

## `std.math` — Math

Constants: `std.math.PI`, `std.math.TAU`, `std.math.E`. All functions take
and return `f64` — cast at the edges (`@f64(x)` / `@f32(y)`).

```rust
# core
sqrt  abs  pow  log  log2  exp
# trig
sin  cos  tan  asin  acos  atan  atan2(y, x)
# rounding
floor  ceil  round  sign
# ranges
clamp(v, lo, hi)  lerp(a, b, t)  min(a, b)  max(a, b)
# angles
deg_to_rad(deg)  rad_to_deg(rad)
```

Called as `std.math.sqrt(2.0)` or via a bound alias (`m :: @std.math` →
`m.sqrt(2.0)`).

---

## `std.mem` — Memory

```rust
std.mem.copy(dst: *u8, src: *u8, n: i64)
std.mem.set(ptr: *u8, val: u8, n: i64)
std.mem.zero(ptr: *u8, n: i64)

# size helpers — bytes for n KB/MB/GB
std.mem.kb(n: i64) -> i64
std.mem.mb(n: i64) -> i64
std.mem.gb(n: i64) -> i64
```

---

## `std.str` — Strings

`str` is a managed string (C++ `std::string` under the hood) — concatenate
with `+`, compare with `==`, index with `[i]`, length via the `.len`
property. No manual free. `istr` is the immutable-contents variant.

```rust
std.str.len(s: str)                    -> i64   # or just s.len
std.str.from_int(n: i64)               -> str
std.str.from_f64(x: f64)               -> str
std.str.parse_int(s: str)              -> i64   # 0 on bad input; see also try @i32(s)
std.str.parse_f64(s: str)              -> f64
std.str.to_upper(s: str)               -> str
std.str.to_lower(s: str)               -> str
std.str.trim(s: str)                   -> str
std.str.starts_with(s: str, prefix: str) -> bool
std.str.ends_with(s: str, suffix: str)   -> bool
std.str.contains(s: str, sub: str)       -> bool
```

For fallible parsing prefer the cast builtins: `n := try @i32(s)` returns
`!i32` and rejects trailing garbage.

---

## `std.log` — Logging

Five level-tagged printers; `fatal` logs then panics.

```rust
std.log.debug(msg: str)    # [DEBUG] msg
std.log.info(msg: str)     # [INFO]  msg
std.log.warn(msg: str)     # [WARN]  msg
std.log.error(msg: str)    # [ERROR] msg
std.log.fatal(msg: str)    # [FATAL] msg, then @panic
```

Level filtering, file output, and color (`set_level`, `set_output`,
`set_color`) are planned.

---

## `std.thread` — Threading

Handles are opaque `i64` values (heap-allocated C++ objects). Always `join`
or `detach` every spawned thread; always `mutex_free` / `atomic_free` when done.

```rust
# ── thread lifecycle ────────────────────────────────────────────
t := std.thread.spawn(my_fn)           # fn my_fn() worker
t := std.thread.spawn_arg(my_fn, ptr)  # fn my_fn(arg: i64) worker

std.thread.join(t)      # block until done; frees handle
std.thread.detach(t)    # run independently; frees handle
std.thread.yield()      # hint to scheduler
std.thread.id()   -> i64   # hashed id of the calling thread
std.thread.cores() -> i32  # hardware_concurrency

# ── mutex ────────────────────────────────────────────────────────
mtx := std.thread.mutex_new()
defer std.thread.mutex_free(mtx)

std.thread.mutex_lock(mtx)
std.thread.mutex_unlock(mtx)

# ── atomic i32 ───────────────────────────────────────────────────
cnt := std.thread.atomic_new(0)
defer std.thread.atomic_free(cnt)

std.thread.atomic_store(cnt, 1)
val := std.thread.atomic_load(cnt)        # -> i32
old := std.thread.atomic_add(cnt, 1)      # fetch-add; returns prev value
won := std.thread.atomic_cas(cnt, 0, 1)   # compare-and-swap; -> bool
```

Worker functions take no args (`spawn`) or a single `i64` (`spawn_arg`).
Pass a raw pointer cast to `i64` to share state between threads.

```rust
fn loader(state: i64)
{
    # cast back: *MyState = @cast(*MyState, state)
    std.thread.mutex_lock(state)
    # ... load assets
    std.thread.mutex_unlock(state)
}

fn main() -> void
{
    mtx := std.thread.mutex_new()
    defer std.thread.mutex_free(mtx)
    t := std.thread.spawn_arg(loader, mtx)
    std.thread.join(t)
}
```

---

## `std.malkur` — Gamedev

See [`Malkur.md`](Malkur.md) for the full surface.

**v0.2 (current):** window/core loop, keyboard, mouse, gamepad (4 SDL
GameController slots, hotplug), 2D shapes, `draw_rect_rot`, textures (BMP +
src/dst subrect via `draw_texture_rect`), camera 2D (pan/zoom, all draw calls
transformed; `begin_camera2d` / `end_camera2d` / `screen_to_world2d` /
`world_to_screen2d`), embedded 8×8 bitmap font (`draw_text` / `measure_text`),
colors + `hex(u32)`, Vec2 math, 2D collision. SDL2 backend; `olrn deps` or
`olrn build` auto-resolves `-lSDL2`.

---
---

# Planned Modules (design spec — not yet implemented)

Everything below is the design target for future versions. APIs may change
when they land.

## `std.io` extensions — Buffered Readers/Writers

Four core types: `file_wr` and `file_rd` for text, `byte_wr` and `byte_rd`
for binary. Open the one you need directly — no raw handle required.

### `file_wr` — Text File Writer

```rust
wr := try io.file_wr("out.txt")        # create / overwrite
wr := try io.file_wr_app("out.txt")    # append mode
defer wr.close()

try wr.write("hello ")                 # write string (no newline)
try wr.write_ln("world")               # write string + newline
try wr.write_fmt("score: {}\n", score) # formatted write
try wr.flush()                         # flush internal buffer to disk
```

### `file_rd` — Text File Reader

```rust
rd := try io.file_rd("in.txt")
defer rd.close()

line  := try rd.read_ln()    -> str    # read one line, strips newline
text  := try rd.read_all()   -> str    # entire file as one str
lines := try rd.read_lines() -> []str  # entire file as []str

# iterate line by line
while rd.has_ln() {
    line := try rd.read_ln()
    @pl(line)
}
```

### `byte_wr` — Binary Byte Writer

```rust
bwr := try io.byte_wr("data.bin")      # create / overwrite
bwr := try io.byte_wr_app("data.bin")  # append mode
defer bwr.close()

try bwr.write(buf: []u8)               # write raw bytes
try bwr.write_u8(val: u8)              # ... u16/u32/u64, i8..i64, f32/f64
try bwr.flush()

# seek (useful for writing headers after data)
try bwr.seek(pos: i64, .Start)         # .Start  .Cur  .End
pos := bwr.tell()  -> i64
```

### `byte_rd` — Binary Byte Reader

```rust
brd := try io.byte_rd("data.bin")
defer brd.close()

buf := try brd.read(n: i32) -> []u8    # read n bytes
b   := try brd.read_u8()    -> u8      # ... u16/u32/u64, i8..i64, f32/f64

ok  := brd.has_next()  -> bool         # false at EOF
sz  := brd.size()      -> i64
rem := brd.remaining() -> i64          # bytes left to read

try brd.seek(pos: i64, .Start)
pos := brd.tell() -> i64
```

### Whole-file Shortcuts

```rust
bytes := try io.read_bytes("data.bin") -> []u8
text  := try io.read_text("notes.txt") -> str
lines := try io.read_lines("log.txt")  -> []str

try io.write_bytes("data.bin", buf)
try io.write_text("notes.txt", "hello\n")
```

## `std.fs` extensions

```rust
fs.mod_time(path: str)  -> !i64   # unix timestamp
fs.ls_full(path)        -> []FileInfo  # name, size, is_dir, mod_time

struct FileInfo {
    name:     str,
    size:     i64,
    is_dir:   bool,
    modTime:  i64,
}

# Path helpers
fs.path_join(a: str, b: str)  -> str
fs.path_dir(path: str)        -> str   # parent dir
fs.path_base(path: str)       -> str   # filename
fs.path_ext(path: str)        -> str   # extension with dot
fs.path_stem(path: str)       -> str   # filename without ext
```

## `std.hash` — Hashing

Non-cryptographic hashes are for speed (maps, checksums).
Cryptographic hashes are for integrity and security.

```rust
# ── Non-cryptographic ─────────────────────────────────
hash.fnv32(data: []u8)              -> u32
hash.fnv64(data: []u8)              -> u64
hash.xxhash32(data: []u8, seed: u32) -> u32    # fast general purpose
hash.xxhash64(data: []u8, seed: u64) -> u64
hash.murmur32(data: []u8, seed: u32) -> u32
hash.crc32(data: []u8)              -> u32     # error detection

# ── Cryptographic ─────────────────────────────────────
hash.sha256(data: []u8)  -> [32]u8
hash.sha512(data: []u8)  -> [64]u8
hash.sha1(data: []u8)    -> [20]u8             # legacy only
hash.md5(data: []u8)     -> [16]u8             # legacy only

# HMAC (keyed hash for message auth)
hash.hmac_sha256(key: []u8, data: []u8) -> [32]u8
hash.hmac_sha512(key: []u8, data: []u8) -> [64]u8

# ── Streaming (large data / chunked input) ────────────
ctx := hash.sha256_init()
hash.sha256_upd(ctx, chunk: []u8)
result := hash.sha256_fin(ctx)      -> [32]u8

ctx := hash.xxhash64_init(seed: u64)
hash.xxhash64_upd(ctx, chunk: []u8)
result := hash.xxhash64_fin(ctx)    -> u64

# ── Helpers ───────────────────────────────────────────
hash.to_hex(bytes: []u8)   -> str    # [32]u8 -> "a3f9..."
hash.from_hex(s: str)      -> ![]u8

# ── Password hashing (slow by design) ────────────────
hash.bcrypt(password: []u8, cost: i32) -> !str
hash.bcrypt_verify(password: []u8, hashed: str) -> bool
```

## `std.crypto` — Encryption

```rust
# ── Password Hashing (high-level, most common use) ────
secret := try crypto.pkhash(s: str)                 -> !str   # hash a password/string
ok     := try crypto.pkhash_auth(secret: str, s: str) -> !bool  # verify against original

# ── File Encryption ───────────────────────────────────
try crypto.enc_file(path: str, pk: str)   # encrypt file in-place with private key
try crypto.dec_file(path: str, pk: str)   # decrypt file in-place with private key

# ── Random ────────────────────────────────────────────
crypto.rand_bytes(buf: []u8)     # fill with CSPRNG bytes
crypto.rand_u32()  -> u32
crypto.rand_u64()  -> u64

# ── Symmetric: ChaCha20-Poly1305 (preferred) ─────────
# Fast, secure, no timing side-channels. Good for games/media.
key   := crypto.chacha_key_gen()    -> [32]u8
nonce := crypto.chacha_nonce_gen()  -> [12]u8

ct, tag := try crypto.chacha_enc(
    pt:    []u8,
    key:   [32]u8,
    nonce: [12]u8,
    aad:   []u8,    # additional auth data (can be empty)
)
pt := try crypto.chacha_dec(ct, tag, key, nonce, aad)

# ── Symmetric: AES-256-GCM ────────────────────────────
key := crypto.aes_key_gen(256)     -> []u8
iv  := crypto.aes_iv_gen()         -> [12]u8
ct, tag := try crypto.aes_enc(pt, key, iv, aad)
pt       := try crypto.aes_dec(ct, tag, key, iv, aad)

# ── Asymmetric: X25519 key exchange ───────────────────
# Generate a shared secret from two key pairs (DH-style)
privKey, pubKey := crypto.x25519_key_gen()
shared := try crypto.x25519_shared(myPrivKey, theirPubKey) -> [32]u8

# ── Asymmetric: Ed25519 signing ───────────────────────
privKey, pubKey := crypto.ed25519_key_gen()
sig := try crypto.ed25519_sign(data: []u8, privKey)    -> [64]u8
ok  := crypto.ed25519_verify(data: []u8, sig, pubKey)  -> bool

# ── Key derivation ────────────────────────────────────
# Use when stretching a password into a key
key := try crypto.argon2id(
    password: []u8,
    salt:     []u8,       # at least 16 random bytes
    mem_kb:   u32,        # memory cost (e.g. 65536)
    iters:    u32,        # time cost (e.g. 3)
    key_len:  u32,
) -> []u8

key := try crypto.pbkdf2_sha256(
    password: []u8,
    salt:     []u8,
    iters:    i32,
    key_len:  i32,
) -> []u8

# ── Helpers ───────────────────────────────────────────
crypto.secure_zero(buf: []u8)       # zero memory in a way compilers won't remove
crypto.const_eq(a: []u8, b: []u8) -> bool  # timing-safe compare
```

## `std.db` — Databases

SQLite is the primary embedded target. A generic `Conn` interface allows
future drivers (Postgres, MySQL, etc.) to plug in.

```rust
# ── Open / Close ──────────────────────────────────────
conn := try db.open("game.db")          # SQLite file
conn := try db.open(":memory:")         # in-memory SQLite
defer db.close(conn)

# ── Execute (no rows returned) ────────────────────────
try db.exec(conn, "CREATE TABLE IF NOT EXISTS scores (
    id    INTEGER PRIMARY KEY,
    name  TEXT NOT NULL,
    score INTEGER DEFAULT 0
)")

# ── Query ─────────────────────────────────────────────
rows := try db.query(conn, "SELECT name, score FROM scores WHERE score > ?", 500)
defer db.rows_close(rows)

while db.rows_next(rows) {
    name  := db.col_str(rows, 0)
    score := db.col_i32(rows, 1)
    @pf("{}: {}\n", name, score)
}

# column accessors
db.col_i32(rows, idx)   -> i32
db.col_i64(rows, idx)   -> i64
db.col_f64(rows, idx)   -> f64
db.col_str(rows, idx)   -> str
db.col_bytes(rows, idx) -> []u8
db.col_null(rows, idx)  -> bool

# ── Prepared statements ───────────────────────────────
stmt := try db.prepare(conn, "INSERT INTO scores (name, score) VALUES (?, ?)")
defer db.stmt_close(stmt)

try db.stmt_exec(stmt, "Alice", 9999)
try db.stmt_exec(stmt, "Bob",   8500)

rows := try db.stmt_query(stmt, args...)
defer db.rows_close(rows)

# ── Transactions ──────────────────────────────────────
try db.begin_tx(conn)
try db.exec(conn, "UPDATE scores SET score = score + 100 WHERE name = ?", "Alice")
db.commit_tx(conn) catch {
    db.rollback_tx(conn)
    ret err.DbFail
}

# ── Row count / last insert ───────────────────────────
n  := db.rows_affected(conn) -> i64
id := db.last_insert_id(conn) -> i64
```

## `std.thread` extensions — Thread Pool & Sync Primitives

`std.thread` is implemented (see above). Planned additions:

```rust
# ── Thread pool ───────────────────────────────────────
pool := thread.pool_new(8)     # 8 workers; 0 = hardware_concurrency
defer thread.pool_free(pool)

thread.pool_run(pool, task_fn, arg)   # queue one task
thread.pool_wait(pool)                # block until all tasks done

# ── RW mutex ──────────────────────────────────────────
rw := thread.rwmutex_new()
thread.read_lock(rw);   defer thread.read_unlock(rw)
thread.write_lock(rw);  defer thread.write_unlock(rw)
thread.rwmutex_free(rw)

# ── Atomic i64 ────────────────────────────────────────
a := thread.atomic64_new(val: i64)
thread.atomic64_load(a)  -> i64
thread.atomic64_store(a, val: i64)
thread.atomic64_add(a, val: i64) -> i64
thread.atomic64_free(a)

# ── Condition variable ────────────────────────────────
cv := thread.cond_new()
thread.cond_wait(cv, mtx)
thread.cond_signal(cv)
thread.cond_broadcast(cv)
thread.cond_free(cv)
```

## `std.net` — Networking

```rust
# ── TCP ───────────────────────────────────────────────
conn := try net.tcp_connect("127.0.0.1", 8080)
defer net.close(conn)

try net.write(conn, buf)
n, data := try net.read(conn, 4096)

# TCP server
srv := try net.tcp_listen("0.0.0.0", 8080)
defer net.close(srv)

while true {
    client := try net.accept(srv)
    thread.spawn(handle_client, client)
}

# ── UDP ───────────────────────────────────────────────
sock := try net.udp_bind("0.0.0.0", 9000)
defer net.close(sock)

try net.udp_send(sock, buf, "192.168.1.1", 9000)
n, data, fromAddr := try net.udp_recv(sock, 1024)

# ── HTTP (simple) ─────────────────────────────────────
resp := try net.http_get("https://example.com/data.json")
defer net.http_resp_close(resp)

body := try net.http_resp_body(resp)   -> []u8
code := resp.statusCode                -> i32

resp := try net.http_post("https://api.example.com/upload", body, "application/json")

# ── Address helpers ───────────────────────────────────
addr := try net.resolve("example.com")   -> str   # DNS lookup
net.is_ipv4(addr: str)   -> bool
net.is_ipv6(addr: str)   -> bool
```

## `std.col` — Collections

```rust
# ── Dynamic array (Vec) ───────────────────────────────
v := col.vec_init(i32)
defer col.vec_deinit(v)

col.vec_push(v, 42)
col.vec_pop(v)            -> i32
col.vec_get(v, idx: i32)  -> i32
col.vec_set(v, idx, val)
col.vec_len(v)            -> i32
col.vec_cap(v)            -> i32
col.vec_clear(v)
col.vec_reserve(v, n)

for item => v.items {}    # slice access via .items field

# ── HashMap ───────────────────────────────────────────
m := col.map_init(str, i32)    # key type, value type
defer col.map_deinit(m)

col.map_set(m, "score", 9999)
val, ok := col.map_get(m, "score")
col.map_del(m, "score")
col.map_has(m, "score")  -> bool
col.map_len(m)           -> i32
col.map_clear(m)

for k, v => m {}          # iterate key-value pairs

# ── HashSet ───────────────────────────────────────────
s := col.set_init(str)
defer col.set_deinit(s)

col.set_add(s, "tag")
col.set_has(s, "tag")  -> bool
col.set_del(s, "tag")
col.set_len(s)         -> i32

# ── Queue (FIFO) ──────────────────────────────────────
q := col.queue_init(i32)
defer col.queue_deinit(q)

col.queue_push(q, val)
val := col.queue_pop(q)    -> i32
col.queue_peek(q)          -> i32
col.queue_len(q)           -> i32

# ── Stack (LIFO) ──────────────────────────────────────
stk := col.stack_init(i32)
col.stack_push(stk, val)
val := col.stack_pop(stk)
val := col.stack_peek(stk)
```

## `std.ser` — Serialization

```rust
# ── JSON ──────────────────────────────────────────────
json_str := try ser.to_json(my_struct)      -> str
my_struct := try ser.from_json(json_str, MyStruct)

# streaming for large payloads
enc := ser.json_enc_init(file_writer)
try ser.json_enc_begin_obj(enc)
try ser.json_enc_field_str(enc, "name", "Alice")
try ser.json_enc_field_i32(enc, "score", 9999)
try ser.json_enc_end_obj(enc)

dec := ser.json_dec_init(json_str)
name  := try ser.json_dec_str(dec, "name")
score := try ser.json_dec_i32(dec, "score")

# ── Binary (compact, fast — good for game saves) ──────
bytes := try ser.to_bin(my_struct)         -> []u8
my_struct := try ser.from_bin(bytes, MyStruct)

# ── TOML (config files, olrn_pkg.toml) ───────────────
cfg := try ser.toml_load("config.toml")
val := try ser.toml_get_str(cfg, "project.name")
val := try ser.toml_get_i32(cfg, "build.workers")
```
