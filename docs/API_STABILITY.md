# API stability

`configlib` is still pre-1.0.

Version `0.13.1` is a fire-testing follow-up release after the release-candidate preparation point. The intended v1 public surface remains additive-only unless fire testing proves a correctness, safety, security, or serious usability defect that must be fixed before v1.0. v0.13.1 adds stable version-string helpers because external-consumer and binding smoke tests proved that this small ABI/API affordance should exist before v1.

## Current promise

- The C++ API is an API-freeze candidate and should only change additively unless fire testing proves a defect.
- The C ABI is the intended long-term binding surface for non-C++ languages and should only change additively unless fire testing proves a defect.
- `1.0.0` will mark the first stable dependency target.

## Naming rules

- `get_<thing>()` returns an optional value when the key or type may be absent.
- `get_<thing>_or()` returns a value using an explicit fallback supplied by the caller.
- `set_<thing>()` stages or writes a value of the named type.
- `_or` is the only public spelling that means fallback.

## Semantic rules

- No silent fallback.
- No invalid handles from successful operations.
- Authoritative defaults are facts.
- Schema validates shape.
- Policy governs behavior.
- AccessPolicy governs runtime mutation, export, secrets, and reset rules.
- Bindings project governed configuration into ordinary structs.

## Result vs report

A `Result` is an operation outcome whose success/failure matters and that may carry a primary value.

A `Report` is an informational summary of work performed, usually with diagnostics and metadata, but not necessarily a primary value.
