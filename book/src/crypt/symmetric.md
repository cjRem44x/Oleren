# Symmetric Encryption

Symmetric encryption uses the same key for both encryption and decryption. It is the right choice when you control both sides — encrypting local data, files, or messages between parties that already share a secret.

Crypt uses XChaCha20-Poly1305 authenticated encryption. Every ciphertext includes an authentication tag; decryption fails with `DecryptFailed` if the data has been tampered with.

## Key generation

```olrn
fn crypt_keygen() -> str
```

Generates a random 32-byte symmetric key. Store it securely — anyone with the key can decrypt.

```olrn
key :: crypt.crypt_keygen()
# save `key` somewhere safe
```

## Encrypt and decrypt

```olrn
fn crypt_encrypt(key: str, plain: str) -> CryptError!str
fn crypt_decrypt(key: str, ct: str) -> CryptError!str
```

`crypt_encrypt` returns a ciphertext that includes a random nonce prepended to the encrypted payload. `crypt_decrypt` extracts the nonce and decrypts. The same key with different plaintexts produces different ciphertexts each call (nonce is random per call).

```olrn
@import ( crypt = @std.crypt, io = @std.io )

fn main() -> !void {
    key :: crypt.crypt_keygen()
    plain :: "secret message"

    ct :: try crypt.crypt_encrypt(key, plain)
    io.println("ciphertext (raw bytes, not printable)")

    recovered :: try crypt.crypt_decrypt(key, ct)
    io.println(recovered)   # "secret message"
}
```

## File encryption

```olrn
fn crypt_enc_file(s: i32, path: str, passphrase: str) -> CryptError!void
fn crypt_dec_file(s: i32, path: str, passphrase: str) -> CryptError!void
```

Encrypts or decrypts a file in-place using a passphrase. The passphrase is run through Argon2id KDF (strength `s`) to derive the encryption key; a random 16-byte salt and 24-byte nonce are stored in the file header.

File format: `PLNT` magic | ver | stren | cipher | 16-byte salt | 24-byte nonce | ciphertext

```olrn
@import ( crypt = @std.crypt )

fn main() -> !void {
    try crypt.crypt_enc_file(1, "save.dat", "mypassphrase")
    # save.dat is now encrypted

    try crypt.crypt_dec_file(1, "save.dat", "mypassphrase")
    # save.dat is restored
}
```

The `s` strength must match between encrypt and decrypt calls.

## Key derivation (KDF)

```olrn
fn crypt_kdf(s: i32, passphrase: str, salt: str, keylen: i32) -> CryptError!str
```

Derives a key of `keylen` bytes from a passphrase and salt using Argon2id. Use this when you need a reproducible key from a passphrase — for example, to share an encryption key between devices without transmitting it.

```olrn
salt :: crypt.crypt_rand_bytes(16)   # generate once, store alongside the ciphertext
key  :: try crypt.crypt_kdf(1, "passphrase", salt, 32)
ct   :: try crypt.crypt_encrypt(key, "data")
```

To decrypt later: retrieve the salt, derive the same key, decrypt.

> Always use a fresh random salt per derived key. Never reuse a salt for different passphrases or contexts.
