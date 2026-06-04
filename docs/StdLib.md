# Oleren Standard Library — Core APIs

Naming convention across all stdlib:
- Functions: `snake_case`
- Variables: `camelCase`
- When a func references a camelCase var, the var name keeps its casing: `set_fooBarThing`
- Long names use readable abbreviations: `get_file_wr`, `buf_rd`, `init`, `deinit`

---

## `std.file` — File I/O

```rust
import ( io = @file )
```

```rust

# ── Open / Close ──────────────────────────────────────
f := try file.open("data.bin", .Read)
defer file.close(f)

# modes: .Read  .Write  .Append  .ReadWrite  .WriteCreate
f := try file.open_ex("data.bin", .Write, .Trunc)   # extra flags

# ── Whole-file helpers ────────────────────────────────
bytes := try file.read_all(f)           -> []u8
text  := try file.read_all_str(f)       -> str
lines := try file.read_lines(f)         -> []str
try file.write_all(f, bytes)
try file.write_str(f, "hello\n")

# ── Streaming reads ───────────────────────────────────
n    := try file.read(f, buf)           # read up to buf.len bytes, ret actual n
line := try file.read_ln(f)             -> str   # one line, strips newline

# ── Streaming writes ──────────────────────────────────
try file.write(f, buf)
try file.write_ln(f, "hello")

# ── Seek / Tell ───────────────────────────────────────
try file.seek(f, 0, .Start)             # .Start  .Cur  .End
pos := file.tell(f)                     -> i64
sz  := file.size(f)                     -> i64

# ── Buffered wrapper ──────────────────────────────────
wr := file.get_wr(f)                    # buffered writer
try wr.write(buf)
try wr.write_ln("line")
try wr.flush()

rd := file.get_rd(f)                    # buffered reader
chunk := try rd.read(n: i32)            -> []u8
line  := try rd.read_ln()               -> str
```

### Filesystem

```rust
# Query
file.exists(path: []chr)    -> bool
file.is_dir(path: []chr)    -> bool
file.is_file(path: []chr)   -> bool
file.size_of(path: []chr)   -> !i64
file.mod_time(path: []chr)  -> !i64   # unix timestamp

# Ops
try file.mkdir(path)
try file.mkdir_all(path)           # create parent dirs too
try file.rm(path)
try file.rm_all(path)              # recursive delete
try file.mv(src, dst)
try file.cp(src, dst)

# Directory listing
entries := try file.ls(path)       -> []str      # names only
entries := try file.ls_full(path)  -> []FileInfo # name, size, is_dir, mod_time

struct FileInfo {
    name:     str,
    size:     i64,
    is_dir:   bool,
    modTime:  i64,
}

# Path helpers
file.path_join(a: []chr, b: []chr)   -> str
file.path_dir(path: []chr)           -> str   # parent dir
file.path_base(path: []chr)          -> str   # filename
file.path_ext(path: []chr)           -> str   # extension with dot
file.path_stem(path: []chr)          -> str   # filename without ext
```

---

## `std.hash` — Hashing

Non-cryptographic hashes are for speed (maps, checksums).
Cryptographic hashes are for integrity and security.

```rust
import ( hsh = @hash )
```

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
hash.from_hex(s: []chr)    -> ![]u8

# ── Password hashing (slow by design) ────────────────
hash.bcrypt(password: []u8, cost: i32) -> !str
hash.bcrypt_verify(password: []u8, hashed: []chr) -> bool
```

---

## `std.crypto` — Encryption

```rust
import ( cry = @crypto )
```

```rust

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

---

## `std.db` — Databases

SQLite is the primary embedded target. A generic `Conn` interface allows
future drivers (Postgres, MySQL, etc.) to plug in.

```rust
import ( db = @db )         # SQLite by default
import ( db = @db.pg )      # Postgres driver
import ( db = @db.mysql )   # MySQL driver
```

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
db.commit_tx(conn) catch |e| {
    db.rollback_tx(conn)
    ret err.DbFail
}

# ── Row count / last insert ───────────────────────────
n  := db.rows_affected(conn) -> i64
id := db.last_insert_id(conn) -> i64
```

---

## `std.thread` — Threading & Parallelism

```rust
import (
    thr  = @thread,
    sync = @sync,
)
```

```rust

# ── Basic threads ─────────────────────────────────────
t := try thread.spawn(my_fn, arg)
thread.join(t)
thread.detach(t)
thread.id()    -> u64    # current thread id
thread.sleep(secs: f64)

# ── Thread pool (parallel work) ───────────────────────
pool := try thread.pool_init(8)    # 8 workers; 0 = number of CPU cores
defer thread.pool_deinit(pool)

thread.pool_run(pool, task_fn, data)    # queue one task
thread.pool_run_n(pool, task_fn, data, n)  # queue n identical tasks
thread.pool_wait(pool)                  # block until all queued work done

# ── Parallel for (data parallelism) ───────────────────
# Split a slice across workers automatically
thread.par_for(pool, items, fn(item: T, idx: i32)
{
    # runs concurrently on each item
})

thread.par_for_range(pool, 0, N, fn(i: i32)
{
    results[i] = heavy_compute(i)
})

# ── Mutex ─────────────────────────────────────────────
mu := sync.mutex_init()
sync.lock(mu)
defer sync.unlock(mu)

# ── RW Mutex ──────────────────────────────────────────
rw := sync.rwmutex_init()
sync.read_lock(rw)    defer sync.read_unlock(rw)
sync.write_lock(rw)   defer sync.write_unlock(rw)

# ── Atomics ───────────────────────────────────────────
sync.atom_load_i32(ptr: *i32)              -> i32
sync.atom_store_i32(ptr: *i32, val: i32)
sync.atom_add_i32(ptr: *i32, n: i32)       -> i32   # ret prev value
sync.atom_cas_i32(ptr: *i32, exp: i32, new: i32) -> bool

# same for i64, u32, u64 variants: atom_load_i64, etc.

# ── Channel (message passing) ─────────────────────────
ch := sync.chan_init(i32, 16)    # typed channel, capacity 16 (0 = unbuffered)
defer sync.chan_close(ch)

sync.chan_send(ch, 42)
val  := sync.chan_recv(ch)       -> i32
val, ok := sync.chan_try_recv(ch)  # non-blocking; ok = false if empty

# ── Semaphore ─────────────────────────────────────────
sem := sync.sem_init(4)          # max 4 concurrent holders
sync.sem_wait(sem)
sync.sem_post(sem)

# ── Once (run exactly one time) ───────────────────────
once := sync.once_init()
sync.once_do(once, init_fn)      # guaranteed single execution across threads

# ── Condition variable ────────────────────────────────
cond := sync.cond_init()
sync.cond_wait(cond, mu)
sync.cond_signal(cond)
sync.cond_broadcast(cond)
```

---

## `std.net` — Networking

```rust
import ( net = @net )
```

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
net.is_ipv4(addr: []chr)   -> bool
net.is_ipv6(addr: []chr)   -> bool
```

---

## `std.col` — Collections

```rust
import ( col = @col )
```

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

---

## `std.ser` — Serialization

```rust
import ( ser = @ser )
```

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

---

## `std.log` — Logging

```rust
import ( log = @log )
```

```rust

# log levels: .Debug  .Info  .Warn  .Error  .Fatal
log.set_level(.Info)
log.set_output(file_writer)   # redirect to file
log.set_color(true)           # colored terminal output

log.debug("frame {}", frame_n)
log.info("window created {}x{}", w, h)
log.warn("texture not found: {}", path)
log.error("failed to open db: {}", err)
log.fatal("out of memory")     # logs then calls @panic
```

---

## `std.time` — Timing

```rust
import ( time = @time )
```

```rust

now  := time.now()           -> i64    # nanoseconds since epoch
mono := time.mono()          -> i64    # monotonic ns (for intervals, not wall time)

elapsed := time.since(mono)  -> f64    # seconds as f64
time.sleep(secs: f64)

# timestamp helpers
time.ns_to_ms(ns: i64)  -> f64
time.ns_to_sec(ns: i64) -> f64

# simple stopwatch
sw := time.sw_start()
# ... do work
dt := time.sw_elapsed(sw)   -> f64   # seconds
time.sw_reset(sw)
```
