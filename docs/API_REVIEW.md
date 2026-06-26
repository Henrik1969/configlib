# API review notes for v0.8.0

This release is a drydock inspection before `configlib` is used as a dependency by larger projects.

## Decisions applied

### Schema vs policy

Schema describes what configuration is allowed to be: presence, type, allowed values, numeric bounds, documentation, and documented defaults.

Policy describes how configuration is discovered, resolved, accessed, mutated, and exported.

### Access policy

Runtime mutability, exportability, secret handling, and reset behavior belong to `AccessPolicy`, not `ConfigSchema`.

### Defaults

Authoritative defaults are facts, normally produced by internal-default loaders/providers. Schema may document a default using `documented_default()`, but schema does not inject facts. Binding fallbacks are local convenience only.

### Value conversion

`Value::as_string()`, `as_integer()`, `as_boolean()`, and `as_floating()` return `std::optional`. Fallback behavior is explicit through `as_*_or()`.

### ConfigView construction

`ConfigView` is no longer default constructible. Successful view creation through `ConfigStore::view()` returns a valid view. Empty views are valid scoped views with no keys, not invalid handles.

### Diagnostics ownership

`ConfigStore::diagnostics()` returns a const reference in the C++ API. If a snapshot-returning API is needed later, it should be named explicitly.

### Export modes

`ExportMode::Redacted` has been replaced by explicit names such as `EffectiveRedacted`, with compatibility aliasing retained for now.

### C ABI

The C ABI is intentionally not fully expanded in v0.8.0. v0.9.0 is reserved for C ABI parity and stabilization over the non-template core.
