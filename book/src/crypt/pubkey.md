# Public-Key Cryptography

Public-key (asymmetric) encryption lets two parties exchange messages securely without sharing a secret in advance. Each party has a key pair: a public key they can share freely and a secret key they keep private.

Crypt uses X25519 (Curve25519 Diffie-Hellman) for key agreement and XChaCha20-Poly1305 for the actual encryption — the same authenticated cipher as symmetric mode.

## Key pair generation

```olrn
fn crypt_box_keygen() -> CryptKeyPair
```

Returns a `CryptKeyPair` with two fields:

| Field | Description |
|---|---|
| `pub_key` | Public key — share with anyone you want to receive messages from |
| `sec_key` | Secret key — never share; store securely |

```olrn
@import ( crypt = @std.crypt )

fn main() {
    alice :: crypt.crypt_box_keygen()
    bob   :: crypt.crypt_box_keygen()

    # alice.pub_key — Alice's public key (share with Bob)
    # bob.pub_key   — Bob's public key (share with Alice)
}
```

## Encrypt

```olrn
fn crypt_box_encrypt(our_sk: str, their_pk: str, plain: str) -> CryptError!str
```

Alice encrypts a message for Bob using her secret key and Bob's public key. Only Bob (with his secret key) can decrypt it.

```olrn
ct :: try crypt.crypt_box_encrypt(alice.sec_key, bob.pub_key, "hello Bob")
```

## Decrypt

```olrn
fn crypt_box_decrypt(our_sk: str, their_pk: str, ct: str) -> CryptError!str
```

Bob decrypts using his secret key and Alice's public key. The library verifies the authentication tag; `DecryptFailed` is returned if the message was tampered with or the keys are wrong.

```olrn
msg :: try crypt.crypt_box_decrypt(bob.sec_key, alice.pub_key, ct)
# msg == "hello Bob"
```

## Full example

```olrn
@import ( crypt = @std.crypt, io = @std.io )

fn main() -> !void {
    alice :: crypt.crypt_box_keygen()
    bob   :: crypt.crypt_box_keygen()

    # Alice sends to Bob
    plain :: "top secret"
    ct    :: try crypt.crypt_box_encrypt(alice.sec_key, bob.pub_key, plain)

    # Bob receives from Alice
    msg :: try crypt.crypt_box_decrypt(bob.sec_key, alice.pub_key, ct)
    io.println(msg)   # "top secret"

    # Tampering causes DecryptFailed
    bad_ct :: ct + "x"
    result :: crypt.crypt_box_decrypt(bob.sec_key, alice.pub_key, bad_ct)
    # result is CryptError.DecryptFailed
}
```

## Notes

- **Key pairs are ephemeral by default.** Generate them fresh each session, or serialize `pub_key`/`sec_key` (raw bytes) and store them yourself.
- **Box encryption provides mutual authentication** — decryption proves both that Alice sent the message (only she has `alice.sec_key`) and that it was meant for Bob (only he has `bob.sec_key`). It does not prove Alice's identity to a third party — use signing for that.
- To encode keys for storage or transmission, use `crypt_base64_encode` / `crypt_hex_encode`.
