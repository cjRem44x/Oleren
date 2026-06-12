# Oleren Standard Library — Tier 2 & 3 Modules

Tier 2 and Tier 3 standard library modules. All are **Planned** (design spec,
not yet implemented). Import via `@std.<module>`.

---

# Tier 2

---

## `std.proc` — Subprocesses

Spawn and communicate with child processes. Backed by `fork`/`exec` on
POSIX and `CreateProcess` on Windows.

```rust
@import ( proc = @std.proc )
```

### Types

```rust
type Proc = i64   # handle to a running child process
```

### Fire and Forget

```rust
# Run a shell command, wait for it, return exit code
proc.run(cmd: str) -> !i32

# Run with explicit argv (no shell — safer for untrusted input)
proc.run_args(args: []str) -> !i32
```

### Capture Output

```rust
# Run and return stdout as a string (stderr goes to the terminal)
proc.output(cmd: str) -> !str

# Run and return both stdout and stderr separately
proc.output_both(cmd: str) -> !(str, str)   # (stdout, stderr)
```

### Non-Blocking Spawn

```rust
proc.spawn(cmd: str) -> !Proc
proc.spawn_args(args: []str) -> !Proc

proc.wait(p: Proc) -> !i32      # block until exit; returns exit code
proc.kill(p: Proc)               # SIGKILL / TerminateProcess
proc.pid(p: Proc) -> i32
proc.is_running(p: Proc) -> bool
```

### Stdin / Stdout Pipes

```rust
proc.write(p: Proc, data: str) -> !void    # write to child stdin
proc.write_line(p: Proc, line: str) -> !void
proc.close_stdin(p: Proc)                   # signal EOF to child

proc.read(p: Proc, n: i32) -> !str         # read from child stdout
proc.read_line(p: Proc) -> !str
proc.read_all(p: Proc) -> !str
proc.read_err(p: Proc, n: i32) -> !str     # read from child stderr
```

### Error Set

```rust
err ProcError { SpawnFailed, WaitFailed, IOFailed, NotRunning }
```

### Example

```rust
@import ( proc = @std.proc )

fn main() -> !void
{
    # simple capture
    out := try proc.output("git log --oneline -5")
    @pl(out)

    # interactive pipe
    p := try proc.spawn("python3 -c \"import sys; sys.stdout.write(sys.stdin.read().upper())\"")
    try proc.write(p, "hello world\n")
    proc.close_stdin(p)
    result := try proc.read_all(p)
    try proc.wait(p)
    @pl(result)  # HELLO WORLD
}
```

---

## `std.bytes` — Byte Buffers & Bit Ops

Mutable byte buffers, endian-aware integer reads/writes, and bit manipulation.
No external dependency.

```rust
@import ( bytes = @std.bytes )
```

### Buffer Type

```rust
type Buf = i64   # mutable heap-allocated byte buffer
```

### Construction

```rust
bytes.new(cap: i32) -> Buf          # empty buffer with pre-allocated capacity
bytes.from_str(s: str) -> Buf       # copy a str into a buffer
bytes.repeat(v: u8, n: i32) -> Buf  # n copies of byte v
bytes.free(b: Buf)
```

### Read / Write

```rust
bytes.len(b: Buf) -> i32
bytes.cap(b: Buf) -> i32
bytes.get(b: Buf, i: i32) -> u8
bytes.set(b: Buf, i: i32, v: u8)
bytes.slice(b: Buf, start: i32, end: i32) -> str  # view as str (no copy)
bytes.to_str(b: Buf) -> str                        # copy entire buffer as str
bytes.clear(b: Buf)
bytes.reserve(b: Buf, n: i32)
```

### Append

```rust
bytes.push(b: Buf, v: u8)
bytes.push_str(b: Buf, s: str)
bytes.push_u16le(b: Buf, v: u16)
bytes.push_u32le(b: Buf, v: u32)
bytes.push_u64le(b: Buf, v: u64)
bytes.push_u16be(b: Buf, v: u16)
bytes.push_u32be(b: Buf, v: u32)
bytes.push_u64be(b: Buf, v: u64)
bytes.push_f32le(b: Buf, v: f32)
bytes.push_f64le(b: Buf, v: f64)
```

### Endian Reads (from a raw str / slice)

```rust
# Read integer at byte offset `off` from a raw str
bytes.u8_at(s: str, off: i32)  -> u8
bytes.u16le(s: str, off: i32)  -> u16
bytes.u32le(s: str, off: i32)  -> u32
bytes.u64le(s: str, off: i32)  -> u64
bytes.u16be(s: str, off: i32)  -> u16
bytes.u32be(s: str, off: i32)  -> u32
bytes.u64be(s: str, off: i32)  -> u64
bytes.f32le(s: str, off: i32)  -> f32
bytes.f64le(s: str, off: i32)  -> f64
```

### Bit Operations

```rust
bytes.popcount(v: u64) -> i32    # count set bits
bytes.clz(v: u64) -> i32        # count leading zeros
bytes.ctz(v: u64) -> i32        # count trailing zeros
bytes.bswap16(v: u16) -> u16    # byte-swap
bytes.bswap32(v: u32) -> u32
bytes.bswap64(v: u64) -> u64
bytes.rotl32(v: u32, n: i32) -> u32
bytes.rotr32(v: u32, n: i32) -> u32
```

### Encoding

```rust
bytes.to_hex(b: Buf) -> str          # lowercase hex string
bytes.from_hex(s: str) -> !Buf       # parse hex string into buffer
bytes.to_base64(b: Buf) -> str
bytes.from_base64(s: str) -> !Buf
```

### Example

```rust
@import ( bytes = @std.bytes )

fn main() -> void
{
    b := bytes.new(16)
    defer bytes.free(b)

    bytes.push_u32le(b, 0xDEADBEEF)
    bytes.push_str(b, "hi")

    @pl(bytes.to_hex(b))   # deadbeef6869
    @pl(@str(bytes.u32le(bytes.to_str(b), 0)))  # 3735928559
}
```

---

## `std.date` — Calendar Date & Time

Calendar-aware date/time on top of Unix timestamps. Backed by C stdlib
`<ctime>` and `<chrono>` — no external dependency.

```rust
@import ( date = @std.date )
```

### Types

```rust
type DateTime = i64   # Unix timestamp (seconds since 1970-01-01 UTC)
type Date     = i64   # days since 1970-01-01 (date-only, no time)
```

### Current Time

```rust
date.now()       -> DateTime   # current UTC time
date.now_local() -> DateTime   # current local time
date.today()     -> Date       # today as a Date (UTC)
```

### Construction

```rust
date.from_ymd(y: i32, m: i32, d: i32) -> !DateTime     # midnight UTC
date.from_ymd_hms(y: i32, m: i32, d: i32,
                  h: i32, min: i32, s: i32) -> !DateTime
date.from_unix(secs: i64) -> DateTime
date.to_unix(dt: DateTime) -> i64
```

### Decomposition

```rust
date.year(dt: DateTime)    -> i32   # e.g. 2026
date.month(dt: DateTime)   -> i32   # 1–12
date.day(dt: DateTime)     -> i32   # 1–31
date.hour(dt: DateTime)    -> i32   # 0–23
date.minute(dt: DateTime)  -> i32   # 0–59
date.second(dt: DateTime)  -> i32   # 0–59
date.weekday(dt: DateTime) -> i32   # 0=Sun … 6=Sat
date.yearday(dt: DateTime) -> i32   # 1–366
date.week(dt: DateTime)    -> i32   # ISO week number 1–53
```

### Arithmetic

```rust
date.add_secs(dt: DateTime, n: i64)  -> DateTime
date.add_mins(dt: DateTime, n: i32)  -> DateTime
date.add_hours(dt: DateTime, n: i32) -> DateTime
date.add_days(dt: DateTime, n: i32)  -> DateTime
date.add_months(dt: DateTime, n: i32) -> DateTime
date.add_years(dt: DateTime, n: i32)  -> DateTime

date.diff_secs(a: DateTime, b: DateTime) -> i64
date.diff_days(a: DateTime, b: DateTime) -> i64
```

### Formatting & Parsing

Uses strftime / strptime format codes (`%Y`, `%m`, `%d`, `%H`, `%M`, `%S`, …).

```rust
date.format(dt: DateTime, fmt: str) -> str
date.format_local(dt: DateTime, fmt: str) -> str
date.parse(s: str, fmt: str) -> !DateTime
date.to_iso8601(dt: DateTime) -> str   # "2026-06-11T14:30:00Z"
date.from_iso8601(s: str) -> !DateTime
date.to_rfc2822(dt: DateTime) -> str   # HTTP Date header format
```

### Timezone

```rust
date.local_offset() -> i32    # minutes east of UTC (e.g. -300 for EST)
date.to_local(dt: DateTime) -> DateTime
date.to_utc(dt: DateTime) -> DateTime
```

### Example

```rust
@import ( date = @std.date )

fn main() -> void
{
    now :: date.now()
    @pl(date.to_iso8601(now))                        # 2026-06-11T...Z
    @pl(date.format(now, "%A, %B %d %Y"))            # Wednesday, June 11 2026

    deadline := date.add_days(now, 30)
    diff     := date.diff_days(deadline, now)
    @pl("days left: " + @str(diff))
}
```

---

## `std.uuid` — UUID Generation

Generate, parse, and format UUIDs. No external dependency — uses the
system CSPRNG for v4 and SHA-1 for v5.

```rust
@import ( uuid = @std.uuid )
```

```rust
uuid.v4() -> str           # random UUID: "f47ac10b-58cc-4372-a567-0e02b2c3d479"
uuid.v5(ns: str, name: str) -> str  # deterministic from namespace UUID + name

# Predefined namespaces for v5
uuid.NS_DNS  :: "6ba7b810-9dad-11d1-80b4-00c04fd430c8"
uuid.NS_URL  :: "6ba7b811-9dad-11d1-80b4-00c04fd430c8"
uuid.NS_OID  :: "6ba7b812-9dad-11d1-80b4-00c04fd430c8"

uuid.nil()   -> str        # "00000000-0000-0000-0000-000000000000"
uuid.valid(s: str) -> bool # validate format
uuid.parse(s: str) -> !str # validate + normalise to lowercase
uuid.strip(s: str) -> str  # remove dashes → 32 hex chars
uuid.from_strip(s: str) -> !str   # 32 hex → canonical UUID
```

### Example

```rust
@import ( uuid = @std.uuid )

fn main() -> void
{
    id   :: uuid.v4()
    @pl(id)   # f47ac10b-58cc-4372-a567-0e02b2c3d479

    page_id :: uuid.v5(uuid.NS_URL, "https://example.com/page/1")
    @pl(page_id)   # deterministic, same every run

    @pl(uuid.strip(id))   # 32 hex chars, no dashes
}
```

---

## `std.toml` — TOML Config Files

Parse and write TOML (v1.0). Used internally for `olrn_pkg.toml`.
Backed by a header-only TOML parser — no external library dependency.

```rust
@import ( toml = @std.toml )
```

### Types

```rust
type Toml = i64   # opaque TOML value (table, array, or scalar)
```

### Parsing

```rust
toml.parse(s: str) -> !Toml
toml.parse_file(path: str) -> !Toml
```

### Serialisation

```rust
toml.to_str(t: Toml) -> str
toml.write_file(t: Toml, path: str) -> !void
```

### Reading (dotted key access: `"server.host"`, `"ports.0"`)

```rust
toml.get(t: Toml, key: str) -> !Toml
toml.has(t: Toml, key: str) -> bool

toml.as_str(t: Toml)   -> !str
toml.as_int(t: Toml)   -> !i64
toml.as_float(t: Toml) -> !f64
toml.as_bool(t: Toml)  -> !bool
toml.as_arr(t: Toml)   -> ![]Toml

toml.keys(t: Toml) -> []str    # top-level table keys
toml.len(t: Toml)  -> i32      # array length or table key count

toml.is_table(t: Toml) -> bool
toml.is_arr(t: Toml)   -> bool
toml.is_str(t: Toml)   -> bool
toml.is_int(t: Toml)   -> bool
```

### Building

```rust
toml.table() -> Toml
toml.arr()   -> Toml
toml.str(v: str)   -> Toml
toml.int(v: i64)   -> Toml
toml.float(v: f64) -> Toml
toml.bool(v: bool) -> Toml

toml.set(t: Toml, key: str, val: Toml)
toml.arr_push(t: Toml, val: Toml)
```

### Error Set

```rust
err TomlError { ParseFailed, TypeMismatch, Missing, IOFailed }
```

### Example

```rust
@import ( toml = @std.toml )

fn main() -> !void
{
    cfg := try toml.parse_file("olrn_pkg.toml")

    name    := try toml.as_str(try toml.get(cfg, "project.name"))
    version := try toml.as_str(try toml.get(cfg, "project.version"))
    @pl(name + " " + version)
}
```

---

## `std.ws` — WebSocket Client

WebSocket (RFC 6455) client over TCP/TLS. Backed by **libwebsockets**.

```rust
@import ( ws = @std.ws )
```

### Types

```rust
type WsConn = i64
```

### Connection

```rust
ws.connect(url: str) -> !WsConn     # "ws://host:port/path" or "wss://..."
ws.connect_headers(url: str, headers: []str) -> !WsConn  # ["Key: Val", ...]
ws.close(c: WsConn)
ws.is_open(c: WsConn) -> bool
```

### Send

```rust
ws.send(c: WsConn, msg: str) -> !void       # text frame (UTF-8)
ws.send_bin(c: WsConn, data: str) -> !void  # binary frame
ws.ping(c: WsConn) -> !void
```

### Receive

```rust
ws.recv(c: WsConn) -> !str                      # blocks until message
ws.recv_timeout(c: WsConn, secs: f64) -> !str   # WsError.Timeout on expire
```

### Error Set

```rust
err WsError {
    ConnFailed, TlsFailed, HandshakeFailed,
    SendFailed, RecvFailed, Timeout, Closed,
}
```

### Example

```rust
@import ( ws = @std.ws, json = @std.json )

fn main() -> !void
{
    c := try ws.connect("wss://echo.websocket.org")
    defer ws.close(c)

    try ws.send(c, "{\"msg\":\"hello\"}")
    resp := try ws.recv_timeout(c, 5.0)
    @pl(resp)
}
```

---

# Tier 3

---

## `std.csv` — CSV

Parse and write comma-separated values. Handles quoted fields, escaped
quotes, custom delimiters. No external dependency.

```rust
@import ( csv = @std.csv )
```

### Options

```rust
struct CsvOpts {
    delim:   str,   # default ","
    quote:   str,   # default "\""
    comment: str,   # skip lines starting with this (default "" = disabled)
    trim:    bool,  # trim whitespace from fields (default false)
}

csv.default_opts() -> CsvOpts
```

### Parsing

```rust
# Parse all rows (including header if present)
csv.parse(s: str) -> [][]str
csv.parse_opts(s: str, opts: CsvOpts) -> [][]str
csv.parse_file(path: str) -> ![][]str
csv.parse_file_opts(path: str, opts: CsvOpts) -> ![][]str

# Parse with explicit header row; returns (headers, data_rows)
csv.parse_header(s: str) -> ([]str, [][]str)
csv.parse_header_file(path: str) -> !([]str, [][]str)
```

### Serialisation

```rust
csv.to_str(rows: [][]str) -> str
csv.to_str_opts(rows: [][]str, opts: CsvOpts) -> str
csv.to_str_header(headers: []str, rows: [][]str) -> str

csv.write_file(path: str, rows: [][]str) -> !void
csv.write_file_header(path: str, headers: []str, rows: [][]str) -> !void
```

### Example

```rust
@import ( csv = @std.csv )

fn main() -> !void
{
    headers, rows := try csv.parse_header_file("data.csv")
    for h => headers { @pl(h) }
    for row => rows {
        for cell => row { @pf("{cell}  ") }
        @pl("")
    }
}
```

---

## `std.xml` — XML

Parse and produce XML. Light DOM — no schema validation, no XPath.
No external dependency (bundled expat or pugixml header-only).

```rust
@import ( xml = @std.xml )
```

### Types

```rust
type XmlNode = i64   # element node (tag + attrs + children + text)
```

### Parsing

```rust
xml.parse(s: str) -> !XmlNode
xml.parse_file(path: str) -> !XmlNode
```

### Serialisation

```rust
xml.to_str(n: XmlNode) -> str
xml.to_str_pretty(n: XmlNode, indent: i32) -> str
xml.write_file(n: XmlNode, path: str) -> !void
```

### Reading

```rust
xml.tag(n: XmlNode) -> str              # element tag name
xml.text(n: XmlNode) -> str             # inner text (direct, not recursive)
xml.text_all(n: XmlNode) -> str         # all descendant text concatenated
xml.attr(n: XmlNode, key: str) -> str   # attribute value or ""
xml.attrs(n: XmlNode) -> []str          # ["key=val", ...] pairs
xml.has_attr(n: XmlNode, key: str) -> bool
xml.children(n: XmlNode) -> []XmlNode
xml.child_count(n: XmlNode) -> i32
xml.parent(n: XmlNode) -> !XmlNode      # XmlError.NoParent at root

# Search descendants
xml.find(n: XmlNode, tag: str) -> !XmlNode         # first match
xml.find_all(n: XmlNode, tag: str) -> []XmlNode
xml.find_attr(n: XmlNode, attr: str, val: str) -> !XmlNode
```

### Building

```rust
xml.node(tag: str) -> XmlNode
xml.node_text(tag: str, text: str) -> XmlNode
xml.attr_set(n: XmlNode, key: str, val: str)
xml.attr_del(n: XmlNode, key: str)
xml.child_add(parent: XmlNode, child: XmlNode)
xml.child_remove(parent: XmlNode, child: XmlNode)
xml.set_text(n: XmlNode, text: str)
```

### Error Set

```rust
err XmlError { ParseFailed, IOFailed, NoParent, NotFound }
```

### Example

```rust
@import ( xml = @std.xml )

fn main() -> !void
{
    doc := try xml.parse_file("config.xml")
    host := try xml.find(doc, "host")
    @pl(xml.text(host))

    # build
    root := xml.node("config")
    xml.child_add(root, xml.node_text("env", "production"))
    @pl(xml.to_str_pretty(root, 2))
}
```

---

## `std.test` — Test Framework

Minimal test runner. Test functions are named `test_*` and discovered
automatically by `olrn test`. No external dependency.

```rust
@import ( t = @std.test )
```

### Assertions

```rust
t.assert(cond: bool, msg: str)         # fail with msg if false
t.assert_eq(a: any, b: any)            # fail if a != b
t.assert_ne(a: any, b: any)            # fail if a == b
t.assert_lt(a: any, b: any)
t.assert_le(a: any, b: any)
t.assert_gt(a: any, b: any)
t.assert_ge(a: any, b: any)
t.assert_near(a: f64, b: f64, eps: f64)   # |a-b| <= eps
t.assert_ok(result: any)               # result must not be an error
t.assert_err(result: any)              # result must be an error
t.assert_contains(s: str, sub: str)    # substring check
t.assert_match(s: str, pattern: str)   # regex match
```

### Test Control

```rust
t.fail(msg: str)     # unconditional failure
t.skip(msg: str)     # skip this test with a reason
t.log(msg: str)      # print only when the test fails
```

### Test Functions

Any function whose name starts with `test_` at the top level of a file is
automatically picked up by `olrn test`. No registration needed.

```rust
fn test_add() -> void
{
    t.assert_eq(1 + 1, 2)
    t.assert_ne(1 + 1, 3)
}

fn test_parse() -> void
{
    result := parse_int("abc")
    t.assert_err(result)

    ok := parse_int("42")
    t.assert_ok(ok)
}
```

### Running Tests

```sh
olrn test                    # run all test_* in current project
olrn test file.olrn          # run tests in a specific file
olrn test --filter add       # run only tests whose name contains "add"
olrn test --verbose          # show passing tests too
```

Output:

```
running 12 tests
  ok  test_add          (0.1ms)
  ok  test_parse        (0.2ms)
  FAIL test_network     (12ms)
       assert_eq failed: expected 200, got 404
       at tests/net_test.olrn:34

12 tests: 11 passed, 1 failed
```

---

## `std.rand` — Extended Random

Pseudo-random utilities beyond the `@rng` builtin. Uses the Xoshiro256++
PRNG — fast and high-quality, but **not cryptographically secure**. For
security use `crypt.rand_bytes` from `@std.pelentar`.

```rust
@import ( rand = @std.rand )
```

### Seeding

```rust
rand.seed(n: u64)     # manual seed — deterministic output (useful for tests)
rand.seed_time()      # seed from current time (called automatically on first use)
```

### Scalar

```rust
rand.int(lo: i64, hi: i64) -> i64       # inclusive range [lo, hi]
rand.uint(lo: u64, hi: u64) -> u64
rand.float(lo: f64, hi: f64) -> f64     # [lo, hi)
rand.bool() -> bool                      # 50/50
rand.byte() -> u8
rand.bytes(n: i32) -> str               # n random bytes (NOT crypto-safe)
```

### Collections

```rust
rand.shuffle(arr: []any)                     # Fisher-Yates in place
rand.choice(arr: []any) -> any               # random element
rand.sample(arr: []any, n: i32) -> []any     # n unique elements without replacement
rand.weighted(weights: []f64) -> i32         # index proportional to weight
```

### Distributions

```rust
rand.normal(mean: f64, std_dev: f64) -> f64   # Gaussian
rand.exp(lambda: f64) -> f64                  # exponential
rand.bernoulli(p: f64) -> bool                # true with probability p
```

### Example

```rust
@import ( rand = @std.rand )

fn main() -> void
{
    rand.seed(42)   # reproducible

    @pl(@str(rand.int(1, 6)))      # dice roll

    arr :[]i32 = [1, 2, 3, 4, 5]
    rand.shuffle(arr)
    for v => arr { @pf("{v} ") }
    @pl("")

    @pl(@str(rand.normal(0.0, 1.0)))   # standard normal sample
}
```

---

# System Dependencies

| Module | Library | pkg-config | Arch | apt |
|---|---|---|---|---|
| `std.proc` | POSIX / Win32 | — | — | — |
| `std.bytes` | C stdlib | — | — | — |
| `std.date` | C stdlib | — | — | — |
| `std.uuid` | C stdlib + CSPRNG | — | — | — |
| `std.toml` | header-only | — | — | — |
| `std.ws` | libwebsockets | `libwebsockets` | `libwebsockets` | `libwebsockets-dev` |
| `std.csv` | C stdlib | — | — | — |
| `std.xml` | header-only | — | — | — |
| `std.test` | C stdlib | — | — | — |
| `std.rand` | C stdlib | — | — | — |
