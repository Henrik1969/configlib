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

If install targets are available in your checked-out version, use:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

For a user-local install:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local"

cmake --build build -j$(nproc)
cmake --install build
```

Then make sure your compiler and loader can find the installed files. For example:

```sh
export CPLUS_INCLUDE_PATH="$HOME/.local/include:$CPLUS_INCLUDE_PATH"
export LIBRARY_PATH="$HOME/.local/lib:$LIBRARY_PATH"
export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"
```

If your current version does not yet provide polished install/export targets, vendor the source or add it as a CMake subdirectory for now. Full install/package hardening belongs to the stabilization track.

---

## 8. Use from a CMake C++ project

### Option A: vendored subdirectory

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
target_link_libraries(myapp PRIVATE configlib)
```

Include it:

```cpp
#include <configlib/configlib.hpp>
```

### Option B: installed library

Once install/package targets are stable, the intended shape is:

```cmake
find_package(configlib REQUIRED)

target_link_libraries(myapp PRIVATE configlib::configlib)
```

Until then, vendoring is the safest route.

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
    printf("configlib version: %s\n", configlib_version_string());
    return 0;
}
```

Compile shape, adjusted for your install path:

```sh
cc main.c -I/path/to/configlib/include -L/path/to/configlib/build -lconfiglib -o myapp
```

The C ABI currently trails the full C++ feature set. Full parity for the non-template core is planned for the v0.9 line.

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
from ctypes import c_char_p

lib = ctypes.CDLL("./build/libconfiglib.so")
lib.configlib_version_string.restype = c_char_p

print(lib.configlib_version_string().decode("utf-8"))
```

### Rust sketch

```rust
use std::ffi::CStr;
use std::os::raw::c_char;

extern "C" {
    fn configlib_version_string() -> *const c_char;
}

fn main() {
    unsafe {
        let ptr = configlib_version_string();
        let version = CStr::from_ptr(ptr).to_string_lossy();
        println!("configlib version: {}", version);
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
    const version = std.mem.span(c.configlib_version_string());
    std.debug.print("configlib version: {s}\n", .{version});
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

`configlib` is still pre-1.0. The v0.8 line is the API cleanup and documentation preparation line.

The intended stabilization path is:

```text
v0.8.x  API cleanup and documentation
v0.9.x  C ABI parity and binding surface stabilization
v0.10.x install/package/CI/hardening
v1.0.0  first stable dependency release
```

If you are integrating before v1.0, prefer vendoring or pinning a tag.

---

## 16. Where to read next

Start here:

```text
docs/EXPLAINED_SIMPLY.md
docs/TERMINOLOGY.md
docs/Programmers_Manual.md
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
