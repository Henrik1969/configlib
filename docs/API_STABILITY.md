# API stability

`configlib` is still pre-1.0.

Version `0.8.0` is the API cleanup and stabilization-preparation release. It is allowed to rename or sharpen public C++ APIs before other projects begin to depend on them.

## Current promise

- The C++ API is source-oriented and may still change before `1.0.0`.
- The C ABI is the intended long-term binding surface, but it is not yet at parity with the C++ feature set.
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
