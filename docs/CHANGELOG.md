# Changelog

## v0.8.0

API cleanup and stabilization-preparation release on top of v0.7.0.

Changed:

- Clarified schema/policy/access boundaries.
- Removed runtime/export/secret metadata from schema rules; those belong to `AccessPolicy`.
- Renamed schema default metadata to `documented_default()`. Schema documents defaults; it does not inject them.
- Made `Value::as_string()`, `as_integer()`, `as_boolean()`, and `as_floating()` return `std::optional`.
- Added explicit `Value::as_*_or()` fallback helpers.
- Normalized getter semantics: `get_*()` may return optional values; `get_*_or()` uses explicit caller fallback.
- Made `ConfigView` and `ConfigTransaction` non-default-constructible to avoid invalid handles.
- Changed `ConfigStore::diagnostics()` to return a const reference.
- Added explicit redacted export names such as `ExportMode::EffectiveRedacted`; `Redacted` remains as a compatibility alias.
- Moved root `example.conf` to `examples/example.conf`.
- Added API consistency tests.
- Updated project version metadata to `0.8.0`.

Added:

- `docs/API_STABILITY.md`.
- `docs/API_REVIEW.md`.
- `docs/LIFETIMES.md`.
- `docs/CODE_DOCUMENTATION.md`.
- `tests/test_api_consistency.cpp`.

Design rule: no silent fallback, no invalid handles, one source of truth for defaults, schema validates shape, policy governs behavior, bindings project into structs.

## v0.6.0

Feature patch on top of v0.5.5.

Added:

- `include/configlib/binding.hpp`.
- `StructBinding<T>` and `BindingResult<T>`.
- Explicit string, bool, int, int64_t, and double field binders.
- Required-field and type-mismatch diagnostics.
- `examples/bindings.cpp`.
- `tests/test_bindings.cpp`.
- `docs/BINDINGS.md`.

Changed:

- README and programmer manual now describe typed struct binding usage.
- Project version metadata to `0.6.0`.

Design rule: bindings are convenience projections; `ConfigStore` remains the source of governed truth.

## v0.5.5

Documentation patch on top of v0.5.4.

Added:

- `docs/Programmers_Manual.md` with practical usage guidance.
- C++ application setup example.
- C ABI example.
- Python `ctypes` binding sketch.
- Rust FFI sketch.
- Zig C-import sketch.
- LuaJIT FFI sketch.
- Shell integration pattern and project layout notes.
- Common mistakes section covering fallback defaults, precedence, scoped views, and secrets.

Changed:

- README documentation index now points to the programmer manual.
- Project version metadata to `0.5.5`.

## v0.5.4

Documentation-only patch.

Added:

- `docs/AUTHOR_NOTE.md` with an informal author note, friendly request for mentions/comments, and AI-assisted development acknowledgement.

Updated:

- README and documentation links to point at the author note.
- Project version metadata to `0.5.4`.

The MIT License remains the only legal license condition.

## v0.5.3

Documentation/legal patch.

- Updated the MIT license copyright holder to `Henrik Sørensen`.
- Updated project version metadata to `0.5.3`.

## v0.5.2

Documentation/legal patch.

Added:

- root `LICENSE` file using the MIT License
- README license section
- source-file SPDX markers: `SPDX-License-Identifier: MIT`
- `docs/LICENSE.md` with licensing notes

Updated project version metadata to `0.5.2`.

## v0.5.1

Documentation-only patch.

Added:

```text
docs/WHEN_TO_USE.md
```

Updated README and architecture notes with a clear tool-choice disclaimer: `configlib` is for governed application configuration, not for replacing simple `argc`/`argv` parsing in tiny tools.


## v0.5.0

Scoped configuration view release.

Added:

- `ConfigView`
- `ConfigStore::view(KeyPath prefix)`
- read-only subtree access over a `ConfigStore`
- view-local getters
- fallback helpers: `get_string_or`, `get_int_or`, `get_bool_or`, `get_double_or`
- subtree key listing
- scoped explain
- scoped full-path export
- scoped local export with prefix stripped
- `docs/VIEWS.md`
- `docs/PLUGIN_MODEL.md` future note
- `examples/views.cpp`
- `tests/test_views.cpp`

Preserved design line:

```text
ConfigStore is the governed runtime vault.
ConfigView is a scoped keyhole into one subtree of that vault.
```

Views are deliberately read-only in v0.5. Runtime mutation remains transaction-based through `ConfigStore`.

## v0.4.0

Runtime configuration store release.

Added:

- `ConfigStore`
- `ConfigTransaction`
- `AccessPolicy`
- `AccessRule`
- `ExportMode`
- runtime `set`, `erase`, `reset_to_base`, and `reset_to_default`
- transaction validation before commit
- failed commits leave the store unchanged
- runtime mutability policy
- exportability policy
- secret-key policy
- reset permission policy
- simple line-based export
- `docs/STORE.md`
- `examples/store.cpp`
- `tests/test_store.cpp`

Preserved design line:

```text
defaults/env/CLI/files -> facts -> policies -> resolver -> ConfigStore -> governed runtime use
```

The store is not a global variable bucket. It is a controlled runtime view with governed mutation and export.

## v0.3.0

File discovery release.

Added:

- `FileDiscoveryPolicy`
- `FileSearchRule`
- `DiscoveredConfigFile`
- `DiscoveryReport`
- `AbsenceAction`
- `ConfigFileFormat`
- `discover_config_files()`
- `load_config_file()`
- `load_config_files()`
- simple `key=value` file loader
- `[section]` support
- file provenance with line numbers
- `docs/FILE_DISCOVERY.md`
- `examples/files.cpp`
- `tests/test_files.cpp`

## v0.2.0

Loader producer release.

Added:

- `InternalDefaultsProvider`
- `EnvironmentLoaderPolicy`
- `CliLoaderPolicy`
- `LoadReport`
- `load_internal_defaults()`
- `load_environment()`
- `load_cli()`
- `parse_value()`
- loader documentation and tests

## v0.1.0

Initial fact/policy resolver release.

Added:

- scalar `Value`
- `KeyPath`
- `Source`
- `Fact` and `FactSet`
- `PolicySet`
- `ResolvedConfig`
- `ResolveResult`
- diagnostics
- C++ API
- opaque C ABI
- examples and tests
