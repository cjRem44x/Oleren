# Signing

Digital signatures let you prove that a message was produced by a specific party and has not been altered since. Unlike box encryption, signing does not encrypt — the message stays readable. It just adds a verifiable proof of origin.

Crypt uses Ed25519 (Edwards-curve Digital Signature Algorithm), a fast and widely trusted signature scheme.

## Key pair generation

```olrn
fn crypt_sign_keygen() -> CryptSignKeyPair
```

Returns a `CryptSignKeyPair` with two fields:

| Field | Description |
|---|---|
| `sign_key` | Private signing key — used to create signatures |
| `verify_key` | Public verification key — distribute to anyone who needs to verify |

```olrn
keys :: crypt.crypt_sign_keygen()
# keys.sign_key   — keep secret
# keys.verify_key — publish or share
```

## Sign

```olrn
fn crypt_sign(sk: str, msg: str) -> str
```

Signs `msg` with the signing key. Returns a 64-byte detached signature (not the message itself). The message and signature are kept separate — pass both to the verifier.

```olrn
sig :: crypt.crypt_sign(keys.sign_key, "release v1.0")
```

## Verify

```olrn
fn crypt_verify(vk: str, msg: str, sig: str) -> bool
```

Returns `true` if the signature is valid for `msg` under the given verify key. Returns `false` if the message was altered, the signature is invalid, or the key is wrong.

```olrn
ok :: crypt.crypt_verify(keys.verify_key, "release v1.0", sig)
if ok { io.println("authentic") }
```

## Full example

```olrn
@import ( crypt = @std.crypt, io = @std.io )

fn main() {
    keys :: crypt.crypt_sign_keygen()
    msg  :: "firmware v2.3.1"

    sig :: crypt.crypt_sign(keys.sign_key, msg)

    # verify — succeeds
    ok :: crypt.crypt_verify(keys.verify_key, msg, sig)
    io.println(ok)   # true

    # tampered message — fails
    ok2 :: crypt.crypt_verify(keys.verify_key, "firmware v2.3.2", sig)
    io.println(ok2)  # false
}
```

## Common use cases

| Use case | Pattern |
|---|---|
| Firmware / release verification | Sign the binary hash, distribute `verify_key` with the product |
| API request signing | Sign `method + path + body + timestamp`, verify server-side |
| Config integrity | Sign config at write time, verify at load time |
| Token issuing | Sign a payload (user id, expiry); verify on each request |

## Encoding signatures for transmission

Signatures are raw binary. Encode them before embedding in JSON, URLs, or headers:

```olrn
sig_b64 :: crypt.crypt_base64_encode(sig)
# transmit sig_b64 as a string

sig_raw :: try crypt.crypt_base64_decode(sig_b64)
ok :: crypt.crypt_verify(vk, msg, sig_raw)
```
