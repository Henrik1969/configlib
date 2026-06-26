# Public API map

This document gives a quick orientation map for the public `configlib` API as of `v0.13.1`.

## Core data

- `Value` — scalar configuration value: null, boolean, integer, floating point, string.
- `ValueType` — runtime type tag for `Value`.
- `KeyPath` — dotted semantic configuration path such as `logging.level`.
- `Source` / `SourceKind` — provenance of a fact or chosen resolved value.
- `Fact` — one raw configuration claim.
- `FactSet` — ordered collection of facts.

## Resolution

- `PolicySet` — source precedence, missing-key behavior, conflict behavior, and legacy/simple key policy rules.
- `ResolveResult` — result of resolving facts with policy.
- `ResolvedConfig` — immutable resolved key/value collection with provenance and explanation.
- `ResolvedEntry` — chosen value for one key plus source/candidate information.

## Validation

- `ConfigSchema` — validates the allowed shape of resolved configuration.
- `SchemaRule` — one schema rule for one key.
- `SchemaPathBuilder` — fluent builder for schema rules.
- `SchemaValidationResult` — validation success/failure and diagnostics.

Schema validates shape. It does not own runtime mutation, export, or secret behavior.

## Runtime access

- `ConfigStore` — governed runtime configuration owner.
- `ConfigTransaction` — staged runtime mutation object.
- `ConfigView` — read-only scoped non-owning view into a `ConfigStore` subtree.
- `AccessPolicy` — runtime mutability, exportability, redaction/secret, and reset rules.
- `ExportMode` — effective/changed/runtime export modes with redacted variants.

## Projection

- `StructBinding<T>` — explicit C++ typed projection from a `ConfigView` into an application struct.
- `BindingResult<T>` — projected struct plus binding diagnostics.

Bindings are convenience projections. They are not the source of governed truth.

## Input/loaders

- `InternalDefaultsProvider` — authoritative default facts supplied by the application.
- `EnvironmentLoaderPolicy` — maps environment variables into facts.
- `CliLoaderPolicy` — maps raw argv tokens into facts.
- `FileDiscoveryPolicy` — finds config files according to policy.
- `LoadReport` — facts and diagnostics produced by a loader.
- `DiscoveryReport` — discovered files plus load report.

## Diagnostics

- `Diagnostic` — one diagnostic event with severity, code, message, key, and source.
- `DiagnosticLog` — collection of diagnostics with formatting and error detection.

## C ABI

- `configlib.h` — opaque C ABI intended as the long-term binding surface for non-C++ languages.

The v0.12 C ABI covers the non-template core: facts, policy construction, resolution, diagnostics, schema validation, access policy, store creation, transactions, scoped views, export, file/env/CLI loading, and scalar getters. C++ templates such as `StructBinding<T>` remain C++-only by design. See [`C_ABI.md`](C_ABI.md).

## Packaging and installed consumption

- `cmake/configlibConfig.cmake.in` — template for the installed CMake package file.
- `pkgconfig/configlib.pc.in` — template for the installed pkg-config metadata.
- Installed CMake targets:
  - `configlib::configlib` — convenience target; shared if available, otherwise static.
  - `configlib::shared` — shared library target when installed.
  - `configlib::static` — static library target when installed.

See [`PACKAGING.md`](PACKAGING.md) for install and consumer examples.

For API freeze cleanup notes, see [`API_FREEZE_CANDIDATE.md`](API_FREEZE_CANDIDATE.md). For the v1 release-candidate checklist and additive-only rule, see [`RELEASE_CANDIDATE.md`](RELEASE_CANDIDATE.md).

For vocabulary and conceptual definitions, see [`TERMINOLOGY.md`](TERMINOLOGY.md). For a newcomer-friendly explanation, see [`EXPLAINED_SIMPLY.md`](EXPLAINED_SIMPLY.md).


For practical build/use/install/binding instructions, see [`HOWTO.md`](HOWTO.md).

## Long-term tooling roadmap

`configlib` is the small governed configuration core. Long-term tools may grow around it after v1, including configuration-law packages and tools such as `configlint`, `configforge`, `configdoctor`, and `configapply`.

See [`MACHINE_CONFIGURATION_ROADMAP.md`](MACHINE_CONFIGURATION_ROADMAP.md).


## Pre-v1 hardening roadmap

The planned v0.13 fire-testing phase is documented in [`FIRE_TESTING.md`](FIRE_TESTING.md). It focuses on hostile inputs, edge cases, C ABI abuse, sanitizer/fuzzing passes, redaction safety, transaction failure behavior, and the rule that every rejection must have a reason.
