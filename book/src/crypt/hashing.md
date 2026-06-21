# Hashing

## General-purpose hashing

```olrn
fn crypt_hash(algo: i32, data: str) -> str
fn crypt_hash_file(algo: i32, path: str) -> CryptError!str
fn crypt_hmac(algo: i32, key: str, data: str) -> str
```

`algo` selects the algorithm. Pass `0` for BLAKE2b (the only algorithm currently supported; additional variants may be added in future versions).

The returned `str` is raw binary. Pass it through `crypt_hex_encode` or `crypt_base64_encode` to get a printable digest.

```olrn
@import ( crypt = @std.crypt, io = @std.io )

fn main() {
    raw  :: crypt.crypt_hash(0, "hello world")
    hex  :: crypt.crypt_hex_encode(raw)
    io.println(hex)

    # file hash
    fhash :: try crypt.crypt_hash_file(0, "data.bin")
    io.println(crypt.crypt_hex_encode(fhash))

    # HMAC — keyed hash for message authentication
    mac :: crypt.crypt_hmac(0, "secretkey", "the message")
    io.println(crypt.crypt_hex_encode(mac))
}
```

## Password hashing (Argon2id)

```olrn
fn crypt_hashpk(s: i32, plain: str) -> CryptError!str
fn crypt_auth_hashpk(plain: str, hashed: str) -> CryptError!bool
```

Password hashing is intentionally slow to resist brute-force attacks. Use `crypt_hashpk` to hash a password at registration or when storing credentials. Use `crypt_auth_hashpk` to verify a supplied password against a stored hash.

The `s` (strength) parameter controls time and memory cost:

| `s` | Constant | Use case |
|---|---|---|
| `0` | `FAST` | Testing, low-security contexts |
| `1` | `DEFAULT` | Most applications |
| `2` | `STRONG` | High-security (slower, more RAM) |

The stored hash is self-contained — it embeds the algorithm, parameters, and salt. You do not need to store anything else.

```olrn
@import ( crypt = @std.crypt, io = @std.io )

fn main() -> !void {
    # registration — hash once and store
    password :: "hunter2"
    stored   :: try crypt.crypt_hashpk(1, password)
    io.println(stored)   # $argon2id$v=19$...

    # login — verify supplied password against stored hash
    ok :: try crypt.crypt_auth_hashpk(password, stored)
    if ok {
        io.println("password correct")
    } else {
        io.println("wrong password")
    }
}
```

> **Never store plain-text passwords.** Always hash with `crypt_hashpk` and verify with `crypt_auth_hashpk`. Do not use `crypt_hash` for passwords — it is not rate-limited.
