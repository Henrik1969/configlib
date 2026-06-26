# API freeze candidate

`configlib` v0.11 is the pre-freeze cleanup point before the v1 stability track.

The purpose of this stage is to remove public-surface mistakes before they become stable fossils. After the actual freeze candidate is accepted, new work should be additive unless a serious correctness, safety, or security defect is found before v1.0.

## Freeze rule

After the freeze candidate, the public v1 surface should be treated as additive-only.

Allowed after freeze:

- Add new functions.
- Add new types.
- Add new optional behavior behind explicit policy.
- Add diagnostics.
- Add docs, examples, tests, and tools.

Not allowed after freeze without explicit ruling:

- Rename public symbols casually.
- Change getter semantics.
- Change ownership or lifetime rules.
- Change C ABI handle ownership rules.
- Change default resolution behavior silently.
- Remove public functions.
- Make previously valid calls invalid without a migration path.

## v0.11 cleanup rulings

### Defaults are facts

Defaults must enter the system as facts, usually from `FactSet::add_default(...)`, internal-default providers, or `configlib_internal_default_*` in the C ABI.

`PolicySet` does not synthesize defaults. Policy governs resolution; it is not a second source of configuration data.

### No implicit fallback getters

C++ getters named `get_*()` return `std::optional`.

C++ getters named `get_*_or(...)` use an explicit caller-provided fallback.

Removed shorthand aliases such as `get_int(...)`, `get_bool(...)`, and `get_double(...)` because their default arguments could hide missing or mistyped values.

### Same-priority disagreement is diagnosed

If two highest-precedence facts for the same key disagree and no explicit conflict policy resolves the case, resolution emits `CONFIG_AMBIGUOUS_PRECEDENCE`.

Insertion order must not silently become configuration law.

### Key paths have validation

`KeyPath::valid_dotted(...)` and `KeyPath::valid()` define canonical dotted-key validity. The C ABI exposes `configlib_key_is_valid(...)` and rejects malformed dotted keys such as `.a`, `a.`, and `a..b`.

### Non-exportable means not exported

`exportable=false` means a key is omitted from export output, including redacted export modes.

Secret redaction is for values that are exportable but must not reveal their actual value. Non-exportable keys are stronger: they do not appear.

## Final audit question

For every public name and behavior, ask:

> Would I be willing to support this after v1.0?

If the answer is no, fix it before freeze.
