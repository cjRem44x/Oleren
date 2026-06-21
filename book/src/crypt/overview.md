# Crypt — Cryptography Library

Crypt is Oleren's built-in cryptography library backed by [libsodium](https://libsodium.org). It provides password hashing, symmetric and asymmetric encryption, digital signatures, key derivation, and encoding utilities — all with a single import.

## Import

```olrn
@import ( crypt = @std.crypt )
```

The alias `crypt` is conventional but not required.

## System requirement

libsodium must be installed on the build machine. The compiler links it automatically when `@std.crypt` is imported:

```sh
sudo pacman -S libsodium      # Arch
sudo apt install libsodium-dev # Debian / Ubuntu
brew install libsodium         # macOS
```

## Error type

All fallible Crypt functions return `CryptError!T`. The error set is:

| Variant | When raised |
|---|---|
| `InitFailed` | libsodium could not initialize |
| `BadKey` | key has wrong length or format |
| `BadInput` | input data is malformed or empty |
| `DecryptFailed` | authentication tag mismatch |
| `HashFailed` | hashing operation failed |
| `IOFailed` | file read / write error |

Use `try` to propagate or `catch` to handle inline:

```olrn
hash :: try crypt.crypt_hashpk(0, password)

# or inline
hash :: crypt.crypt_hash(0, data) catch |e| {
    io.println("hash error")
    ret
}
```

## Key types

Two struct types are returned by key-generation functions. They are defined in the C++ backend and are available in any file that imports `@std.crypt`:

| Type | Fields |
|---|---|
| `CryptKeyPair` | `pub_key: str`, `sec_key: str` |
| `CryptSignKeyPair` | `verify_key: str`, `sign_key: str` |

## Feature overview

| Category | Functions |
|---|---|
| Password hashing | `crypt_hashpk`, `crypt_auth_hashpk` |
| General hashing | `crypt_hash`, `crypt_hash_file`, `crypt_hmac` |
| Symmetric encryption | `crypt_keygen`, `crypt_encrypt`, `crypt_decrypt` |
| File encryption | `crypt_enc_file`, `crypt_dec_file` |
| Key derivation | `crypt_kdf` |
| Public-key encryption | `crypt_box_keygen`, `crypt_box_encrypt`, `crypt_box_decrypt` |
| Signing | `crypt_sign_keygen`, `crypt_sign`, `crypt_verify` |
| Random | `crypt_rand_bytes`, `crypt_rand_hex` |
| Encoding | `crypt_base64_encode/decode`, `crypt_hex_encode/decode` |
