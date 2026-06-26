# Roadmap

## v0.8.5 completed

- Added `docs/MACHINE_CONFIGURATION_ROADMAP.md`.
- Recorded the long-term railroad beyond v1: configuration-law packages, post-v1 tools, and governed local-machine configuration.
- Explicitly kept this outside the v1 core feature set.
- Marked v0.9 as the next practical engineering phase: C ABI parity and binding-surface stabilization.

## v0.9.0 completed

- Expanded the opaque C ABI across the non-template core.
- Added schema/access-policy/store/transaction/view/export coverage for binding languages.
- Added `docs/C_ABI.md`.
- Kept C++ behavior unchanged.

## v0.9.1 completed

- Added `docs/FIRE_TESTING.md`.
- Recorded the v0.13.x trial-by-fire phase before v1.0.
- Defined the core hardening rule: every rejection must have a reason.
- Captured hostile-input, C ABI abuse, conflict, redaction, transaction, stress, sanitizer, and fuzzing test directions.
- Kept v0.9.1 documentation-only so v0.10 can focus on packaging/install hardening.

## Road to v1.0

Recommended stabilization track:

- `v0.10.x` — packaging, install rules, CMake package export, pkg-config if useful, warning cleanup, sanitizer documentation, fresh-tarball validation, and CI template.
- `v0.11.x` — API freeze candidate and final public-header/C ABI review.
- `v0.12.x` — release-candidate cleanup if needed.
- `v0.13.x` — fire testing: edge cases, hostile inputs, failure semantics, sanitizer/fuzzing/stress tests, and security-boundary review.
- `v1.0.0` — first stable release.

See `FIRE_TESTING.md` for the v0.13 trial-by-fire checklist.

## Next: v0.10 packaging, install, and hardening

The next recommended phase is to make the library easier and safer to consume as a dependency.

Focus areas:

- facts and fact sets
- policy construction
- resolver access
- diagnostics
- resolved-config getters
- schema construction and validation
- store creation and access
- transactions
- scoped views
- export functions
- explicit ownership/lifetime rules for all C handles and returned strings

Do not start plugin ABI, live reload, watcher integration, config-law packages, or machine-management tools until the C ABI stability island exists.

## Post-v1 railroad: configuration tools and machine governance

After v1, `configlib` can become the stable core under a tooling ecosystem:

- `configlint` for mocking, resolving, validating, inspecting, and explaining configuration setups
- `configforge` for generating candidate native configuration from intent plus installed law packages
- `configdoctor` for diagnosing bad or suspicious configuration
- `configapply` for dry-run, backup, apply, verify, and rollback
- `configctl` as a possible operator-facing frontend

The longer rail is not one universal configuration format. It is one governed way to reason about many native configuration systems.

See `MACHINE_CONFIGURATION_ROADMAP.md`.


## v0.8.0 completed

- API cleanup and stabilization-preparation pass.
- Schema/policy/access boundary clarified.
- No silent `Value::as_*` fallback.
- No default-constructible `ConfigView` or `ConfigTransaction`.
- Explicit redacted export mode names.
- API stability, review, lifetime, and code-documentation notes added.

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


## Licensing

`configlib` is released under the MIT License. See `LICENSE` and `docs/LICENSE.md`.

The informal author/development note lives in `docs/AUTHOR_NOTE.md`.


## Documentation status

Programmer manual added in v0.5.5. Future releases should keep language-binding examples aligned with the C ABI and any official binding packages.


## Completed in v0.6.0

- Explicit typed binding helpers over `ConfigView`.


## v0.7.0 completed

- Added in-code schema/contract validation.
- Added validation over `ResolvedConfig` and scoped `ConfigView`.
- Added schema diagnostics for missing required keys, type mismatches, string allow-list violations, and numeric range violations.

## Next stabilization direction

The next recommended phase is API cleanup and naming review before the C ABI stabilization pass. Avoid plugin ABI, live reload, watchers, i18n machinery, or Flowcore integration until the small core API is stable.
