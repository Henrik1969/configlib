# HOWTO: using configlib as an outside developer

This document is written for a developer who has just discovered `configlib` and wants to know what to do next.

It answers practical questions:

- How do I decide whether I should use `configlib`?
- How do I build it?
- How do I run the tests?
- How do I install it?
- How do I use it from C++?
- How do I use it from C?
- How do I start making bindings for another language?
- How do I package it with my own project?

For terminology, see [`TERMINOLOGY.md`](TERMINOLOGY.md). For a quick conceptual explanation, see [`EXPLAINED_SIMPLY.md`](EXPLAINED_SIMPLY.md).

---

## 1. Should I use configlib?

Use `configlib` when your application has configuration from several places and you care about rules, provenance, validation, runtime access, and diagnostics.

Good fit:

```text
internal defaults
+ config files
+ environment variables
+ command-line options
+ runtime overrides
+ validation
+ scoped subsystem views
+ export/redaction rules
+ diagnostics explaining what happened
```

Poor fit:

```text
a tiny tool that just reads two argv numbers and exits
```

For tiny tools, use ordinary argument parsing. `configlib` is for governed application configuration, not for replacing every `argc`/`argv` loop.

---

## 2. Requirements

The current project is built around:

```text
C++20
CMake
Ninja or Make
A normal C/C++ toolchain such as GCC or Clang
```

On Debian/Ubuntu/Pop!_OS-like systems, the basic tools are usually:

```sh
sudo apt install build-essential cmake ninja-build
```

For generated documentation:

```sh
sudo apt install doxygen graphviz texlive-latex-base texlive-latex-extra
```

The LaTeX packages are only needed if you want PDF-style generated manuals.

---

## 3. Get the source

Clone with SSH:

```sh
git clone git@github.com:Henrik1969/configlib.git
cd configlib
```

Or HTTPS:

```sh
git clone https://github.com/Henrik1969/configlib.git
cd configlib
```

Check the version:

```sh
grep CONFIGLIB_VERSION_STRING include/configlib/version.hpp
```

---

## 4. Build from source

Recommended development build:

```sh
cmake -S . -B build -G Ninja
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Using a controlled percentage of CPU threads:

```sh
pct=75
jobs=$(( $(nproc) * pct / 100 ))
(( jobs < 1 )) && jobs=1

cmake -S . -B build -G Ninja \
  && cmake --build build -j"$jobs" \
  && ctest --test-dir build --output-on-failure
```

Build with Make instead of Ninja:

```sh
cmake -S . -B build
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

---

## 5. Run examples

After building, run the examples from the repository root:

```sh
./build/configlib_basic_cpp
./build/configlib_basic_c
./build/configlib_loaders_cpp
./build/configlib_files_cpp
./build/configlib_store_cpp
./build/configlib_views_cpp
./build/configlib_bindings_cpp
./build/configlib_schema_cpp
```

Example with environment and CLI facts:

```sh
MYAPP_LOG_LEVEL=debug \
MYAPP_SERVER_PORT=9000 \
./build/configlib_loaders_cpp --log-level=trace --port 10001 --feature
```

---

## 6. Generate documentation

The project includes a `Doxyfile` with HTML and LaTeX output enabled.

Direct Doxygen invocation:

```sh
doxygen Doxyfile
firefox build/docs/html/index.html &
```

CMake docs target:

```sh
cmake -S . -B build -G Ninja -DCONFIGLIB_BUILD_DOCS=ON
cmake --build build --target configlib_docs
firefox build/docs/html/index.html &
```

Generated LaTeX output is placed under:

```text
build/docs/latex
```

To try producing the generated PDF:

```sh
cd build/docs/latex
make
firefox refman.pdf &
```

The generated docs are a baseline. You are expected to style and polish manuals for your own project needs.

---

## 7. Install locally

Use a local staging prefix first:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PWD/install"
cmake --build build -j$(nproc)
cmake --install build
```

For a user-local install:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local"

cmake --build build -j$(nproc)
cmake --install build
```

The install tree contains headers, the enabled library targets, CMake package files, and `pkg-config` metadata. See [`PACKAGING.md`](PACKAGING.md).

For a `$HOME/.local` shared-library install, make sure your runtime loader can find the library if your platform does not already search that prefix:

```sh
export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"
```

---

## 8. Use from a CMake C++ project

### Option A: installed package

After installing `configlib`, point CMake at the install prefix:

```sh
cmake -S myapp -B myapp-build \
  -DCMAKE_PREFIX_PATH=/path/to/configlib/install
```

Consumer `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22)
project(myapp LANGUAGES CXX)

find_package(configlib CONFIG REQUIRED)

add_executable(myapp src/main.cpp)
target_link_libraries(myapp PRIVATE configlib::configlib)
```

The installed package also exposes `configlib::shared` and `configlib::static` when those library forms were installed.

### Option B: vendored subdirectory

Project layout:

```text
myapp/
├── CMakeLists.txt
├── external/
│   └── configlib/
└── src/
    └── main.cpp
```

In your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22)
project(myapp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/configlib)

add_executable(myapp src/main.cpp)
target_link_libraries(myapp PRIVATE configlib::static)
```

Include it:

```cpp
#include <configlib/configlib.hpp>
```


---

## 9. Minimal C++ use

This tiny example uses internal default facts, policy, schema validation, a runtime store, and a scoped view.

```cpp
#include <configlib/configlib.hpp>
#include <iostream>

int main()
{
    configlib::FactSet facts;

    // Defaults are facts. They are the authoritative fallback source.
    configlib::InternalDefaultsProvider defaults("myapp defaults");
    defaults.add(configlib::KeyPath("myapp.logging.level"), configlib::Value("info"));
    defaults.add(configlib::KeyPath("myapp.logging.color"), configlib::Value(true));
    facts.extend(defaults.load().facts());

    // Policy is the rulebook for resolution behavior.
    configlib::PolicySet policy;
    policy.precedence().set(configlib::SourceKind::InternalDefault, 10);

    auto resolved = configlib::resolve(facts, policy);
    if (!resolved.ok()) {
        std::cerr << resolved.diagnostics().to_string();
        return 1;
    }

    // Schema validates shape. It does not inject defaults.
    configlib::ConfigSchema schema;
    schema.path(configlib::KeyPath("myapp.logging.level"))
        .string()
        .required()
        .allowed({"trace", "debug", "info", "warn", "error"})
        .documented_default(configlib::Value("info"));

    auto checked = schema.validate(resolved.config());
    if (!checked.ok()) {
        std::cerr << checked.diagnostics().to_string();
        return 1;
    }

    // Store is the governed runtime vault.
    auto store = configlib::ConfigStore::from_result(resolved);

    // View is a scoped keyhole into one subtree.
    auto logging = store.view(configlib::KeyPath("myapp.logging"));

    std::cout << "level = "
              << logging.get_string_or(configlib::KeyPath("level"), "info")
              << "\n";
}
```

---

## 10. Use from C

The C API is exposed through:

```c
#include <configlib/configlib.h>
```

The C ABI is intended as the long-term binding surface for non-C++ languages. It uses opaque handles so C++ implementation details do not leak across the ABI.

Basic shape:

```c
#include <configlib/configlib.h>
#include <stdio.h>

int main(void)
{
    printf("configlib version: %d.%d.%d\n",
           configlib_version_major(),
           configlib_version_minor(),
           configlib_version_patch());
    return 0;
}
```

Compile through `pkg-config` after installation:

```sh
export PKG_CONFIG_PATH=/path/to/configlib/install/lib/pkgconfig:$PKG_CONFIG_PATH
c++ main.c $(pkg-config --cflags --libs configlib) -o myapp
```

The C ABI is the intended binding surface for non-C++ languages. It covers the non-template core; C++ template helpers such as `StructBinding<T>` remain C++-only.

---

## 11. Use from another language

The recommended binding path is:

```text
your language
    |
    v
C FFI layer
    |
    v
configlib C ABI
    |
    v
configlib C++ implementation
```

Do not bind directly to C++ templates such as `StructBinding<T>` from a foreign language. Use the C ABI for cross-language work.

### Binding checklist

For a new language binding, expose these concepts first:

```text
version
facts
policy
resolve
resolved config getters
diagnostics
store
view
transaction
schema validation
export
```

Handle rules:

```text
successful create/open calls return valid handles
failed calls return an error/null handle according to the C ABI contract
caller must destroy handles it owns
strings returned by the C ABI must have documented lifetime/free rules
no C++ exceptions may cross the C ABI
```

### Python sketch with ctypes

```python
import ctypes

lib = ctypes.CDLL("./build/libconfiglib.so")

print(
    lib.configlib_version_major(),
    lib.configlib_version_minor(),
    lib.configlib_version_patch(),
    sep=".",
)
```

### Rust sketch

```rust
extern "C" {
    fn configlib_version_major() -> i32;
    fn configlib_version_minor() -> i32;
    fn configlib_version_patch() -> i32;
}

fn main() {
    unsafe {
        println!(
            "configlib version: {}.{}.{}",
            configlib_version_major(),
            configlib_version_minor(),
            configlib_version_patch()
        );
    }
}
```

### Zig sketch

```zig
const std = @import("std");
const c = @cImport({
    @cInclude("configlib/configlib.h");
});

pub fn main() void {
    std.debug.print(
        "configlib version: {d}.{d}.{d}\n",
        .{ c.configlib_version_major(), c.configlib_version_minor(), c.configlib_version_patch() },
    );
}
```

These are starting points. Serious bindings should wrap ownership, diagnostics, and errors in idiomatic language constructs.

---

## 12. Recommended project integration pattern

For a medium-sized application, create one configuration module in your app:

```text
src/config/
├── app_config.cpp
├── app_config.hpp
└── schema.cpp
```

That module should:

```text
1. create internal defaults
2. discover/load config files
3. load environment facts
4. load CLI facts
5. define resolution policy
6. resolve facts
7. validate schema
8. create ConfigStore
9. hand out scoped views or typed bindings to subsystems
```

Avoid letting every subsystem load config files or environment variables by itself. That recreates the global config swamp.

Good:

```cpp
auto logging = store.view(configlib::KeyPath("myapp.logging"));
Logger logger(logging);
```

Better when a subsystem wants a typed snapshot:

```cpp
LoggingConfig cfg = logging_binding.read(logging).value();
Logger logger(cfg);
```

Bad:

```cpp
Logger logger;
logger.read_env_itself();
logger.read_config_file_itself();
```

---

## 13. Common mistakes

### Mistake: using schema as defaults

Do not do this conceptually:

```text
schema says default = info, therefore configlib should inject info
```

Correct rule:

```text
Defaults are facts.
Schema may document expected defaults but does not inject them.
```

### Mistake: using fallback getters as real defaults

This is okay for defensive code:

```cpp
view.get_string_or(configlib::KeyPath("level"), "info");
```

But authoritative defaults should come from default facts:

```cpp
defaults.add(configlib::KeyPath("logging.level"), configlib::Value("info"));
```

### Mistake: treating `StructBinding<T>` as truth

Bindings are projections. The `ConfigStore` remains the governed runtime truth.

### Mistake: passing the whole store everywhere

Prefer scoped views:

```cpp
store.view(configlib::KeyPath("database"));
store.view(configlib::KeyPath("logging"));
store.view(configlib::KeyPath("ai.router"));
```

Subsystems should not get keys to the whole castle unless they truly need them.

### Mistake: expecting silent fallback

`get_*()` functions do not silently fall back. Use `get_*_or()` when you explicitly want fallback behavior.

---

## 14. Troubleshooting

### Build cannot find Ninja

Install Ninja or use the default generator:

```sh
sudo apt install ninja-build
cmake -S . -B build
```

### Doxygen target is missing

Configure with docs enabled:

```sh
cmake -S . -B build -DCONFIGLIB_BUILD_DOCS=ON
```

Also make sure Doxygen is installed:

```sh
doxygen --version
```

### Runtime cannot find shared library

Use one of:

```sh
export LD_LIBRARY_PATH=/path/to/configlib/build:$LD_LIBRARY_PATH
```

or install the library into a standard path.

### A setting is not what I expected

Use explanation/provenance support. The whole point of `configlib` is to answer:

```text
Where did this value come from?
Why did it win?
What did it override?
```

---

## 15. Stability expectations

`configlib` is still pre-1.0. The current focus is hardening the dependency-consumption surface and preparing for API freeze.

The intended stabilization path is:

```text
v0.8.x   API cleanup and documentation
v0.9.x   C ABI parity and binding surface stabilization
v0.10.x  install/package/build hardening
v0.11.x  API freeze candidate
v0.12.x  release-candidate cleanup if needed
v0.13.x  trial by fire: hostile tests, sanitizer/fuzzing/stress checks
v1.0.0   first stable dependency release
```

If you are integrating before v1.0, prefer vendoring or pinning a tag.

---

## 16. Where to read next

Start here:

```text
docs/EXPLAINED_SIMPLY.md
docs/TERMINOLOGY.md
docs/Programmers_Manual.md
docs/PACKAGING.md
docs/PUBLIC_API_MAP.md
```

Then continue with topic-specific docs:

```text
docs/LOADERS.md
docs/FILE_DISCOVERY.md
docs/STORE.md
docs/VIEWS.md
docs/BINDINGS.md
docs/SCHEMA.md
docs/API_STABILITY.md
docs/LIFETIMES.md
docs/DOCUMENTATION_GENERATION.md
```
