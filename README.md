# configlib v0.5

`configlib` is a policy/fact driven configuration resolution and runtime access library for C and C++ programs.

It is built around a simple design law:

> All inputs become facts. All behavior is governed by policy. The mechanism discovers, normalizes, resolves, validates, stores, and exposes configuration. No application meaning is hardcoded into the mechanism.

v0.5 extends v0.4 with scoped subtree views:

- `ConfigView` read-only scoped access into a `ConfigStore`
- `ConfigStore::view(prefix)` helper
- view-local getters such as `view.get_string("level")` for `logging.level`
- fallback helpers such as `get_string_or`, `get_int_or`, `get_bool_or`, and `get_double_or`
- subtree key listing
- scoped explain
- full-path scoped export
- local scoped export with the prefix stripped
- view example and tests
- updated documentation and changelog

The important rule still holds: `ConfigStore` is the governed runtime vault; `ConfigView` is a scoped keyhole into one subtree of that vault.

## Build

```sh
cmake -S . -B build -G Ninja
cmake --build build -j20
ctest --test-dir build --output-on-failure
```

## Run examples

```sh
./build/configlib_basic_cpp
./build/configlib_basic_c
MYAPP_LOG_LEVEL=debug MYAPP_SERVER_PORT=9000 ./build/configlib_loaders_cpp --log-level=trace --port 10001 --feature
./build/configlib_files_cpp
./build/configlib_store_cpp
./build/configlib_views_cpp
```

## Current scope

- C++20 library
- Static and shared build targets
- Opaque C ABI
- Scalar value model: null, bool, int64, double, string
- Dotted key paths
- Facts with source/provenance and precedence
- Policies for required/defaulted/optional keys
- Type validation
- String allow-lists
- Integer min/max validation
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
- Simple export modes
- Scoped read-only `ConfigView`
- Scoped export, local export, explain, and key listing


## When not to use configlib

`configlib` is deliberately not a replacement for every tiny `argc`/`argv` loop. For small tools where command-line arguments are the whole configuration surface, plain argument parsing is still the right tool.

Use `configlib` when configuration becomes a governed runtime concern: defaults, files, environment, CLI, runtime overrides, validation, precedence, provenance, scoped subsystem views, export policy, and diagnostics. See [`docs/WHEN_TO_USE.md`](docs/WHEN_TO_USE.md).

## Documentation

See `docs/`. Start with:

- `docs/ARCHITECTURE.md`
- `docs/API.md`
- `docs/LOADERS.md`
- `docs/FILE_DISCOVERY.md`
- `docs/STORE.md`
- `docs/VIEWS.md`
- `docs/PLUGIN_MODEL.md`
- `docs/CHANGELOG.md`
