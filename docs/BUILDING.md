# Building configlib

## Requirements

- CMake 3.22 or newer
- C++20 compiler
- Ninja recommended

## Configure

```sh
cmake -S . -B build -G Ninja
```

## Build

```sh
cmake --build build -j"$(nproc)"
```

## Test

```sh
ctest --test-dir build --output-on-failure
```

## Install

Install into a local prefix while testing:

```sh
cmake --install build --prefix "$PWD/install"
```

The install step places headers, libraries, CMake package files, and a pkg-config file below the prefix. See `PACKAGING.md` for the full dependency-consumption story.

## Options

```text
CONFIGLIB_BUILD_SHARED   default ON
CONFIGLIB_BUILD_STATIC   default ON
CONFIGLIB_BUILD_EXAMPLES default ON
CONFIGLIB_BUILD_TESTS    default ON
CONFIGLIB_BUILD_DOCS     default OFF
```

Example:

```sh
cmake -S . -B build -G Ninja \
  -DCONFIGLIB_BUILD_SHARED=ON \
  -DCONFIGLIB_BUILD_STATIC=ON \
  -DCONFIGLIB_BUILD_EXAMPLES=ON \
  -DCONFIGLIB_BUILD_TESTS=ON
```

Dependency-only build:

```sh
cmake -S . -B build -G Ninja \
  -DCONFIGLIB_BUILD_EXAMPLES=OFF \
  -DCONFIGLIB_BUILD_TESTS=OFF
```

## Examples

```sh
./build/configlib_basic_cpp
./build/configlib_basic_c
./build/configlib_loaders_cpp --log-level=trace --port 10001 --feature
./build/configlib_files_cpp
./build/configlib_store_cpp
./build/configlib_views_cpp
./build/configlib_bindings_cpp
./build/configlib_schema_cpp
```

With environment intake:

```sh
MYAPP_LOG_LEVEL=debug MYAPP_SERVER_PORT=9000 \
  ./build/configlib_loaders_cpp --log-level=trace
```

The CLI value wins over environment by default because `PolicySet` gives CLI higher precedence than environment.

## Documentation target

```sh
cmake -S . -B build -G Ninja -DCONFIGLIB_BUILD_DOCS=ON
cmake --build build --target configlib_docs
```

## Hardening builds

Sanitizer example:

```sh
cmake -S . -B build-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan -j"$(nproc)"
ctest --test-dir build-asan --output-on-failure
```

Maintainer warning-clean example:

```sh
cmake -S . -B build-warnings -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build-warnings -j"$(nproc)"
```

`-Werror` is a maintainer gate, not a promise that every user compiler warning will be treated as a hard error by default.
