# configlib v0.10.0

`configlib` is a policy/fact driven configuration resolution and runtime access library for C and C++ programs.

It is built around a simple design law:

> All inputs become facts. All behavior is governed by policy. The mechanism discovers, normalizes, resolves, validates, stores, and exposes configuration. No application meaning is hardcoded into the mechanism.

v0.10.0 is a packaging/install hardening release on top of v0.9.x. It adds CMake install rules, an exported CMake package, a pkg-config file, and packaging documentation so the library can be consumed as a normal dependency.

Important current capabilities include:

- `ConfigView` read-only scoped access into a `ConfigStore`
- `ConfigStore::view(prefix)` helper
- view-local getters such as `view.get_string("level")` for `logging.level`
- explicit fallback helpers such as `get_string_or`, `get_integer_or`, `get_boolean_or`, and `get_floating_or`
- subtree key listing
- scoped explain
- full-path scoped export
- local scoped export with the prefix stripped
- view example and tests
- installed headers, libraries, CMake package files, and pkg-config metadata
- updated documentation and changelog

The important rule still holds: `ConfigStore` is the governed runtime vault; `ConfigView` is a scoped keyhole into one subtree of that vault.

## Build

```sh
cmake -S . -B build -G Ninja
cmake --build build -j20
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$PWD/install"
```


## Generate documentation

The project ships a Doxygen configuration with HTML and LaTeX output enabled:

```sh
doxygen Doxyfile
```

Generated docs are written below `build/docs/`. LaTeX generation is enabled deliberately so later releases can produce polished PDF manuals. A CMake helper target is also available when Doxygen is installed:

```sh
cmake -S . -B build -G Ninja -DCONFIGLIB_BUILD_DOCS=ON
cmake --build build --target configlib_docs
```

## Run examples

```sh
./build/configlib_basic_cpp
./build/configlib_basic_c
MYAPP_LOG_LEVEL=debug MYAPP_SERVER_PORT=9000 ./build/configlib_loaders_cpp --log-level=trace --port 10001 --feature
./build/configlib_files_cpp
./build/configlib_store_cpp
./build/configlib_views_cpp
./build/configlib_bindings_cpp
./build/configlib_schema_cpp
```

## Current scope

- C++20 library
- Static and shared build targets
- CMake install/export support
- pkg-config metadata
- Expanded opaque C ABI for binding languages
- Scalar value model: null, bool, int64, double, string
- Dotted key paths
- Facts with source/provenance and precedence
- Policies for source precedence and conflict behavior
- Schema validation for required/optional keys
- Schema type validation
- Schema string allow-lists and numeric ranges
- Conflict strategies
- Resolved config view
- Explanation/provenance report
- Diagnostics
- Internal defaults provider
- Environment loader with explicit mappings and prefix-to-dotted-key mapping
- CLI loader with `--opt value`, `--opt=value`, and boolean flags
- File discovery policy
- Absence policy for missing config files
- Simple key=value config file loader
- Runtime `ConfigStore`
- Transactional runtime mutation
- Runtime access policy
- Explicit export modes including redacted variants
- Scoped read-only `ConfigView`
- Scoped export, local export, explain, and key listing
- Explicit typed `StructBinding<T>` projections
- In-code `ConfigSchema` validation rules
- Required/optional schema keys
- Schema type checks, string allow-lists, numeric ranges, and documented defaults


## When not to use configlib

`configlib` is deliberately not a replacement for every tiny `argc`/`argv` loop. For small tools where command-line arguments are the whole configuration surface, plain argument parsing is still the right tool.

Use `configlib` when configuration becomes a governed runtime concern: defaults, files, environment, CLI, runtime overrides, validation, precedence, provenance, scoped subsystem views, export policy, and diagnostics. See [`docs/WHEN_TO_USE.md`](docs/WHEN_TO_USE.md).

## License

`configlib` is released under the MIT License. See [`LICENSE`](LICENSE).

There is also an informal author/development note in [`docs/AUTHOR_NOTE.md`](docs/AUTHOR_NOTE.md). It is not an extra license condition; it explains the spirit of the release and acknowledges AI-assisted development.

Source files carry the short SPDX marker:

```text
SPDX-License-Identifier: MIT
```

## Documentation

See `docs/`. Start with:

- `docs/HOWTO.md`
- `docs/PACKAGING.md`
- `docs/FIRE_TESTING.md`
- `docs/MACHINE_CONFIGURATION_ROADMAP.md`
- `docs/Programmers_Manual.md`
- `docs/C_ABI.md`
- `docs/TERMINOLOGY.md`
- `docs/EXPLAINED_SIMPLY.md`

- `docs/ARCHITECTURE.md`
- `docs/API.md`
- `docs/PUBLIC_API_MAP.md`
- `docs/DOCUMENTATION_GENERATION.md`
- `docs/LOADERS.md`
- `docs/FILE_DISCOVERY.md`
- `docs/STORE.md`
- `docs/VIEWS.md`
- `docs/BINDINGS.md`
- `docs/SCHEMA.md`
- `docs/PLUGIN_MODEL.md`
- `docs/CHANGELOG.md`
