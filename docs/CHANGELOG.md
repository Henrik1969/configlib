# Changelog

## v1.0.0

First stable release of configlib.

This release promotes the governed configuration core to v1.0.0 after fire testing and external-consumer validation.

Validated areas include:

- C++ API version helpers.
- C ABI version helpers.
- External CMake C++ consumer.
- pkg-config C consumer.
- Python ctypes smoke test.
- LuaJIT FFI smoke test.
- Rust FFI smoke test.
- Baseline build/test.
- ASAN/UBSAN.
- Valgrind.
- ABI dump / abidw.
- Coverage generation.
- Install tree validation.

Known non-blocking issues:

- Local clang++ toolchain on the test host has a C++ standard-library setup issue.
- Doxygen HTML output path is source-relative in the current docs setup.


## v0.13.1

Fire-test follow-up release.

Added:

- `configlib::version_major()`, `configlib::version_minor()`, `configlib::version_patch()`, and `configlib::version_string()` C++ helpers.
- `CONFIGLIB_C_VERSION_STRING` and `configlib_version_string()` in the C ABI.
- Regression coverage for C++ and C ABI version-string helpers.

Fixed / clarified:

- External consumer and binding smoke tests now have a stable, simple symbol to call for version validation.
- Project version to `0.13.1`.
- C ABI version macros to `0.13.1`.

Design note: this is an additive fire-test remedy. The external fire-test harness remains outside the project for now and will later become a separate generalized tool.

## v0.13.0

Trial-by-fire release.

Added:

- `tests/test_fire.cpp`, a hostile/edge-case test suite for the v0.13 fire-testing phase.
- Regression coverage for malformed direct facts, malformed file/env/CLI keys, same-priority ambiguity, identical duplicate facts, transaction failure state preservation, scoped-view isolation, export/redaction non-leakage, and simple stress behavior.

Changed:

- Project version to `0.13.0`.
- C ABI version macros to `0.13.0`.
- Resolver now rejects invalid dotted keys with `CONFIG_INVALID_KEY` instead of allowing malformed facts into the resolved configuration.
- Built-in loaders now reject invalid dotted keys at the boundary with explicit diagnostics:
  - `CONFIG_FILE_BAD_KEY`
  - `CONFIG_ENV_BAD_KEY`
  - `CONFIG_CLI_BAD_KEY`
  - `CONFIG_DEFAULT_BAD_KEY`
- Test count increased from 9 to 10.

Design note: v0.13.0 is the first real fire-test pass. It keeps the post-v0.12 additive-only rule, except for correctness/safety fixes proven by hostile testing before v1.0.

## v0.12.0

Release-candidate preparation release.

Added:

- `docs/RELEASE_CANDIDATE.md`, documenting the v1 release-candidate checklist and additive-only rule after v0.12.
- Explicit exception rule: post-v0.12 breaking changes require a correctness, safety, security, or serious usability reason proven before v1.
- v1 gate question focused on failure behavior, hostile inputs, broken files, clumsy bindings, and trustworthiness.

Changed:

- Project version to `0.12.0`.
- C ABI version macros to `0.12.0`.
- README, roadmap, public API map, and API stability documentation now point to the release-candidate track.

Design note: v0.12.0 is behavior-neutral. It marks the public-surface transition point: later work should be additive unless fire testing proves a pre-v1 correction is necessary.

## v0.11.0

Pre-freeze API cleanup and audit-remediation release.

Added:
- `docs/API_FREEZE_CANDIDATE.md` documenting the additive-only rule intended after the freeze candidate.
- Regression coverage for same-priority conflicting facts and non-exportable redacted export behavior.

Changed:
- Project version to `0.11.0`.
- C ABI version macros to `0.11.0`.
- Same-priority conflicting facts under `HighestPrecedenceWins` now emit `CONFIG_AMBIGUOUS_PRECEDENCE` instead of silently relying on insertion order.
- Redacted export modes no longer export keys marked `exportable=false`; non-exportable means not exported.
- Documentation now states that defaults are facts, not policy-owned synthesized values.

Removed before v1 freeze:
- `PolicySet::defaulted(...)`, `MissingPolicy::UseDefault`, and policy-owned default storage.
- C policy-default helpers `configlib_default_*`; use `configlib_internal_default_*` to add real default facts.
- C++ shorthand implicit-fallback getter aliases such as `get_int`, `get_bool`, `get_double`, and view `get_*_or` aliases. Use canonical optional getters or explicit `_or` helpers instead.

## v0.10.0

Packaging/install hardening release.

Added:

- CMake install rules for headers and enabled library targets.
- Exported CMake package files under `lib/cmake/configlib`.
- Installed imported targets: `configlib::configlib`, `configlib::shared`, and `configlib::static`.
- `pkgconfig/configlib.pc.in` and installed `configlib.pc`.
- `cmake/configlibConfig.cmake.in`.
- `docs/PACKAGING.md`.

Updated:

- README build/install section.
- `docs/BUILDING.md` with install and sanitizer examples.
- `docs/HOWTO.md` with dependency-consumption notes.
- `docs/PUBLIC_API_MAP.md` with packaging files.
- `docs/ROADMAP.md` to mark v0.10.0 complete.
- Project version to `0.10.0`.

Design note: v0.10.0 changes no configuration behavior. It strengthens the dependency-consumption and install story before the API-freeze track.

## v0.9.1

Fire-testing roadmap release on top of v0.9.0.

Added:

- `docs/FIRE_TESTING.md`, a pre-v1 hardening roadmap for the v0.13.x trial-by-fire phase.
- Explicit acceptance rule: every rejection must have a reason.
- Test categories for conflict resolution, false claims, schema rejection, store/view isolation, transactions, export/redaction, C ABI abuse, malformed loader input, stress cases, and security boundaries.
- Sanitizer, fuzzing, and regression-test guidance.
- v1 gate question: whether configlib can be trusted when users are wrong, files are broken, bindings are clumsy, and attackers are annoying.

Changed:

- Bumped project version to `0.9.1`.
- README, roadmap, terminology, and public API map now link the fire-testing track.

Design note: v0.9.1 changes no library behavior. It records the hostile-testing and failure-semantics track that must be completed before v1.0.

## v0.9.0

C ABI parity and binding-surface release on top of v0.8.5.

Added/expanded:

- `docs/C_ABI.md`, documenting opaque C handles, ownership, borrowed strings, and binding-language rules.
- C ABI version/status/name helpers.
- C ABI double facts/defaults/getters.
- C ABI policy helpers for allowed strings and integer ranges.
- C ABI file loading helper for simple key=value config files.
- C ABI schema handle with required/optional rules, allowed strings, numeric ranges, documented defaults, descriptions, and validation results.
- C ABI access policy handle for runtime mutability, exportability, secret/redaction, and reset behavior.
- C ABI store handle created from a resolve result plus context policies and optional access policy.
- C ABI store getters, explain, diagnostics, export, runtime-change checks, scoped view creation, and transaction creation.
- C ABI transaction handle for staged set/erase/reset/validate/commit/rollback operations.
- C ABI view handle for scoped read-only getters, explain, full export, and local export.
- Broader C ABI smoke test covering schema, store, access policy, transactions, redaction, and scoped views.

Changed:

- Bumped project version to `0.9.0`.
- `configlib.h` C ABI version is now `0.9.0`.
- README and public API map now describe v0.9 as the binding-surface stabilization island.

C++ behavior is unchanged. C++ template projections such as `StructBinding<T>` remain outside the C ABI by design.


## v0.8.5

Roadmap/documentation release on top of v0.8.4.

Added:

- `docs/MACHINE_CONFIGURATION_ROADMAP.md`, a long-term roadmap for post-v1 configuration tooling and machine configuration governance.
- Configuration-law package concept for describing native configuration surfaces in machine-readable form.
- Post-v1 tool direction for `configlint`, `configforge`, `configdoctor`, `configapply`, `configctl`, and related tools.
- Whole-machine lifecycle scope from bootloader through kernel command line, initramfs, init/service layer, user session, shutdown, rollback, and recovery.
- Explicit boundary that this is not a v1 core requirement; it is a railroad marker for work that can grow around a stable `configlib` v1.

Updated:

- README documentation index and release description.
- Roadmap with the machine-configuration direction and v0.9 C ABI focus.
- Public API map with the long-term tooling/law-package roadmap link.
- Terminology guide with configuration law and configuration swamp definitions.
- Project version metadata to `0.8.5`.

Design note: v0.8.5 changes no library behavior. It records the long rail beyond v1 so v0.9 can focus on C ABI parity and binding-surface stabilization.

## v0.8.4

HOWTO/documentation release on top of v0.8.3.

Added:

- `docs/HOWTO.md`, a practical guide for third-party developers discovering configlib.
- Build, test, documentation-generation, local install, vendoring, and CMake integration instructions.
- C++, C ABI, Python ctypes, Rust FFI, and Zig `@cImport` starting points.
- Recommended application integration pattern and common mistakes section.

Updated:

- README documentation index.
- Public API map cross-links.
- Project version metadata to `0.8.4`.

Design note: v0.8.4 changes no library behavior. It improves onboarding for outside developers before the stable-release track.

## v0.8.3

Plain-language documentation release on top of v0.8.2.

Added:

- `docs/EXPLAINED_SIMPLY.md`, a newcomer-friendly explanation of configlib.
- A courtroom metaphor mapping facts, policy, schema, resolver, resolved config, diagnostics, store, view, and bindings to ordinary language.
- A commented C++ example that uses the metaphor directly inside code comments.

Updated:

- README documentation index.
- Public API map cross-links.
- Terminology guide cross-links.
- Project version metadata to `0.8.3`.

Design note: v0.8.3 changes no library behavior. It improves approachability before the stable-release track.

## v0.8.2

Terminology/documentation clarity release on top of v0.8.1.

Added:

- `docs/TERMINOLOGY.md`, a plain-language glossary for third-party readers.
- Definitions for governance, fact, source, provenance, policy, schema, store, view, binding, diagnostics, runtime mutation, redaction, and related terms.

Changed:

- README now points readers to the terminology guide.
- Project version metadata to `0.8.2`.

Design note: v0.8.2 does not change library behavior. It clarifies the vocabulary needed before a stable public release.

## v0.8.1

Documentation and generated-code-documentation preparation release on top of v0.8.0.

Added:

- Doxygen-compatible public API comments in public headers.
- Root `Doxyfile` for generated documentation.
- HTML and LaTeX documentation output configuration.
- Optional `CONFIGLIB_BUILD_DOCS` CMake target when Doxygen is available.
- `docs/PUBLIC_API_MAP.md`.
- `docs/DOCUMENTATION_GENERATION.md`.

Changed:

- README now documents how to generate API documentation.
- `docs/CODE_DOCUMENTATION.md` now describes the active documentation policy instead of a future placeholder.
- Project version metadata to `0.8.1`.

Design note: LaTeX output is enabled intentionally as a foundation for later polished PDF manuals.

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
