# Changelog

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
