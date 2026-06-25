# configlib roadmap

## Implemented

### v0.1: in-memory core

- value model
- key paths
- source/provenance
- facts
- policy set
- resolver
- diagnostics
- explanation/provenance output
- C++ API
- opaque C ABI
- examples
- tests

### v0.2: intake adapters

- environment loader
- CLI option mapper
- internal defaults provider
- loader diagnostics
- loader example
- loader tests
- changelog

### v0.3: file discovery and simple file loading

- file discovery policy
- explicit config file injection
- missing-file absence policy
- internal defaults fallback path
- simple key=value loader
- section support
- file provenance with line numbers

### v0.4: runtime store

- `ConfigStore`
- runtime transaction staging
- commit/rollback
- reset to base/default
- access policy
- export policy
- secret redaction

### v0.5: scoped views

- `ConfigView`
- scoped getters
- fallback getters
- subtree key listing
- scoped explain
- scoped full/local export

## Likely v0.6 direction

Typed binding helpers are a strong next candidate. They would allow old-school struct convenience without giving up configlib provenance and policy.

Possible scope:

- bind a subtree view into a typed struct
- explicit field mapping
- diagnostic output on missing/type-invalid fields
- optional fields and fallback policy
- no code generation yet

## Later directions

- mutable scoped views with explicit write policy
- reload strategy and change notifications
- watcher integration
- object/array value model
- deliberate merge semantics
- JSON/TOML adapters as plugins or optional components
- schema/policy import/export
- generated config templates
- richer emitters
- policy-controlled persistence of changed-only configuration
- C ABI expansion for store/view features
- plugin ABI
- i18n/message catalog integration using stable monikers
- secret providers
- remote config providers

## Long-term direction

`configlib` is intended as a reusable brick used by:

- ordinary C/C++ applications
- CLIs
- daemons
- GUI/TUI applications
- language tooling
- Flowcore/Flowmini runtime policy envelopes
- plugin systems
- larger governed runtime environments

The mechanism should remain replaceable as long as the fact/policy/result/store/view contracts stay stable.

## Non-goal reminder

`configlib` should remain a power tool, not framework tax. Future versions must preserve the boundary documented in `WHEN_TO_USE.md`: do not force the library into tiny programs where plain argument parsing is cleaner.
