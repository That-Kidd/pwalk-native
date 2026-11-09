#!/bin/sh
# Cross-compiles build-win/pokewalker.exe on Linux: one exe with SDL2 linked
# in, for Windows 10 or later. The first run downloads zig (used as the
# Windows C compiler) and SDL2's MinGW development package into
# ~/.cache/pokewalker-build, checked by SHA-256. Set ZIG or SDL2 to use
# your own copies instead.
set -e
cd "$(dirname "$0")/.."

ZIG_VERSION=0.13.0
ZIG_SHA256=d45312e61ebcc48032b77bc4cf7fd6915c11fa16e4aad116b66c9468211230ea
SDL2_VERSION=2.30.9
SDL2_SHA256=b188d165dc4a0372042bff3d0c18e9bb90772581e92a7b8b47b521b47714f75c

CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/pokewalker-build"

fetch() { # url file sha256
    if [ ! -f "$CACHE/$2" ]; then
        echo "downloading $1"
        mkdir -p "$CACHE"
        curl -fL --retry 3 -o "$CACHE/$2.part" "$1"
        mv "$CACHE/$2.part" "$CACHE/$2"
    fi
    if ! echo "$3  $CACHE/$2" | sha256sum -c --quiet -; then
        echo "$CACHE/$2 does not match its SHA-256; delete it and retry" >&2
        exit 1
    fi
}

if [ -z "$ZIG" ]; then
    ZIG="$CACHE/zig-linux-x86_64-$ZIG_VERSION/zig"
    if [ ! -x "$ZIG" ]; then
        fetch "https://ziglang.org/download/$ZIG_VERSION/zig-linux-x86_64-$ZIG_VERSION.tar.xz" \
            zig.tar.xz "$ZIG_SHA256"
        tar -xJf "$CACHE/zig.tar.xz" -C "$CACHE"
    fi
fi

if [ -z "$SDL2" ]; then
    SDL2="$CACHE/SDL2-$SDL2_VERSION/x86_64-w64-mingw32"
    if [ ! -d "$SDL2" ]; then
        fetch "https://github.com/libsdl-org/SDL/releases/download/release-$SDL2_VERSION/SDL2-devel-$SDL2_VERSION-mingw.tar.gz" \
            sdl2.tar.gz "$SDL2_SHA256"
        tar -xzf "$CACHE/sdl2.tar.gz" -C "$CACHE"
    fi
fi

mkdir -p build-win
"$ZIG" cc -target x86_64-windows-gnu -std=c99 -O2 -Wall \
    -Iinclude -Isrc -Ifrontend/sdl -I"$SDL2/include/SDL2" -Dmain=SDL_main \
    src/api/*.c src/hw/*.c src/firmware/*.c frontend/sdl/*.c \
    -L"$SDL2/lib" -lmingw32 -lSDL2main -Wl,-Bstatic -lSDL2 -Wl,-Bdynamic \
    -lws2_32 -lcomdlg32 -lsetupapi -lwinmm -limm32 -lversion -lole32 -loleaut32 \
    -lgdi32 -luser32 -lshell32 -ladvapi32 -lcfgmgr32 -Wl,--subsystem,windows \
    -o build-win/pokewalker.exe
echo "built build-win/pokewalker.exe"
