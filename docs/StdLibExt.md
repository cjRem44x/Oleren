# Oleren Standard Library — Extended Modules

Tier 1 standard library modules. All are imported via `@std.<module>` and
use the flat `module_fn()` naming convention. Sections below marked
**Planned** are design spec, not yet implemented.

---

## `std.env` — Environment & Process

> **Planned**

Access command-line arguments, environment variables, and process state.
No external dependency — backed by C stdlib and POSIX.

```rust
@import ( env = @std.env )
```

### Arguments

```rust
env.args() -> []str          # all argv as a slice (index 0 is exe path)
env.arg(i: i32) -> str       # single arg; "" if out of range
env.arg_count() -> i32       # argc
```

### Environment Variables

```rust
env.get(key: str) -> str           # value or "" if not set
env.get_or(key: str, def: str) -> str
env.set(key: str, val: str)
env.del(key: str)
env.has(key: str) -> bool
env.all() -> []str                 # all vars as "KEY=VAL" strings
```

### Process

```rust
env.exit(code: i32)      # terminate immediately with exit code
env.pid() -> i32          # current process ID
env.exe() -> str          # absolute path to the running executable
env.cwd() -> str          # current working directory (alias: std.fs.cwd)
env.set_cwd(path: str) -> !void
```

### Example

```rust
@import ( env = @std.env )

fn main() -> void
{
    if env.arg_count() < 2 {
        @pl("usage: " + env.arg(0) + " <name>")
        env.exit(1)
    }
    name :: env.get_or("GREETING", "hello")
    @pf("{name}, {arg}\n") # note: this is pseudocode; real Oleren needs vars
    @pl(name + ", " + env.arg(1))
}
```

---

## `std.path` — Path Manipulation

> **Planned**

Pure string operations on filesystem paths. No I/O — for actual filesystem
calls see `std.fs`. Separator is always `/` internally; Windows paths are
normalised on input.

```rust
@import ( path = @std.path )
```

### Decomposition

```rust
path.dirname(p: str) -> str      # "/a/b/c.txt" → "/a/b"
path.basename(p: str) -> str     # "/a/b/c.txt" → "c.txt"
path.stem(p: str) -> str         # "/a/b/c.txt" → "c"
path.ext(p: str) -> str          # "/a/b/c.txt" → ".txt"  (includes dot)
path.split(p: str) -> (str, str) # (dirname, basename)
path.split_ext(p: str) -> (str, str)  # ("path/c", ".txt")
```

### Construction

```rust
path.join(a: str, b: str) -> str          # join two segments
path.join3(a: str, b: str, c: str) -> str
path.with_ext(p: str, ext: str) -> str    # replace or add extension
path.with_name(p: str, name: str) -> str  # replace basename
```

### Normalisation

```rust
path.normalize(p: str) -> str    # collapse .., ., double slashes
path.abs(p: str) -> !str         # resolve relative to cwd; IOFailed on error
path.rel(p: str, base: str) -> !str   # make p relative to base
path.is_abs(p: str) -> bool
path.is_rel(p: str) -> bool
```

### Glob

```rust
# Match a path against a glob pattern (* = any chars, ** = any depth)
path.glob_match(pattern: str, p: str) -> bool

# Expand a glob pattern against the filesystem — returns matching paths
path.glob(pattern: str) -> ![]str
```

### Example

```rust
@import ( path = @std.path )

fn process(file: str) -> void
{
    dir  :: path.dirname(file)
    name :: path.stem(file)
    ext  :: path.ext(file)
    out  :: path.join(dir, name + "_out" + ext)
    @pl(out)
}
```

---

## `std.json` — JSON

> **Planned**

Parse and produce JSON. The `Json` type is a tagged value that can hold
any JSON primitive or container.

```rust
@import ( json = @std.json )
```

### The Json Type

```rust
# Opaque tagged value — inspect with the helpers below.
# Internally a std::variant wrapping null/bool/i64/f64/str/array/object.
type Json = i64
```

### Parsing

```rust
json.parse(s: str) -> !Json         # parse JSON from a string
json.parse_file(path: str) -> !Json # read and parse a file
```

### Serialisation

```rust
json.to_str(j: Json) -> str                    # compact JSON
json.to_str_pretty(j: Json, indent: i32) -> str   # indented
```

### Reading Values

```rust
json.is_null(j: Json)  -> bool
json.is_bool(j: Json)  -> bool
json.is_int(j: Json)   -> bool
json.is_float(j: Json) -> bool
json.is_str(j: Json)   -> bool
json.is_arr(j: Json)   -> bool
json.is_obj(j: Json)   -> bool

json.as_bool(j: Json)  -> !bool
json.as_int(j: Json)   -> !i64
json.as_float(j: Json) -> !f64
json.as_str(j: Json)   -> !str

json.len(j: Json) -> i32           # array length or object key count

# Object access
json.has(j: Json, key: str) -> bool
json.get(j: Json, key: str) -> !Json   # JsonError.Missing if absent
json.keys(j: Json) -> []str

# Array access
json.at(j: Json, i: i32) -> !Json    # JsonError.OutOfRange if absent
```

### Building Values

```rust
json.null()                   -> Json
json.bool(v: bool)            -> Json
json.int(v: i64)              -> Json
json.float(v: f64)            -> Json
json.str(v: str)              -> Json
json.arr()                    -> Json   # empty array
json.obj()                    -> Json   # empty object

json.arr_push(j: Json, val: Json)           # append to array
json.obj_set(j: Json, key: str, val: Json)  # set object field
json.obj_del(j: Json, key: str)             # remove field
```

### Error Set

```rust
err JsonError { ParseFailed, TypeMismatch, Missing, OutOfRange, IOFailed }
```

### Example

```rust
@import ( json = @std.json )

fn main() -> !void
{
    src :: "{\"name\":\"Oleren\",\"version\":3,\"stable\":true}"
    j   := try json.parse(src)

    name    := try json.as_str(try json.get(j, "name"))
    version := try json.as_int(try json.get(j, "version"))
    @pl(name + " v" + @str(version))

    # build
    out := json.obj()
    json.obj_set(out, "lang",   json.str("Oleren"))
    json.obj_set(out, "tier",   json.int(1))
    json.obj_set(out, "active", json.bool(true))
    @pl(json.to_str_pretty(out, 2))
}
```

---

## `std.net` — Sockets

> **Planned**

TCP/UDP networking and DNS. Backed by POSIX sockets on Linux/macOS and
Winsock2 on Windows.

```rust
@import ( net = @std.net )
```

### Types

```rust
type TcpConn     = i64
type TcpListener = i64
type UdpSock     = i64
```

### TCP Client

```rust
net.tcp_connect(addr: str) -> !TcpConn  # "host:port"
net.tcp_close(c: TcpConn)

net.tcp_read(c: TcpConn, n: i32) -> !str        # up to n bytes
net.tcp_read_exact(c: TcpConn, n: i32) -> !str  # exactly n bytes
net.tcp_read_line(c: TcpConn) -> !str           # read until \n
net.tcp_read_all(c: TcpConn) -> !str            # read until EOF

net.tcp_write(c: TcpConn, data: str) -> !void
net.tcp_write_line(c: TcpConn, line: str) -> !void  # appends \n

net.tcp_set_timeout(c: TcpConn, secs: f64)   # 0 = no timeout
net.tcp_set_nodelay(c: TcpConn, on: bool)    # disable Nagle
net.tcp_peer_addr(c: TcpConn) -> str          # "ip:port"
net.tcp_local_addr(c: TcpConn) -> str
```

### TCP Server

```rust
net.tcp_listen(addr: str) -> !TcpListener     # "0.0.0.0:8080"
net.tcp_accept(l: TcpListener) -> !TcpConn    # blocks until connection
net.tcp_listener_close(l: TcpListener)
net.tcp_listener_addr(l: TcpListener) -> str
```

### UDP

```rust
net.udp_bind(addr: str) -> !UdpSock
net.udp_close(s: UdpSock)

net.udp_send(s: UdpSock, addr: str, data: str) -> !void
net.udp_recv(s: UdpSock, n: i32) -> !(str, str)  # (data, from_addr)
net.udp_set_timeout(s: UdpSock, secs: f64)
net.udp_set_broadcast(s: UdpSock, on: bool)
```

### DNS

```rust
net.dns_lookup(host: str) -> ![]str    # A/AAAA records — list of IPs
net.dns_reverse(ip: str) -> !str       # PTR record
```

### Error Set

```rust
err NetError {
    ConnRefused, ConnTimeout, ConnReset,
    HostNotFound, AddrInUse,
    ReadFailed, WriteFailed,
    Closed, InvalidAddr,
}
```

### Example

```rust
@import ( net = @std.net )

fn main() -> !void
{
    c := try net.tcp_connect("example.com:80")
    defer net.tcp_close(c)

    try net.tcp_write(c, "GET / HTTP/1.0\r\nHost: example.com\r\n\r\n")
    resp := try net.tcp_read_all(c)
    @pl(resp)
}
```

---

## `std.http` — HTTP Client

> **Planned**

HTTP/HTTPS client backed by **libcurl**. Supports GET, POST, and a full
request builder for headers, auth, timeouts, and file downloads.

```rust
@import ( http = @std.http )
```

### Response Type

```rust
struct Response {
    status:  i32,
    body:    str,
}

http.resp_header(r: Response, key: str) -> str  # "" if absent
http.resp_ok(r: Response) -> bool               # status 200-299
```

### Simple Requests

```rust
http.get(url: str) -> !Response
http.post(url: str, body: str, content_type: str) -> !Response
http.put(url: str, body: str, content_type: str) -> !Response
http.patch(url: str, body: str, content_type: str) -> !Response
http.delete(url: str) -> !Response

# Convenience: POST with JSON body
http.post_json(url: str, json_body: str) -> !Response
```

### Request Builder

```rust
type Request = i64

http.request(method: str, url: str) -> Request
http.req_header(r: Request, key: str, val: str) -> Request  # chainable
http.req_body(r: Request, data: str) -> Request
http.req_body_json(r: Request, json: str) -> Request
http.req_auth_basic(r: Request, user: str, pass: str) -> Request
http.req_auth_bearer(r: Request, token: str) -> Request
http.req_timeout(r: Request, secs: f64) -> Request
http.req_follow(r: Request, on: bool) -> Request  # follow redirects
http.req_send(r: Request) -> !Response
```

### File Download

```rust
# Stream a URL directly to disk — avoids loading entire body into memory
http.download(url: str, dest_path: str) -> !void
http.download_progress(url: str, dest_path: str,
                       fn(received: i64, total: i64) -> void) -> !void
```

### Error Set

```rust
err HttpError {
    ConnFailed, Timeout, TlsFailed,
    BadUrl, TooManyRedirects,
    IOFailed, BadResponse,
}
```

### Example

```rust
@import ( http = @std.http, json = @std.json )

fn main() -> !void
{
    # simple GET
    r := try http.get("https://api.github.com/zen")
    @pl(r.body)

    # POST JSON with auth
    payload :: "{\"title\":\"bug\",\"body\":\"details\"}"
    resp := try http.request("POST", "https://api.github.com/repos/x/y/issues")
        |> http.req_auth_bearer("ghp_token")
        |> http.req_body_json(payload)
        |> http.req_send()
    @pl(@str(resp.status))

    # download a file
    try http.download("https://example.com/archive.zip", "/tmp/archive.zip")
}
```

---

## `std.compress` — Compression

> **Planned**

In-memory and file-level compression. Backends: **zlib** (gzip/zlib),
**libzstd** (zstd).

```rust
@import ( compress = @std.compress )
```

### Algorithm Enum

```rust
enum algo => i32 {
    GZIP = 0,
    ZLIB = 1,
    ZSTD = 2,
}
```

### In-Memory

```rust
compress.encode(a: algo, data: str) -> !str
compress.decode(a: algo, data: str) -> !str

# zstd-only: level 1 (fast) to 22 (best); default is 3
compress.encode_level(data: str, level: i32) -> !str
```

### Files

```rust
compress.compress_file(a: algo, src: str, dst: str) -> !void
compress.decompress_file(a: algo, src: str, dst: str) -> !void

# Convenience: compress → "path.gz" / "path.zst"; decompress strips suffix
compress.gzip_file(src: str) -> !void       # produces src.gz
compress.ungzip_file(src: str) -> !void     # src must end in .gz
compress.zstd_file(src: str) -> !void       # produces src.zst
compress.unzstd_file(src: str) -> !void
```

### Error Set

```rust
err CompressError { EncodeFailed, DecodeFailed, IOFailed, BadData }
```

### Example

```rust
@import ( compress = @std.compress )

fn main() -> !void
{
    data :: "hello world! " * 1000   # hypothetical repeat
    gz   := try compress.encode(compress.algo.GZIP, data)
    @pl("compressed: " + @str(gz.len) + " bytes")

    back := try compress.decode(compress.algo.GZIP, gz)
    @pl("restored:   " + @str(back.len) + " bytes")

    # compress a file to disk
    try compress.zstd_file("build/output.bin")
    # produces build/output.bin.zst
}
```

---

## `std.regex` — Regular Expressions

> **Planned**

Pattern matching, capture groups, find, replace, split. Backed by C++17
`std::regex` (no external dependency). For performance-critical hot paths
use a compiled `Regex` handle.

```rust
@import ( re = @std.regex )
```

### One-Shot (compile + run)

```rust
re.match(pattern: str, text: str) -> bool    # full-string match
re.search(pattern: str, text: str) -> bool   # match anywhere in text
re.find(pattern: str, text: str) -> str      # first match or ""
re.find_all(pattern: str, text: str) -> []str
re.captures(pattern: str, text: str) -> []str  # [full, group1, group2, ...]
re.captures_all(pattern: str, text: str) -> [][]str
re.replace(pattern: str, text: str, repl: str) -> str     # first match
re.replace_all(pattern: str, text: str, repl: str) -> str
re.split(pattern: str, text: str) -> []str
```

### Compiled Regex (reuse across calls)

```rust
type Regex = i64

re.compile(pattern: str) -> !Regex
re.free(r: Regex)

re.regex_match(r: Regex, text: str) -> bool
re.regex_search(r: Regex, text: str) -> bool
re.regex_find(r: Regex, text: str) -> str
re.regex_find_all(r: Regex, text: str) -> []str
re.regex_captures(r: Regex, text: str) -> []str
re.regex_captures_all(r: Regex, text: str) -> [][]str
re.regex_replace(r: Regex, text: str, repl: str) -> str
re.regex_replace_all(r: Regex, text: str, repl: str) -> str
re.regex_split(r: Regex, text: str) -> []str
```

### Replacement Syntax

`$0` — full match, `$1` `$2` … — capture groups, `$name` — named group.

```rust
# Named captures: (?P<name>pattern)
r   := try re.compile("(?P<year>\\d{4})-(?P<month>\\d{2})-(?P<day>\\d{2})")
out :: re.regex_replace_all(r, "born 1991-04-23", "$day/$month/$year")
# → "born 23/04/1991"
```

### Error Set

```rust
err RegexError { BadPattern, IOFailed }
```

### Example

```rust
@import ( re = @std.regex )

fn main() -> !void
{
    emails  := re.find_all("[a-zA-Z0-9._%+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z]{2,}",
                           "contact bob@example.com or alice@test.org")
    for e => emails { @pl(e) }

    # compiled for reuse
    ip_pat := try re.compile("\\b(\\d{1,3}\\.){3}\\d{1,3}\\b")
    defer re.free(ip_pat)

    ok :: re.regex_search(ip_pat, "server at 192.168.1.1")
    @pl("has ip: " + @str(ok))
}
```

---

## System Dependencies

| Module | Library | pkg-config | Arch | apt |
|---|---|---|---|---|
| `std.env` | C stdlib | — | — | — |
| `std.path` | C stdlib | — | — | — |
| `std.json` | header-only | — | — | — |
| `std.net` | POSIX / Winsock | — | — | — |
| `std.http` | libcurl | `libcurl` | `curl` | `libcurl4-openssl-dev` |
| `std.compress` | zlib + libzstd | `zlib`, `libzstd` | `zlib`, `zstd` | `zlib1g-dev`, `libzstd-dev` |
| `std.regex` | C++17 std::regex | — | — | — |
