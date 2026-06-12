# Pelentar — Oleren Cryptography Library

## Philosophy

Pelentar is the built-in cryptography library for Oleren. It exposes two
layers:

- **Easy API** — opinionated, hard to misuse. One enum controls security
  level; algorithms are chosen for you. Use these unless you have a specific
  reason not to.
- **Low-level API** — direct access to primitives (hash, encrypt, sign,
  encode) with explicit algorithm selection.

All keys, hashes, and ciphertext are returned as `str` in hex or base64.
Files are encrypted in-place; originals are overwritten (see enc_file /
dec_file for the convention).

Backend: **libsodium** (Argon2id, XChaCha20-Poly1305, Ed25519, BLAKE2b).
Importing `@std.pelentar` auto-links `-lsodium` via pkg-config; if libsodium
is not installed the build stops with OS-specific install instructions.

```rust
@import ( crypt = @std.pelentar )
```

---

## Status

> **Planned** — not yet implemented. This document is the API spec.

---

## Security Level Enum

All easy-API functions accept a `stren` (strength) enum that selects the
cost parameters without exposing algorithm internals.

```rust
enum stren => i32 {
    FAST    = 0,   # interactive / low-latency (login forms, API calls)
    DEFAULT = 1,   # moderate (recommended for most uses)
    STRONG  = 2,   # sensitive / offline-attack-resistant (key derivation, exports)
}
```

| Level | Argon2id ops | Argon2id mem | When to use |
|---|---|---|---|
| `FAST` | `OPSLIMIT_INTERACTIVE` | `MEMLIMIT_INTERACTIVE` (64 MB) | High-frequency auth, low-power devices |
| `DEFAULT` | `OPSLIMIT_MODERATE` | `MEMLIMIT_MODERATE` (256 MB) | General-purpose password storage |
| `STRONG` | `OPSLIMIT_SENSITIVE` | `MEMLIMIT_SENSITIVE` (1 GB) | Master keys, archive encryption, exports |

For file encryption the `stren` level controls key derivation cost (same
Argon2id table above); the cipher is always XChaCha20-Poly1305 regardless
of strength.

---

## Easy API

### Password Hashing

```rust
# Hash a plaintext password. Returns a self-contained encoded string that
# includes algorithm, parameters, and salt — safe to store directly in a DB.
# Uses Argon2id with parameters matching `stren`.
fn crypt.hashpk(stren: stren, plain: str) -> !str

# Verify a plaintext password against a stored hash from hashpk().
# Returns true if the password matches. Timing-safe comparison.
fn crypt.auth_hashpk(stren: stren, plain: str, hashed: str) -> !bool
```

**Example:**

```rust
@import ( crypt = @std.pelentar )

fn register(password: str) -> !str
{
    ret try crypt.hashpk(crypt.stren.DEFAULT, password)
}

fn login(password: str, stored_hash: str) -> !bool
{
    ret try crypt.auth_hashpk(crypt.stren.DEFAULT, password, stored_hash)
}
```

> `stren` passed to `auth_hashpk` is a hint for the re-hash budget —
> the actual parameters are embedded in the hash string, so the verification
> always uses the original cost regardless of the `stren` argument.

---

### File Encryption / Decryption

```rust
# Encrypt a file in-place. The original file is replaced with ciphertext.
# A random nonce is prepended; `pk` is a passphrase that is key-derived
# at the cost specified by `stren` (Argon2id). The encrypted file gets
# a `.plntr` extension (e.g. "data.bin" → "data.bin.plntr").
fn crypt.enc_file(stren: stren, path: str, pk: str) -> !void

# Decrypt a `.plntr` file back to plaintext. The decrypted file replaces
# the encrypted one (strips the `.plntr` extension).
fn crypt.dec_file(stren: stren, path: str, pk: str) -> !void
```

**Example:**

```rust
@import ( crypt = @std.pelentar )

fn backup(path: str, passphrase: str) -> !void
{
    try crypt.enc_file(crypt.stren.STRONG, path, passphrase)
    # path is now "path.plntr", original overwritten
}

fn restore(path: str, passphrase: str) -> !void
{
    # path should be "something.plntr"
    try crypt.dec_file(crypt.stren.STRONG, path, passphrase)
}
```

**File format** (binary layout of `.plntr` files):

```
[4 bytes magic: "PLNT"]
[1 byte version: 0x01]
[1 byte stren: 0=FAST 1=DEFAULT 2=STRONG]
[32 bytes Argon2id salt]
[24 bytes XChaCha20-Poly1305 nonce]
[N bytes ciphertext + 16 bytes Poly1305 tag]
```

The header contains everything needed to decrypt; the passphrase is the
only external input required.

---

## Low-Level API

### Hashing

Non-cryptographic integrity checking → SHA-256/512. Cryptographic
integrity → BLAKE2b. Password storage → use `hashpk`, never these.

```rust
enum hash_algo => i32 {
    SHA256  = 0,
    SHA512  = 1,
    BLAKE2B = 2,   # default recommendation
}

# Hash arbitrary data. Returns lowercase hex string.
fn crypt.hash(algo: hash_algo, data: str) -> str

# Hash a file by path. Returns lowercase hex string.
fn crypt.hash_file(algo: hash_algo, path: str) -> !str

# HMAC — keyed hash for message authentication.
fn crypt.hmac(algo: hash_algo, key: str, data: str) -> str
```

**Example:**

```rust
digest  :: crypt.hash(crypt.hash_algo.BLAKE2B, "hello world")
mac     :: crypt.hmac(crypt.hash_algo.SHA256, secret_key, payload)
chk_sum := try crypt.hash_file(crypt.hash_algo.SHA256, "archive.tar")
```

---

### Symmetric Encryption

Authenticated encryption (AEAD) — confidentiality + integrity in one step.
Uses XChaCha20-Poly1305 internally.

```rust
# Generate a random 32-byte secret key. Returns hex string (64 chars).
fn crypt.keygen() -> str

# Encrypt plaintext with a hex key. Returns base64-encoded ciphertext
# with a random nonce prepended (nonce + ciphertext + tag, all in one blob).
fn crypt.encrypt(key: str, plaintext: str) -> !str

# Decrypt a base64 blob produced by encrypt(). Returns the original plaintext.
fn crypt.decrypt(key: str, ciphertext: str) -> !str
```

**Example:**

```rust
key        :: crypt.keygen()
ciphertext := try crypt.encrypt(key, "secret message")
plaintext  := try crypt.decrypt(key, ciphertext)
```

---

### Asymmetric Encryption (Box)

Public-key authenticated encryption between two parties (X25519 + XSalsa20).

```rust
struct KeyPair {
    pub_key: str,   # hex-encoded X25519 public key  (64 chars)
    sec_key: str,   # hex-encoded X25519 secret key  (64 chars)
}

# Generate a keypair.
fn crypt.box_keygen() -> KeyPair

# Encrypt for a recipient's public key using our secret key.
# Output is base64-encoded.
fn crypt.box_encrypt(our_sk: str, their_pk: str, plaintext: str) -> !str

# Decrypt using our secret key and the sender's public key.
fn crypt.box_decrypt(our_sk: str, their_pk: str, ciphertext: str) -> !str
```

**Example:**

```rust
alice :: crypt.box_keygen()
bob   :: crypt.box_keygen()

# Alice encrypts for Bob
ct := try crypt.box_encrypt(alice.sec_key, bob.pub_key, "hello bob")

# Bob decrypts from Alice
pt := try crypt.box_decrypt(bob.sec_key, alice.pub_key, ct)
```

---

### Signatures (Ed25519)

```rust
struct SignKeyPair {
    verify_key: str,   # hex-encoded Ed25519 public key (64 chars)
    sign_key:   str,   # hex-encoded Ed25519 secret key (128 chars)
}

# Generate a signing keypair.
fn crypt.sign_keygen() -> SignKeyPair

# Sign a message. Returns hex-encoded 64-byte signature.
fn crypt.sign(sk: str, msg: str) -> str

# Verify a signature. Returns true if valid.
fn crypt.verify(vk: str, msg: str, sig: str) -> bool
```

**Example:**

```rust
kp  :: crypt.sign_keygen()
sig :: crypt.sign(kp.sign_key, "payload")
ok  :: crypt.verify(kp.verify_key, "payload", sig)
```

---

### Key Derivation

Derive a deterministic key from a passphrase and salt (Argon2id).

```rust
# Derive a key of `len` bytes from a passphrase. Salt must be 32 bytes
# (hex-encoded, 64 chars) — generate once with rand_hex(32) and store it.
# Returns hex-encoded key of length `len * 2`.
fn crypt.kdf(stren: stren, passphrase: str, salt: str, len: i32) -> !str
```

---

### Random

Cryptographically secure random bytes (libsodium `randombytes_buf`).

```rust
# `n` random bytes returned as a raw str.
fn crypt.rand_bytes(n: i32) -> str

# `n` random bytes returned as a lowercase hex string (2n chars).
fn crypt.rand_hex(n: i32) -> str
```

---

### Encoding Utilities

```rust
fn crypt.base64_encode(data: str) -> str
fn crypt.base64_decode(data: str) -> !str   # LoadFailed on bad input

fn crypt.hex_encode(data: str) -> str
fn crypt.hex_decode(data: str) -> !str      # LoadFailed on bad input
```

---

## Error Handling

All fallible Pelentar functions return `!T` using the built-in error
mechanism. The error set is:

```rust
err PelentrError {
    InitFailed,    # libsodium init failed
    BadKey,        # key is wrong length or malformed
    BadInput,      # bad base64/hex/format
    DecryptFailed, # authentication tag mismatch (tampering or wrong key)
    HashFailed,    # argon2 / hash operation failed (OOM, bad params)
    IOFailed,      # file read/write error
}
```

---

## System Dependency

| Library | pkg-config | Arch | apt | brew |
|---|---|---|---|---|
| libsodium | `libsodium` | `libsodium` | `libsodium-dev` | `libsodium` |

The build system detects it automatically — `olrn deps` shows status:

```
$ olrn deps
system deps for src/main/olrn/main.olrn
└─ libsodium (@std.pelentar)  found 1.0.20
```

Install if missing:

```sh
# Arch / CachyOS
sudo pacman -S libsodium

# Debian / Ubuntu
sudo apt install libsodium-dev

# macOS
brew install libsodium
```

---

## Full Example

```rust
@import (
    crypt = @std.pelentar,
    io    = @std.io,
)

fn main() -> !void
{
    # --- password hashing ---
    hash := try crypt.hashpk(crypt.stren.DEFAULT, "hunter2")
    ok   := try crypt.auth_hashpk(crypt.stren.DEFAULT, "hunter2", hash)
    @pl("password ok: " + ok)

    # --- symmetric encrypt/decrypt ---
    key := crypt.keygen()
    ct  := try crypt.encrypt(key, "top secret")
    pt  := try crypt.decrypt(key, ct)
    @pl("roundtrip: " + pt)

    # --- file encrypt ---
    try crypt.enc_file(crypt.stren.STRONG, "report.pdf", "my passphrase")
    # report.pdf is now report.pdf.plntr

    try crypt.dec_file(crypt.stren.STRONG, "report.pdf.plntr", "my passphrase")
    # report.pdf.plntr is now report.pdf again

    # --- signing ---
    kp  :: crypt.sign_keygen()
    sig :: crypt.sign(kp.sign_key, "hello")
    @pl("valid: " + crypt.verify(kp.verify_key, "hello", sig))

    # --- hash a file ---
    digest := try crypt.hash_file(crypt.hash_algo.BLAKE2B, "report.pdf")
    @pf("blake2b: {digest}\n")
}
```
