# Fire testing roadmap

`configlib` must not reach v1.0 merely because the happy path works.

The v0.13.x fire-testing series is the deliberate "try to break it" phase before the first stable release. Its purpose is to prove that the library fails safely, explains itself, preserves state, and does not turn bad input into silent mystery behavior.

This document is a roadmap and acceptance checklist. It does not add new runtime behavior by itself.

## Guiding sentence

> v0.13 is not about proving configlib works. It is about proving configlib fails safely, explains itself, and preserves state.

## Core rule

Every rejection must have a reason.

If a fact is ignored, rejected, overwritten, redacted, denied, or made invisible, there must be a diagnostic or explanation path that says why.

The user should be able to distinguish:

- key not found
- wrong type
- invalid value
- unknown key
- forbidden source
- denied mutation
- denied export
- redacted secret
- ambiguous conflict
- invalid handle
- internal failure

No silent guessing.

## Safety boundary

The core library must remain non-destructive.

`configlib` itself must not:

- edit `/etc`
- write bootloader files
- restart services
- execute validators
- apply system changes
- mutate the local machine outside the caller-owned process state

Those actions belong to later tools such as `configapply`, not to the core library.

The core safety question is therefore:

> Can broken, hostile, oversized, conflicting, or badly ordered input corrupt configlib state, leak secrets, bypass policy, or produce unexplained results?

## Conflict and precedence fire tests

### Same key, different source priority

Expected behavior:

- the higher-priority source wins
- the losing candidates remain explainable
- diagnostics are emitted when policy says the conflict should be reported

### Same key, same priority, incompatible values

This is a critical swamp-avoidance case.

Example:

```text
file:a.conf says logging.level = debug
file:b.conf says logging.level = warn
both have the same source kind
both have the same priority
no explicit tie-break policy exists
```

Preferred conservative default:

```text
same priority + same key + incompatible value + no tie rule = diagnostic complaint
```

Possible explicit future tie policies:

- `RejectAmbiguous`
- `FirstWins`
- `LastWins`
- `SourceOrderWins`
- `MergeIfCompatible`

If the library chooses a winner, the reason must be explainable.

### Duplicate identical facts

Expected behavior:

- no corruption
- deterministic result
- optional diagnostic depending on verbosity/policy

### Conflicting defaults

Expected behavior:

- defaults are still facts
- duplicate/conflicting defaults follow the same conflict rules as other facts
- schema `documented_default()` never becomes an injected value

## False-claim fire tests

A false claim is any configuration claim that should not be accepted as final truth.

Examples:

- unknown key
- wrong type
- invalid enum value
- value outside numeric range
- forbidden source
- source not allowed to override this key
- runtime mutation denied by access policy
- secret requested through non-redacted export
- malformed config-file line
- malformed CLI mapping
- invalid environment mapping

Expected behavior:

- strict mode rejects or fails validation
- permissive mode may keep extra facts, but must warn if policy says so
- wrong types and invalid values must not become final values silently
- forbidden values must be rejected with diagnostics
- forbidden sources must be ignored or rejected according to policy, with explanation

## Schema rejection fire tests

Test cases should cover:

- missing required keys
- present optional keys
- unknown keys under strict schema mode
- boolean expected, string supplied
- integer expected, float/string supplied
- string expected, integer supplied
- string allow-list rejection
- integer min/max/range rejection
- floating min/max/range rejection
- documented defaults not being injected
- diagnostics pointing at the offending key and source when possible

Expected behavior:

- validation failure does not mutate resolved config
- every failure has a diagnostic
- diagnostics remain stable enough for tests to assert codes/categories, not brittle prose

## Store, view, and access-policy fire tests

Test cases should cover:

- `ConfigView` cannot escape its prefix
- scoped view access to sibling/outside keys fails
- hidden/export-denied keys do not appear in exports
- secrets are redacted in redacted export modes
- unredacted export behavior follows explicit access policy
- changed-only export does not leak hidden values
- runtime mutation denied for immutable keys
- reset denied where reset policy forbids it
- `reset_to_default` cannot invent schema documented defaults as facts
- diagnostics explain denied mutation/export/reset attempts

Expected behavior:

- access policy is enforced consistently from store and view APIs
- read-only views stay read-only
- failed operations preserve store state

## Transaction fire tests

Test cases should cover:

- staged set then rollback
- staged erase then rollback
- staged reset then rollback
- validate failure then commit attempt
- double commit
- rollback after commit
- commit after rollback
- transaction created from a store, then store remains stable after transaction failure
- multiple independent transactions if supported by contract

Expected behavior:

- failed transaction does not partially mutate the store
- rollback returns to previous effective state
- commit is atomic at the store level
- invalid transaction lifecycle is rejected or diagnosed according to the API contract

## Export and redaction fire tests

Test cases should cover:

- effective export
- effective redacted export
- changed-only export
- changed-only redacted export
- runtime-only export
- runtime-only redacted export
- scoped full-path export
- scoped local export
- secret values in nested/scoped views
- export-denied keys

Expected behavior:

- secrets are never leaked through redacted modes
- export-denied keys are omitted or diagnosed according to policy
- local scoped export strips only the intended prefix
- exports are deterministic enough for testing

## C ABI abuse tests

The C ABI is the binding-language surface, so it must tolerate clumsy callers.

Test cases should cover:

- null handles
- null strings
- empty strings
- destroyed handles where the contract allows detection
- double-destroy behavior if the contract defines it
- getters with missing keys
- getters with wrong type
- caller-owned buffers too small, if buffer APIs are added later
- borrowed string lifetime documented and tested where practical
- C functions never throw C++ exceptions across the ABI

Expected behavior:

- invalid inputs return status codes or safe nulls according to contract
- no C++ exception crosses the C ABI
- no memory leak, use-after-free, or double free under sanitizer runs

## Loader and parser fire tests

Test cases should cover:

- empty files
- files without trailing newline
- malformed `key=value` lines
- duplicate sections
- deeply nested section/key combinations
- extremely long keys
- extremely long values
- invalid numeric syntax
- strings that look numeric but are not
- invalid UTF-8 or unusual bytes, if supported/rejected by policy
- environment variables with bad prefixes
- CLI options missing values
- duplicate CLI options
- unknown CLI options

Expected behavior:

- malformed input produces diagnostics
- parser failure does not corrupt accumulated facts
- large input does not hang or blow up memory unexpectedly

## Large-input and stress tests

Test cases should cover:

- thousands of facts
- many facts for the same key
- many unique keys
- deep key paths
- large string values
- repeated resolve operations
- repeated store creation/destruction
- repeated C ABI create/destroy loops

Expected behavior:

- deterministic results
- bounded/reasonable memory behavior
- no leaks under sanitizer or valgrind-style runs
- no pathological slowdowns for ordinary large inputs

## Security-boundary tests

The library should be tested against misuse patterns:

- secret leakage through export
- bypassing access policy through scoped views
- mutating immutable keys through transactions
- treating schema documented defaults as real facts
- misleading provenance after conflict resolution
- confusing diagnostics that hide the losing candidate
- integer overflow in priority or numeric parsing
- denial-of-service through oversized input

Expected behavior:

- policy cannot be bypassed by using a different public access path
- provenance remains truthful
- diagnostics do not claim a value came from a source it did not come from

## Sanitizer builds

v0.13 should document and run sanitizer builds regularly.

Example AddressSanitizer/UndefinedBehaviorSanitizer pass:

```sh
cmake -S . -B build-asan -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined"

cmake --build build-asan -j"$(nproc)"
ctest --test-dir build-asan --output-on-failure
```

Additional hardening tools to consider:

- valgrind
- clang-tidy
- cppcheck
- coverage reports
- fuzzing
- ABI/API compatibility checking

## Fuzzing candidates

Good fuzz targets include:

- `KeyPath` parsing
- `Value` parsing
- simple config-file loader
- environment key mapping
- CLI option mapping
- C ABI string/handle boundary helpers

A fuzz target does not need semantic success. It needs:

- no crash
- no hang
- no memory corruption
- no invalid state escape
- diagnostics when rejected

## Regression rule

Every bug found during fire testing should become a regression test.

A fire-test regression should state:

- what was attacked
- what went wrong
- what must happen now
- what diagnostic/status is expected
- what state must remain unchanged

## v1 gate

Before v1.0, the release question is:

> Can we trust this library when users are wrong, files are broken, bindings are clumsy, and attackers are annoying?

If the answer is yes, v1.0 is earned.
