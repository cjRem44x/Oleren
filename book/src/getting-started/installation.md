# Installation

## Prerequisites

- `gcc` / `g++` (C++17, tested with GCC 12+)
- `make`
- `pkg-config` (for optional library deps)
- **Optional:** SDL2, SDL2_mixer, SDL2_image, SDL2_ttf (for Gdev gamedev)
- **Optional:** libsodium (for Crypt crypto)

## Build from source

```sh
git clone https://github.com/cjRem44x/Oleren.git
cd Oleren
make
```

This produces the `olrn` binary in the repo root. Add it to your PATH:

```sh
export PATH="$PATH:/path/to/Oleren"
```

## Verify

```sh
olrn version
# olrn 0.1.6
```
