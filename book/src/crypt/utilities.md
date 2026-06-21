# Utilities

## Random data

```olrn
fn crypt_rand_bytes(n: i32) -> str
fn crypt_rand_hex(n: i32) -> str
```

`crypt_rand_bytes` returns `n` cryptographically random bytes as a raw `str`. `crypt_rand_hex` returns `n` random bytes encoded as lowercase hex (output length is `2*n` characters).

Both use libsodium's CSPRNG, which seeds from the OS entropy source.

```olrn
@import ( crypt = @std.crypt, io = @std.io )

fn main() {
    token :: crypt.crypt_rand_hex(16)    # 32-character hex token
    io.println(token)

    salt :: crypt.crypt_rand_bytes(16)   # raw 16-byte salt for KDF
}
```

Use `crypt_rand_bytes` for keys, salts, and nonces. Use `crypt_rand_hex` for tokens, session IDs, or anything that must be printable without further encoding.

## Base64 encoding

```olrn
fn crypt_base64_encode(data: str) -> str
fn crypt_base64_decode(b64: str) -> CryptError!str
```

Standard base64 (RFC 4648). Use this to represent binary data (keys, ciphertexts, signatures, hashes) as ASCII-safe text for storage in JSON, databases, or HTTP headers.

```olrn
key    :: crypt.crypt_keygen()
stored :: crypt.crypt_base64_encode(key)
# stored == "aGVsbG8gd29ybGQ=" (example)

key2 :: try crypt.crypt_base64_decode(stored)
# key2 == key
```

`crypt_base64_decode` returns `BadInput` if the input is not valid base64.

## Hex encoding

```olrn
fn crypt_hex_encode(data: str) -> str
fn crypt_hex_decode(hex: str) -> CryptError!str
```

Lowercase hex. Useful for checksums, digests, and any context where a compact printable representation is needed and base64 would be too wide.

```olrn
raw :: crypt.crypt_hash(0, "some data")
hex :: crypt.crypt_hex_encode(raw)
io.println(hex)   # e.g. "a94f5374..."

back :: try crypt.crypt_hex_decode(hex)
# back == raw
```

`crypt_hex_decode` returns `BadInput` if the string contains non-hex characters or has an odd length.

## Choosing an encoding

| Need | Use |
|---|---|
| Shortest printable form | `crypt_base64_encode` |
| Human-readable digest / checksum | `crypt_hex_encode` |
| Random printable token | `crypt_rand_hex(n)` |
| Raw binary (for further crypto ops) | keep as `str` — no encoding |

## Quick-reference table

| Function | Input | Output |
|---|---|---|
| `crypt_rand_bytes(n)` | length | `n` random bytes |
| `crypt_rand_hex(n)` | length | `2n` hex chars |
| `crypt_base64_encode(data)` | raw bytes | base64 string |
| `crypt_base64_decode(b64)` | base64 string | `CryptError!str` raw bytes |
| `crypt_hex_encode(data)` | raw bytes | hex string |
| `crypt_hex_decode(hex)` | hex string | `CryptError!str` raw bytes |
