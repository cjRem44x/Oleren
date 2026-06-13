# Oleren Standard Library

Naming convention across all stdlib:
- Functions: `snake_case`
- Variables / struct fields: `camelCase`
- When a func name references a camelCase variable, the casing is preserved: `set_fooBarThing`
- Long names use readable abbreviations: `get_file_wr`, `buf_rd`, `init`, `deinit`

Import the whole stdlib or bind one module:

```rust
@import (
    std = @std,        # everything: std.io.*, std.fs.*, std.env.*, ...
    io  = @std.io,     # or bind one module directly
)
# top-level bind (no block needed):
io :: @std.io
```

Qualified calls map to flat names — `std.io.open` emits `io_open`.

---

# Core Modules — Implemented

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

## `std.fs` — Filesystem

```rust
std.fs.exists(path: str)          -> bool
std.fs.is_dir(path: str)          -> bool
std.fs.is_file(path: str)         -> bool
std.fs.size(path: str)            -> i64    # -1 if missing
std.fs.mkdir(path: str)           -> bool   # creates parents too
std.fs.rm(path: str)              -> bool
std.fs.rm_all(path: str)          -> i64    # recursive; count removed
std.fs.ls(path: str)              -> []str  # sorted names
std.fs.rename(from: str, to: str) -> bool
std.fs.copy(from: str, to: str)   -> bool
std.fs.cwd()                      -> str
```

## `std.time` — Timing

```rust
now  := std.time.now()          # -> i64, nanoseconds since epoch
mono := std.time.mono()         # -> i64, monotonic ns (for intervals)
std.time.sleep(secs: f64)
dt   := std.time.since(t0)      # -> f64 seconds since an earlier stamp

# stopwatch
sw := std.time.sw_start()
dt := std.time.sw_elapsed(sw)   # -> f64 seconds

# conversions
std.time.ns_to_ms(ns: i64)  -> f64
std.time.ns_to_sec(ns: i64) -> f64
```

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

## `std.mem` — Memory

```rust
std.mem.copy(dst: *u8, src: *u8, n: i64)
std.mem.set(ptr: *u8, val: u8, n: i64)
std.mem.zero(ptr: *u8, n: i64)

std.mem.kb(n: i64) -> i64
std.mem.mb(n: i64) -> i64
std.mem.gb(n: i64) -> i64
```

## `std.str` — Strings

`str` is a managed string (`std::string` under the hood). Concatenate with `+`,
compare with `==`, index with `[i]`, length via `.len`. No manual free.

```rust
std.str.from_int(n: i64)               -> str
std.str.from_f64(x: f64)               -> str
std.str.parse_int(s: str)              -> i64   # 0 on bad input
std.str.parse_f64(s: str)              -> f64
std.str.to_upper(s: str)               -> str
std.str.to_lower(s: str)               -> str
std.str.trim(s: str)                   -> str
std.str.starts_with(s: str, prefix: str) -> bool
std.str.ends_with(s: str, suffix: str)   -> bool
std.str.contains(s: str, sub: str)       -> bool
```

Prefer the cast builtins for fallible parsing: `n := try @i32(s)` returns `!i32`.

## `std.log` — Logging

Five level-tagged printers; `fatal` logs then panics.

```rust
std.log.debug(msg: str)    # [DEBUG] msg
std.log.info(msg: str)     # [INFO]  msg
std.log.warn(msg: str)     # [WARN]  msg
std.log.error(msg: str)    # [ERROR] msg
std.log.fatal(msg: str)    # [FATAL] msg → @panic
```

Level filtering, file output, and color (`set_level`, `set_output`, `set_color`) are planned.

## `std.thread` — Threading

Handles are opaque `i64` values. Always `join` or `detach` every spawned thread;
always `mutex_free` / `atomic_free` when done.

```rust
# lifecycle
t := std.thread.spawn(my_fn)           # fn my_fn()
t := std.thread.spawn_arg(my_fn, ptr)  # fn my_fn(arg: i64)

std.thread.join(t)
std.thread.detach(t)
std.thread.yield()
std.thread.id()    -> i64
std.thread.cores() -> i32

# mutex
mtx := std.thread.mutex_new()
defer std.thread.mutex_free(mtx)
std.thread.mutex_lock(mtx)
std.thread.mutex_unlock(mtx)

# atomic i32
cnt := std.thread.atomic_new(0)
defer std.thread.atomic_free(cnt)
std.thread.atomic_store(cnt, 1)
val := std.thread.atomic_load(cnt)          # -> i32
old := std.thread.atomic_add(cnt, 1)        # fetch-add; returns prev
won := std.thread.atomic_cas(cnt, 0, 1)     # compare-and-swap; -> bool
```

## `std.malkur` — Gamedev

See [`Malkur.md`](Malkur.md) for the full API surface. Raylib-inspired flat API
on an SDL2 backend. v0.4 shipped: window/loop, keyboard, mouse, gamepad, 2D
shapes, textures (PNG/JPG via SDL_image), camera 2D, embedded 8×8 bitmap font,
TTF/OTF fonts (SDL_ttf), audio — sounds + streaming music (SDL_mixer), colors,
Vec2 math, 2D collision.

---

# Planned Extensions

Everything in this section is design spec — not yet implemented. APIs may change
when they land.

---

## Core Extensions

### `std.io` — Buffered Readers / Writers

Four dedicated handle types for common I/O patterns.

#### `file_wr` — Text File Writer

```rust
wr := try io.file_wr("out.txt")        # create / overwrite
wr := try io.file_wr_app("out.txt")    # append
defer wr.close()

try wr.write("hello ")
try wr.write_ln("world")
try wr.write_fmt("score: {}\n", score)
try wr.flush()
```

#### `file_rd` — Text File Reader

```rust
rd := try io.file_rd("in.txt")
defer rd.close()

line  := try rd.read_ln()    # -> str
text  := try rd.read_all()   # -> str
lines := try rd.read_lines() # -> []str

while rd.has_ln() {
    line := try rd.read_ln()
    @pl(line)
}
```

#### `byte_wr` — Binary Byte Writer

```rust
bwr := try io.byte_wr("data.bin")
defer bwr.close()

try bwr.write(buf: []u8)
try bwr.write_u8(val: u8)   # also u16/u32/u64, i8..i64, f32/f64
try bwr.flush()
try bwr.seek(pos: i64, .Start)
pos := bwr.tell() -> i64
```

#### `byte_rd` — Binary Byte Reader

```rust
brd := try io.byte_rd("data.bin")
defer brd.close()

buf := try brd.read(n: i32) -> []u8
b   := try brd.read_u8()    -> u8   # also u16/u32/u64, i8..i64, f32/f64

ok  := brd.has_next()  -> bool
sz  := brd.size()      -> i64
rem := brd.remaining() -> i64
try brd.seek(pos: i64, .Start)
```

#### Whole-file shortcuts

```rust
bytes := try io.read_bytes("data.bin") -> []u8
text  := try io.read_text("notes.txt") -> str
lines := try io.read_lines("log.txt")  -> []str

try io.write_bytes("data.bin", buf)
try io.write_text("notes.txt", "hello\n")
```

---

### `std.fs` Extensions

```rust
fs.mod_time(path: str) -> !i64      # unix timestamp
fs.ls_full(path: str)  -> []FileInfo

struct FileInfo {
    name:    str,
    size:    i64,
    is_dir:  bool,
    modTime: i64,
}

fs.path_join(a: str, b: str) -> str
fs.path_dir(path: str)       -> str
fs.path_base(path: str)      -> str
fs.path_ext(path: str)       -> str
fs.path_stem(path: str)      -> str
```

---

### `std.hash` — Hashing

```rust
# non-cryptographic
hash.fnv32(data: []u8)               -> u32
hash.fnv64(data: []u8)               -> u64
hash.xxhash32(data: []u8, seed: u32) -> u32
hash.xxhash64(data: []u8, seed: u64) -> u64
hash.murmur32(data: []u8, seed: u32) -> u32
hash.crc32(data: []u8)               -> u32

# cryptographic
hash.sha256(data: []u8) -> [32]u8
hash.sha512(data: []u8) -> [64]u8
hash.sha1(data: []u8)   -> [20]u8   # legacy only
hash.md5(data: []u8)    -> [16]u8   # legacy only

# HMAC
hash.hmac_sha256(key: []u8, data: []u8) -> [32]u8
hash.hmac_sha512(key: []u8, data: []u8) -> [64]u8

# streaming
ctx := hash.sha256_init()
hash.sha256_upd(ctx, chunk: []u8)
result := hash.sha256_fin(ctx) -> [32]u8

# helpers
hash.to_hex(bytes: []u8) -> str
hash.from_hex(s: str)    -> ![]u8
```

---

### `std.crypto` — Encryption

For the full high-level API see [`Pelentar.md`](Pelentar.md) (`@std.pelentar`).
The `std.crypto` surface is the lower-level symmetric/asymmetric layer:

```rust
# symmetric: ChaCha20-Poly1305
key   := crypto.chacha_key_gen()   -> [32]u8
nonce := crypto.chacha_nonce_gen() -> [12]u8
ct, tag := try crypto.chacha_enc(pt, key, nonce, aad)
pt      := try crypto.chacha_dec(ct, tag, key, nonce, aad)

# symmetric: AES-256-GCM
key := crypto.aes_key_gen(256) -> []u8
iv  := crypto.aes_iv_gen()     -> [12]u8
ct, tag := try crypto.aes_enc(pt, key, iv, aad)
pt      := try crypto.aes_dec(ct, tag, key, iv, aad)

# asymmetric: X25519 key exchange
privKey, pubKey := crypto.x25519_key_gen()
shared := try crypto.x25519_shared(myPrivKey, theirPubKey) -> [32]u8

# asymmetric: Ed25519 signing
privKey, pubKey := crypto.ed25519_key_gen()
sig := try crypto.ed25519_sign(data, privKey) -> [64]u8
ok  := crypto.ed25519_verify(data, sig, pubKey) -> bool

# KDF
key := try crypto.argon2id(password, salt, mem_kb, iters, key_len) -> []u8
key := try crypto.pbkdf2_sha256(password, salt, iters, key_len)    -> []u8

# CSPRNG
crypto.rand_bytes(buf: []u8)
crypto.rand_u32() -> u32
crypto.rand_u64() -> u64

# helpers
crypto.secure_zero(buf: []u8)
crypto.const_eq(a: []u8, b: []u8) -> bool   # timing-safe compare
```

---

### `std.db` — Database

SQLite as the primary embedded target; a generic `Conn` interface allows future
drivers (Postgres, MySQL) to plug in.

```rust
conn := try db.open("game.db")     # SQLite file or ":memory:"
defer db.close(conn)

try db.exec(conn, "CREATE TABLE ...")

rows := try db.query(conn, "SELECT name, score FROM t WHERE score > ?", 500)
defer db.rows_close(rows)

while db.rows_next(rows) {
    name  := db.col_str(rows, 0)
    score := db.col_i32(rows, 1)
}

# column accessors: col_i32 / col_i64 / col_f64 / col_str / col_bytes / col_null

# prepared statements
stmt := try db.prepare(conn, "INSERT INTO t (name, score) VALUES (?, ?)")
defer db.stmt_close(stmt)
try db.stmt_exec(stmt, "Alice", 9999)

# transactions
try db.begin_tx(conn)
try db.exec(conn, "UPDATE t SET score = score + 100 WHERE name = ?", "Alice")
db.commit_tx(conn) catch { db.rollback_tx(conn); ret err.DbFail }

n  := db.rows_affected(conn)   -> i64
id := db.last_insert_id(conn)  -> i64
```

---

### `std.thread` Extensions — Pool & Sync Primitives

```rust
# thread pool
pool := thread.pool_new(8)     # 0 = hardware_concurrency
defer thread.pool_free(pool)
thread.pool_run(pool, task_fn, arg)
thread.pool_wait(pool)

# RW mutex
rw := thread.rwmutex_new()
thread.read_lock(rw);   defer thread.read_unlock(rw)
thread.write_lock(rw);  defer thread.write_unlock(rw)
thread.rwmutex_free(rw)

# atomic i64
a := thread.atomic64_new(val: i64)
thread.atomic64_load(a)  -> i64
thread.atomic64_store(a, val: i64)
thread.atomic64_add(a, val: i64) -> i64
thread.atomic64_free(a)

# condition variable
cv := thread.cond_new()
thread.cond_wait(cv, mtx)
thread.cond_signal(cv)
thread.cond_broadcast(cv)
thread.cond_free(cv)
```

---

### `std.col` — Collections

> **Note:** the built-in `@ls(T)` covers dynamic arrays — see the
> [Lists section](Language.md#lists-ls) in Language.md. `std.col` provides
> the other collection types: HashMap, HashSet, Queue, Stack.

```rust
# HashMap
m := col.map_init(str, i32)
defer col.map_deinit(m)
col.map_set(m, "key", 1)
val, ok := col.map_get(m, "key")
col.map_del(m, "key")
col.map_has(m, "key") -> bool
for k, v => m {}

# HashSet
s := col.set_init(str)
col.set_add(s, "tag")
col.set_has(s, "tag") -> bool
col.set_del(s, "tag")

# Queue (FIFO)
q := col.queue_init(i32)
col.queue_push(q, val)
val := col.queue_pop(q)
val := col.queue_peek(q)

# Stack (LIFO)
stk := col.stack_init(i32)
col.stack_push(stk, val)
val := col.stack_pop(stk)
val := col.stack_peek(stk)
```

---

### `std.ser` — Serialisation

```rust
# JSON
json_str  := try ser.to_json(my_struct)
my_struct := try ser.from_json(json_str, MyStruct)

# binary (compact — good for game saves)
bytes     := try ser.to_bin(my_struct)
my_struct := try ser.from_bin(bytes, MyStruct)

# TOML
cfg  := try ser.toml_load("config.toml")
name := try ser.toml_get_str(cfg, "project.name")
n    := try ser.toml_get_i32(cfg, "build.workers")
```

---

## Tier 1 Extended

### `std.env` — Environment & Process

Access command-line arguments, environment variables, and process state.

```rust
@import ( env = @std.env )

# arguments
env.args() -> []str
env.arg(i: i32) -> str       # "" if out of range
env.arg_count() -> i32

# env vars
env.get(key: str) -> str
env.get_or(key: str, def: str) -> str
env.set(key: str, val: str)
env.del(key: str)
env.has(key: str) -> bool
env.all() -> []str            # ["KEY=VAL", ...]

# process
env.exit(code: i32)
env.pid() -> i32
env.exe() -> str              # absolute path of running binary
env.cwd() -> str
env.set_cwd(path: str) -> !void
```

---

### `std.path` — Path Manipulation

Pure string operations on paths. No I/O — for actual filesystem calls see `std.fs`.
Separator is always `/` internally; Windows paths normalised on input.

```rust
@import ( path = @std.path )

# decompose
path.dirname(p: str) -> str      # "/a/b/c.txt" → "/a/b"
path.basename(p: str) -> str     # "c.txt"
path.stem(p: str) -> str         # "c"
path.ext(p: str) -> str          # ".txt"
path.split(p: str) -> (str, str) # (dirname, basename)
path.split_ext(p: str) -> (str, str)  # ("path/c", ".txt")

# construct
path.join(a: str, b: str) -> str
path.join3(a: str, b: str, c: str) -> str
path.with_ext(p: str, ext: str) -> str
path.with_name(p: str, name: str) -> str

# normalise
path.normalize(p: str) -> str
path.abs(p: str) -> !str          # resolve relative to cwd
path.rel(p: str, base: str) -> !str
path.is_abs(p: str) -> bool
path.is_rel(p: str) -> bool

# glob
path.glob_match(pattern: str, p: str) -> bool  # * = any, ** = any depth
path.glob(pattern: str) -> ![]str              # expand against filesystem
```

---

### `std.json` — JSON

```rust
@import ( json = @std.json )

type Json = i64   # opaque tagged value

# parse / serialise
json.parse(s: str) -> !Json
json.parse_file(path: str) -> !Json
json.to_str(j: Json) -> str
json.to_str_pretty(j: Json, indent: i32) -> str

# type checks
json.is_null / is_bool / is_int / is_float / is_str / is_arr / is_obj

# scalar reads
json.as_bool(j: Json) -> !bool
json.as_int(j: Json)  -> !i64
json.as_float(j: Json) -> !f64
json.as_str(j: Json)  -> !str
json.len(j: Json)     -> i32

# object
json.has(j: Json, key: str) -> bool
json.get(j: Json, key: str) -> !Json
json.keys(j: Json) -> []str

# array
json.at(j: Json, i: i32) -> !Json

# builders
json.null() / json.bool(v) / json.int(v) / json.float(v) / json.str(v)
json.arr()  / json.obj()
json.arr_push(j, val)
json.obj_set(j, key, val)
json.obj_del(j, key)

err JsonError { ParseFailed, TypeMismatch, Missing, OutOfRange, IOFailed }
```

---

### `std.net` — Sockets

TCP/UDP networking and DNS. POSIX sockets on Linux/macOS, Winsock2 on Windows.

```rust
@import ( net = @std.net )

type TcpConn = i64 / TcpListener = i64 / UdpSock = i64

# TCP client
net.tcp_connect(addr: str) -> !TcpConn   # "host:port"
net.tcp_close(c)
net.tcp_read(c, n: i32) -> !str
net.tcp_read_exact(c, n: i32) -> !str
net.tcp_read_line(c) -> !str
net.tcp_read_all(c)  -> !str
net.tcp_write(c, data: str) -> !void
net.tcp_write_line(c, line: str) -> !void
net.tcp_set_timeout(c, secs: f64)
net.tcp_set_nodelay(c, on: bool)

# TCP server
net.tcp_listen(addr: str) -> !TcpListener
net.tcp_accept(l) -> !TcpConn
net.tcp_listener_close(l)

# UDP
net.udp_bind(addr: str) -> !UdpSock
net.udp_close(s)
net.udp_send(s, addr: str, data: str) -> !void
net.udp_recv(s, n: i32) -> !(str, str)   # (data, from_addr)
net.udp_set_timeout(s, secs: f64)

# DNS
net.dns_lookup(host: str) -> ![]str
net.dns_reverse(ip: str)  -> !str

err NetError { ConnRefused, ConnTimeout, ConnReset, HostNotFound,
               AddrInUse, ReadFailed, WriteFailed, Closed, InvalidAddr }
```

---

### `std.http` — HTTP Client

Backed by **libcurl**.

```rust
@import ( http = @std.http )

struct Response { status: i32, body: str }
http.resp_header(r, key: str) -> str
http.resp_ok(r) -> bool

# simple
http.get(url: str) -> !Response
http.post(url, body, content_type: str) -> !Response
http.put(url, body, content_type: str)  -> !Response
http.patch(url, body, content_type: str) -> !Response
http.delete(url: str) -> !Response
http.post_json(url, json_body: str) -> !Response

# builder
type Request = i64
http.request(method, url: str) -> Request
http.req_header(r, key, val: str) -> Request
http.req_body(r, data: str)     -> Request
http.req_body_json(r, json: str) -> Request
http.req_auth_basic(r, user, pass: str) -> Request
http.req_auth_bearer(r, token: str)     -> Request
http.req_timeout(r, secs: f64)  -> Request
http.req_follow(r, on: bool)    -> Request
http.req_send(r) -> !Response

# file download
http.download(url, dest: str) -> !void
http.download_progress(url, dest: str, fn(received: i64, total: i64) -> void) -> !void

err HttpError { ConnFailed, Timeout, TlsFailed, BadUrl, TooManyRedirects,
                IOFailed, BadResponse }
```

---

### `std.compress` — Compression

Backed by **zlib** (gzip/zlib) and **libzstd**.

```rust
@import ( compress = @std.compress )

enum algo => i32 { GZIP = 0, ZLIB = 1, ZSTD = 2 }

compress.encode(a: algo, data: str) -> !str
compress.decode(a: algo, data: str) -> !str
compress.encode_level(data: str, level: i32) -> !str  # zstd: 1–22

compress.compress_file(a, src, dst: str) -> !void
compress.decompress_file(a, src, dst: str) -> !void

# convenience
compress.gzip_file(src: str) -> !void     # → src.gz
compress.ungzip_file(src: str) -> !void
compress.zstd_file(src: str) -> !void     # → src.zst
compress.unzstd_file(src: str) -> !void

err CompressError { EncodeFailed, DecodeFailed, IOFailed, BadData }
```

---

### `std.regex` — Regular Expressions

C++17 `std::regex` backend — no external dependency.

```rust
@import ( re = @std.regex )

# one-shot
re.match(pattern, text: str) -> bool
re.search(pattern, text: str) -> bool
re.find(pattern, text: str) -> str
re.find_all(pattern, text: str) -> []str
re.captures(pattern, text: str) -> []str        # [full, g1, g2, ...]
re.captures_all(pattern, text: str) -> [][]str
re.replace(pattern, text, repl: str) -> str
re.replace_all(pattern, text, repl: str) -> str
re.split(pattern, text: str) -> []str

# compiled
type Regex = i64
re.compile(pattern: str) -> !Regex
re.free(r)
re.regex_match / regex_search / regex_find / regex_find_all
re.regex_captures / regex_captures_all
re.regex_replace / regex_replace_all / regex_split

# replacement: $0 = full, $1 $2 = groups, $name = named group (?P<name>...)

err RegexError { BadPattern, IOFailed }
```

---

## Tier 2 Extended

### `std.proc` — Subprocesses

Spawn and communicate with child processes. POSIX `fork`/`exec` on Linux/macOS,
`CreateProcess` on Windows.

```rust
@import ( proc = @std.proc )

type Proc = i64

# fire-and-forget
proc.run(cmd: str) -> !i32               # exit code
proc.run_args(args: []str) -> !i32

# capture
proc.output(cmd: str) -> !str            # stdout as str
proc.output_both(cmd: str) -> !(str, str) # (stdout, stderr)

# non-blocking
proc.spawn(cmd: str) -> !Proc
proc.spawn_args(args: []str) -> !Proc
proc.wait(p) -> !i32
proc.kill(p)
proc.pid(p) -> i32
proc.is_running(p) -> bool

# pipes
proc.write(p, data: str) -> !void
proc.write_line(p, line: str) -> !void
proc.close_stdin(p)
proc.read(p, n: i32) -> !str
proc.read_line(p) -> !str
proc.read_all(p) -> !str
proc.read_err(p, n: i32) -> !str

err ProcError { SpawnFailed, WaitFailed, IOFailed, NotRunning }
```

**Example**

```rust
out := try proc.output("git log --oneline -5")
@pl(out)

p := try proc.spawn("python3 -c \"import sys; sys.stdout.write(sys.stdin.read().upper())\"")
try proc.write(p, "hello\n")
proc.close_stdin(p)
@pl(try proc.read_all(p))   # HELLO
try proc.wait(p)
```

---

### `std.bytes` — Byte Buffers & Bit Ops

```rust
@import ( bytes = @std.bytes )

type Buf = i64

# construction
bytes.new(cap: i32) -> Buf
bytes.from_str(s: str) -> Buf
bytes.repeat(v: u8, n: i32) -> Buf
bytes.free(b)

# access
bytes.len(b) / bytes.cap(b) -> i32
bytes.get(b, i: i32) -> u8
bytes.set(b, i: i32, v: u8)
bytes.slice(b, start, end: i32) -> str
bytes.to_str(b) -> str
bytes.clear(b)
bytes.reserve(b, n: i32)

# append
bytes.push(b, v: u8)
bytes.push_str(b, s: str)
bytes.push_u16le / push_u32le / push_u64le(b, v)
bytes.push_u16be / push_u32be / push_u64be(b, v)
bytes.push_f32le / push_f64le(b, v)

# endian reads from raw str
bytes.u8_at(s, off: i32) -> u8
bytes.u16le / u32le / u64le(s, off) -> uN
bytes.u16be / u32be / u64be(s, off) -> uN
bytes.f32le / f64le(s, off) -> fN

# bit ops
bytes.popcount(v: u64) -> i32
bytes.clz(v: u64) -> i32
bytes.ctz(v: u64) -> i32
bytes.bswap16 / bswap32 / bswap64(v) -> uN
bytes.rotl32 / rotr32(v: u32, n: i32) -> u32

# encoding
bytes.to_hex(b) -> str
bytes.from_hex(s: str) -> !Buf
bytes.to_base64(b) -> str
bytes.from_base64(s: str) -> !Buf
```

---

### `std.date` — Calendar Date & Time

Calendar-aware date/time on top of Unix timestamps. C stdlib `<ctime>` +
`<chrono>` — no external dependency.

```rust
@import ( date = @std.date )

type DateTime = i64   # Unix timestamp (seconds UTC)
type Date     = i64   # days since 1970-01-01

# current time
date.now() -> DateTime
date.now_local() -> DateTime
date.today() -> Date

# construction
date.from_ymd(y, m, d: i32) -> !DateTime
date.from_ymd_hms(y, m, d, h, min, s: i32) -> !DateTime
date.from_unix(secs: i64) -> DateTime
date.to_unix(dt) -> i64

# decompose
date.year / month / day / hour / minute / second(dt) -> i32
date.weekday(dt) -> i32   # 0=Sun…6=Sat
date.yearday / week(dt) -> i32

# arithmetic
date.add_secs / add_mins / add_hours / add_days / add_months / add_years(dt, n)
date.diff_secs(a, b) -> i64
date.diff_days(a, b) -> i64

# format / parse (strftime codes: %Y %m %d %H %M %S …)
date.format(dt, fmt: str) -> str
date.format_local(dt, fmt: str) -> str
date.parse(s, fmt: str) -> !DateTime
date.to_iso8601(dt) -> str    # "2026-06-11T14:30:00Z"
date.from_iso8601(s: str) -> !DateTime
date.to_rfc2822(dt) -> str

# timezone
date.local_offset() -> i32    # minutes east of UTC
date.to_local / to_utc(dt) -> DateTime
```

---

### `std.uuid` — UUID Generation

```rust
@import ( uuid = @std.uuid )

uuid.v4() -> str           # random
uuid.v5(ns: str, name: str) -> str   # deterministic SHA-1

uuid.NS_DNS :: "6ba7b810-9dad-11d1-80b4-00c04fd430c8"
uuid.NS_URL :: "6ba7b811-9dad-11d1-80b4-00c04fd430c8"
uuid.NS_OID :: "6ba7b812-9dad-11d1-80b4-00c04fd430c8"

uuid.nil()   -> str
uuid.valid(s: str) -> bool
uuid.parse(s: str) -> !str    # validate + normalise lowercase
uuid.strip(s: str) -> str     # remove dashes → 32 hex chars
uuid.from_strip(s: str) -> !str
```

---

### `std.toml` — TOML Config Files

Used internally for `olrn_pkg.toml`. Header-only parser, no external dep.

```rust
@import ( toml = @std.toml )

type Toml = i64

toml.parse(s: str) -> !Toml
toml.parse_file(path: str) -> !Toml
toml.to_str(t) -> str
toml.write_file(t, path: str) -> !void

# dotted key access: "server.host", "ports.0"
toml.get(t, key: str) -> !Toml
toml.has(t, key: str) -> bool
toml.as_str / as_int / as_float / as_bool(t) -> !T
toml.as_arr(t) -> ![]Toml
toml.keys(t) -> []str
toml.len(t) -> i32
toml.is_table / is_arr / is_str / is_int(t) -> bool

# build
toml.table() / toml.arr() -> Toml
toml.str(v) / toml.int(v) / toml.float(v) / toml.bool(v) -> Toml
toml.set(t, key: str, val: Toml)
toml.arr_push(t, val: Toml)

err TomlError { ParseFailed, TypeMismatch, Missing, IOFailed }
```

---

### `std.ws` — WebSocket Client

Backed by **libwebsockets**.

```rust
@import ( ws = @std.ws )

type WsConn = i64

ws.connect(url: str) -> !WsConn     # "ws://..." or "wss://..."
ws.connect_headers(url: str, headers: []str) -> !WsConn
ws.close(c)
ws.is_open(c) -> bool

ws.send(c, msg: str) -> !void        # text frame
ws.send_bin(c, data: str) -> !void   # binary frame
ws.ping(c) -> !void

ws.recv(c) -> !str
ws.recv_timeout(c, secs: f64) -> !str   # WsError.Timeout on expire

err WsError { ConnFailed, TlsFailed, HandshakeFailed,
              SendFailed, RecvFailed, Timeout, Closed }
```

---

## Tier 3 Extended

### `std.csv` — CSV

```rust
@import ( csv = @std.csv )

struct CsvOpts { delim: str, quote: str, comment: str, trim: bool }
csv.default_opts() -> CsvOpts

csv.parse(s: str) -> [][]str
csv.parse_opts(s, opts) -> [][]str
csv.parse_file(path: str) -> ![][]str
csv.parse_file_opts(path, opts) -> ![][]str
csv.parse_header(s: str) -> ([]str, [][]str)    # (headers, rows)
csv.parse_header_file(path: str) -> !([]str, [][]str)

csv.to_str(rows: [][]str) -> str
csv.to_str_opts(rows, opts) -> str
csv.to_str_header(headers: []str, rows: [][]str) -> str
csv.write_file(path, rows) -> !void
csv.write_file_header(path, headers, rows) -> !void
```

---

### `std.xml` — XML

Light DOM — no schema validation, no XPath. Header-only parser, no external dep.

```rust
@import ( xml = @std.xml )

type XmlNode = i64

xml.parse(s: str) -> !XmlNode
xml.parse_file(path: str) -> !XmlNode
xml.to_str(n) -> str
xml.to_str_pretty(n, indent: i32) -> str
xml.write_file(n, path: str) -> !void

# read
xml.tag(n) -> str
xml.text(n) -> str
xml.text_all(n) -> str          # all descendant text
xml.attr(n, key: str) -> str    # "" if absent
xml.attrs(n) -> []str           # ["key=val", ...]
xml.has_attr(n, key: str) -> bool
xml.children(n) -> []XmlNode
xml.child_count(n) -> i32
xml.parent(n) -> !XmlNode

xml.find(n, tag: str) -> !XmlNode
xml.find_all(n, tag: str) -> []XmlNode
xml.find_attr(n, attr, val: str) -> !XmlNode

# build
xml.node(tag: str) -> XmlNode
xml.node_text(tag, text: str) -> XmlNode
xml.attr_set(n, key, val: str)
xml.attr_del(n, key: str)
xml.child_add(parent, child: XmlNode)
xml.child_remove(parent, child: XmlNode)
xml.set_text(n, text: str)

err XmlError { ParseFailed, IOFailed, NoParent, NotFound }
```

---

### `std.test` — Test Framework

Test functions named `test_*` are discovered automatically by `olrn test`.

```rust
@import ( t = @std.test )

t.assert(cond: bool, msg: str)
t.assert_eq / assert_ne / assert_lt / assert_le / assert_gt / assert_ge(a, b)
t.assert_near(a, b, eps: f64)   # |a-b| <= eps
t.assert_ok(result)             # must not be an error
t.assert_err(result)            # must be an error
t.assert_contains(s, sub: str)
t.assert_match(s, pattern: str)

t.fail(msg: str)    # unconditional failure
t.skip(msg: str)    # skip with reason
t.log(msg: str)     # shown only when the test fails
```

```rust
fn test_add() -> void
{
    t.assert_eq(1 + 1, 2)
    t.assert_ne(1 + 1, 3)
}
```

```sh
olrn test                  # all test_* in project
olrn test file.olrn        # specific file
olrn test --filter add     # by name substring
olrn test --verbose        # show passing tests
```

---

### `std.rand` — Extended Random

Xoshiro256++ PRNG — fast, high-quality, **not cryptographically secure**.
For security use `crypto.rand_bytes`.

```rust
@import ( rand = @std.rand )

rand.seed(n: u64)     # deterministic (useful for tests)
rand.seed_time()      # auto-seeded from current time on first use

rand.int(lo, hi: i64) -> i64
rand.uint(lo, hi: u64) -> u64
rand.float(lo, hi: f64) -> f64
rand.bool() -> bool
rand.byte() -> u8
rand.bytes(n: i32) -> str    # NOT crypto-safe

rand.shuffle(arr: []any)                     # Fisher-Yates in place
rand.choice(arr: []any) -> any
rand.sample(arr: []any, n: i32) -> []any     # without replacement
rand.weighted(weights: []f64) -> i32         # index proportional to weight

rand.normal(mean, std_dev: f64) -> f64       # Gaussian
rand.exp(lambda: f64) -> f64
rand.bernoulli(p: f64) -> bool
```

---

# System Dependencies

| Module | Library | pkg-config | Arch | apt |
|---|---|---|---|---|
| `std.io` `std.fs` `std.time` `std.math` `std.mem` `std.str` `std.log` | C stdlib / C++17 | — | — | — |
| `std.thread` | `<thread>` / pthreads | — | — | — |
| `std.env` `std.path` `std.uuid` | C stdlib | — | — | — |
| `std.json` `std.toml` `std.xml` | header-only | — | — | — |
| `std.net` `std.proc` | POSIX / Winsock2 | — | — | — |
| `std.http` | libcurl | `libcurl` | `curl` | `libcurl4-openssl-dev` |
| `std.compress` | zlib + libzstd | `zlib`, `libzstd` | `zlib`, `zstd` | `zlib1g-dev`, `libzstd-dev` |
| `std.regex` | C++17 `std::regex` | — | — | — |
| `std.ws` | libwebsockets | `libwebsockets` | `libwebsockets` | `libwebsockets-dev` |
| `std.bytes` `std.date` `std.csv` `std.rand` | C stdlib | — | — | — |
| `std.test` | C stdlib | — | — | — |
