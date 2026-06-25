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
cmake --build build -j20
```

## Test

```sh
ctest --test-dir build --output-on-failure
```

## Options

```text
CONFIGLIB_BUILD_SHARED   default ON
CONFIGLIB_BUILD_STATIC   default ON
CONFIGLIB_BUILD_EXAMPLES default ON
CONFIGLIB_BUILD_TESTS    default ON
```

Example:

```sh
cmake -S . -B build -G Ninja \
  -DCONFIGLIB_BUILD_SHARED=ON \
  -DCONFIGLIB_BUILD_STATIC=ON \
  -DCONFIGLIB_BUILD_EXAMPLES=ON \
  -DCONFIGLIB_BUILD_TESTS=ON
```

## v0.2 examples

```sh
./build/configlib_loaders_cpp --log-level=trace --port 10001 --feature
```

With environment intake:

```sh
MYAPP_LOG_LEVEL=debug MYAPP_SERVER_PORT=9000 \
  ./build/configlib_loaders_cpp --log-level=trace
```

The CLI value wins over environment by default because `PolicySet` gives CLI higher precedence than environment.
