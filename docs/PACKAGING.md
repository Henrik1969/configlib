# Packaging, installation, and consumption

`configlib` v0.10.0 adds the first deliberately boring packaging layer.
The goal is simple: a developer should be able to build the project from a
source archive, install it into a prefix, and consume it through normal CMake or
`pkg-config` mechanisms.

This document is about consuming `configlib` as a dependency. It does not change
library behavior.

## Build and test from source

```sh
cmake -S . -B build -G Ninja
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

By default the project builds both shared and static libraries, examples, and
tests.

## Build options

```text
CONFIGLIB_BUILD_SHARED   default ON
CONFIGLIB_BUILD_STATIC   default ON
CONFIGLIB_BUILD_EXAMPLES default ON
CONFIGLIB_BUILD_TESTS    default ON
CONFIGLIB_BUILD_DOCS     default OFF
```

For a dependency-only build:

```sh
cmake -S . -B build -G Ninja \
  -DCONFIGLIB_BUILD_EXAMPLES=OFF \
  -DCONFIGLIB_BUILD_TESTS=OFF
cmake --build build -j"$(nproc)"
```

## Install into a local prefix

Do not test installation by writing into `/usr` first. Use a local staging
prefix:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build -j"$(nproc)"
cmake --install build
```

The install tree contains:

```text
install/include/configlib/...
install/lib/libconfiglib.so
install/lib/libconfiglib.a
install/lib/cmake/configlib/configlibConfig.cmake
install/lib/cmake/configlib/configlibTargets.cmake
install/lib/pkgconfig/configlib.pc
```

The exact library directory may be `lib`, `lib64`, or another platform-specific
value controlled by CMake's `GNUInstallDirs`.

## Use from another CMake project

After installing into a prefix, point CMake at that prefix:

```sh
cmake -S consumer -B consumer-build \
  -DCMAKE_PREFIX_PATH=/path/to/configlib/install
```

Example `CMakeLists.txt` for a consumer:

```cmake
cmake_minimum_required(VERSION 3.22)
project(configlib_consumer LANGUAGES CXX)

find_package(configlib CONFIG REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE configlib::configlib)
```

The installed package exports these imported targets:

```text
configlib::configlib   convenience target; shared if available, otherwise static
configlib::shared      shared library target when installed
configlib::static      static library target when installed
```

Use `configlib::configlib` for ordinary consumers. Use `configlib::shared` or
`configlib::static` only when the application deliberately wants one form.

## Use from C with pkg-config

The installed `configlib.pc` file allows C and non-CMake consumers to discover
compiler and linker flags:

```sh
export PKG_CONFIG_PATH=/path/to/configlib/install/lib/pkgconfig:$PKG_CONFIG_PATH
cc main.c $(pkg-config --cflags --libs configlib)
```

Because the implementation is C++, C programs normally need the platform C++
runtime at link time. The pkg-config file exposes the library itself; if a
platform linker does not automatically pull the C++ runtime through the shared
library, link the final program with the C++ compiler driver instead:

```sh
c++ main.c $(pkg-config --cflags --libs configlib)
```

## Build shared-only or static-only

Shared-only:

```sh
cmake -S . -B build-shared -G Ninja \
  -DCONFIGLIB_BUILD_SHARED=ON \
  -DCONFIGLIB_BUILD_STATIC=OFF
```

Static-only:

```sh
cmake -S . -B build-static -G Ninja \
  -DCONFIGLIB_BUILD_SHARED=OFF \
  -DCONFIGLIB_BUILD_STATIC=ON
```

At least one library form should be enabled for installation to be useful.

## Fresh-tarball validation

Before publishing a release archive, validate the archive itself rather than the
working directory:

```sh
tar -xzf configlib_vX_Y_Z.tar.gz
cd configlib_vX_Y_Z
cmake -S . -B build -G Ninja
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/install"
```

Then validate consumption from the installed package using a tiny external CMake
project.

## Hardening build examples

AddressSanitizer and UndefinedBehaviorSanitizer are useful before v1:

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

Warning-clean builds are also part of the v1 path:

```sh
cmake -S . -B build-warnings -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build-warnings -j"$(nproc)"
```

Treat `-Werror` as a maintainer check, not necessarily as the default for users.

## Packaging boundary

v0.10.0 does not introduce distro packaging files such as `.deb`, `.rpm`, or
Arch `PKGBUILD`. It prepares the upstream install surface that such packages can
use later.

The intended order is:

```text
source tree -> CMake build -> install prefix -> package manager recipe later
```

Do not make the package manager recipe the first trusted installation story. The
plain CMake install must work first.
