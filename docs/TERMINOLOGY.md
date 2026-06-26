# configlib terminology

`configlib` uses a few words in a specific way. This document explains those words for readers who are new to the project or who are used to smaller configuration helpers.


For a less formal introduction with a metaphor and a commented example, see [`EXPLAINED_SIMPLY.md`](EXPLAINED_SIMPLY.md).

The short version:

> `configlib` treats configuration as governed data: values come from explicit sources, are resolved by explicit rules, validated by schema, exposed through controlled stores/views, and explained through diagnostics and provenance.

## Governance

**Governance** is the explicit rule layer around configuration.

It covers questions such as:

- Where may configuration values come from?
- Which source wins when several sources define the same key?
- Which values are valid?
- Which settings may be changed at runtime?
- Which values may be exported?
- Which values are secrets and must be redacted?
- How does the library explain the final resolved value?

Ungoverned configuration is just a pile of values. Governed configuration is values plus rules, provenance, validation, access control, mutation control, export control, and diagnostics.

Example:

```text
server.port = 8080
```

With governance, the system may also know:

```text
source: internal default
schema: integer, range 1..65535
policy: CLI/env/file may override; runtime mutation is denied
diagnostics: report an error if the value is not a valid port
```

## Fact

A **fact** is a raw claim about configuration.

Examples:

```text
logging.level = debug
server.port = 8080
ai.remote_allowed = false
```

A fact is not automatically the final truth. It is an input claim from some source. Several facts may claim different values for the same key; policy decides which one wins.

Facts should carry source/provenance information so the final result can be explained.

## FactSet

A **FactSet** is a collection of facts.

Loaders and providers produce facts, and the resolver consumes them.

Typical fact sources:

- internal defaults
- config files
- environment variables
- command-line arguments
- runtime overrides

## Source

A **source** describes where a fact came from.

Examples:

```text
internal-default
file: ./xyz.conf
env: XYZ_LOG_LEVEL
cli: --log-level
runtime
```

Sources matter because they are used for precedence, diagnostics, and explanation.

## Provenance

**Provenance** means origin/history information for a value.

For a resolved value, provenance answers:

- Where did the winning value come from?
- What other values did it override?
- Which source produced each competing value?

Example explanation:

```text
logging.level = trace
chosen from: cli: --log-level trace
overrode: env: XYZ_LOG_LEVEL=debug
overrode: file: ~/.config/xyz/xyz.conf -> logging.level = info
overrode: internal-default -> logging.level = warn
```

## KeyPath

A **KeyPath** is a dotted configuration key.

Examples:

```text
logging.level
server.port
xyz.ai.remote_allowed
```

A `KeyPath` is a semantic configuration path, not a filesystem path.

## Value

A **Value** is a typed scalar configuration value.

Current scalar categories include:

- null
- boolean
- integer
- floating-point
- string

`Value::as_*()` functions do not silently invent fallback values. If a fallback is wanted, callers must use the explicit `_or` form, such as `as_string_or(...)`.

## Policy

A **policy** describes how configuration behaves.

Policy answers questions such as:

- Which source has higher precedence?
- How are conflicts handled?
- How are config files discovered?
- How are environment variables mapped?
- How are command-line arguments mapped?
- Which runtime mutations are allowed?
- Which values may be exported?
- Which values are secrets?

Policy governs behavior. It does not describe only the shape of valid data; that is schema's job.

## PolicySet

A **PolicySet** is the collection of policies used by the resolver and runtime store.

`PolicySet` is about behavior, not merely type checking.

## AccessPolicy

An **AccessPolicy** controls runtime access behavior.

It decides things such as:

- whether a key may be changed at runtime
- whether a key may be reset
- whether a key may be exported
- whether a key is secret and should be redacted

Access rules are policy, not schema.

## Schema

A **schema** describes the allowed shape of configuration.

Schema answers questions such as:

- Is this key required or optional?
- What type must the value have?
- Is a string restricted to a set of allowed values?
- Is an integer or floating-point value restricted by min/max/range?

Schema validates shape. Policy governs behavior.

## ConfigSchema

A **ConfigSchema** is a set of schema rules.

Example:

```cpp
configlib::ConfigSchema schema;

schema.path(configlib::KeyPath("logging.level"))
    .string()
    .required()
    .allowed({"trace", "debug", "info", "warn", "error"})
    .documented_default(configlib::Value("info"));
```

The `documented_default()` call records default information as documentation/metadata. It does not inject a default fact.

## Default

A **default** is the value used when no higher-precedence source supplies a value.

In `configlib`, the authoritative source of defaults is facts.

Recommended pattern:

```text
internal defaults provider -> default facts -> resolver
```

Schema may document expected defaults, and bindings may provide local fallback values, but those are not the authoritative default mechanism.

Design rule:

> Defaults are facts.

## Resolver

The **resolver** applies policy to facts and produces resolved configuration.

It decides which fact wins when multiple facts define the same key. It also records diagnostics and explanation/provenance information.

## ResolveResult

A **ResolveResult** is the result of resolving facts under policy.

It contains:

- success/failure state
- resolved configuration
- diagnostics
- provenance/explanation data

A result is an operation outcome whose success/failure matters and which may contain a primary value.

## Report

A **report** is an informational summary of work performed.

A report may contain diagnostics and metadata, but it is primarily a summary rather than the main resolved value of the operation.

Naming rule:

- use **Result** when success/failure and a primary returned value matter
- use **Report** for informational summaries

## Diagnostic

A **diagnostic** is a structured message produced by the library.

Diagnostics may describe:

- errors
- warnings
- informational notes
- validation failures
- denied runtime mutations
- missing files
- type mismatches

Diagnostics are intended for humans and tools. They should be more useful than a plain `false` return value.

## DiagnosticLog

A **DiagnosticLog** is a collection of diagnostics.

It lets callers inspect everything the library noticed during loading, resolving, validating, storing, exporting, or binding.

## ResolvedConfig

A **ResolvedConfig** is the configuration after facts have been resolved under policy.

It is the resolved value set before it is wrapped in the runtime `ConfigStore`.

## ConfigStore

A **ConfigStore** is the governed runtime container for resolved configuration.

It provides:

- access to resolved values
- diagnostics
- transactions for runtime changes
- reset behavior
- export behavior
- scoped views

Short phrase:

> `ConfigStore` is the governed runtime vault.

## ConfigTransaction

A **ConfigTransaction** stages runtime changes before committing them.

It is used so callers can:

- stage changes
- validate them
- commit them
- roll them back/drop them

A transaction is not a free-for-all global variable editor. It is checked against policy.

## ConfigView

A **ConfigView** is a scoped read-only view into part of a `ConfigStore`.

Example:

```cpp
auto logging = store.view(configlib::KeyPath("logging"));
logging.get_string(configlib::KeyPath("level")); // reads logging.level
```

Short phrase:

> `ConfigView` is a scoped keyhole into one subtree of the store.

A view borrows store state and must not outlive the `ConfigStore` that created it.

## Binding

A **binding** projects governed configuration into an application-owned typed structure.

Example:

```cpp
struct LoggingConfig {
    std::string level;
    bool color;
};
```

A `StructBinding<LoggingConfig>` can read fields from a `ConfigView` and produce a `LoggingConfig` value.

Bindings are convenience projections. They are not the source of truth.

Design rule:

> `ConfigStore` remains truth. Bindings are snapshots/projections.

## Loader

A **loader** reads some external or caller-supplied input and produces facts.

Examples:

- environment loader
- CLI loader
- config-file loader
- internal defaults provider

Loaders do not own application meaning. They produce facts with sources and diagnostics.

## Provider

A **provider** is similar to a loader, but often produces facts from an internal or service-backed source rather than parsing a file or argv.

The internal defaults provider is the simplest example.

Future providers may include secret providers, remote config providers, schema providers, or plugin-backed providers.

## Export

**Export** means producing a textual or structured representation of configuration.

Export is governed by policy. Some values may be hidden, redacted, or excluded.

Examples:

- effective config export
- redacted effective config export
- changed-only export
- view-local export

## Redaction

**Redaction** means hiding sensitive values when exporting or displaying configuration.

Example:

```text
auth.token = <redacted>
```

Redaction is policy/access behavior, not schema shape.

## Secret

A **secret** is a configuration value that must not be casually shown, exported, logged, or exposed.

Examples:

- API tokens
- passwords
- private keys

In the current library, secret handling is export/redaction policy. Future versions may add stronger secret-provider mechanics.

## Runtime mutation

**Runtime mutation** means changing configuration while the program is running.

In `configlib`, runtime mutation should happen through `ConfigTransaction`, not by directly editing random global state.

Policy decides which keys may be changed.

## Scoped access

**Scoped access** means giving a subsystem access only to the part of configuration it needs.

Example:

```cpp
auto parser_cfg = store.view(configlib::KeyPath("flowmini.parser"));
```

The parser sees local keys such as `recover` or `trace`, not the entire application configuration universe.

## Projection

A **projection** is a derived view or representation of configuration.

Examples:

- `ConfigView` is a scoped projection over a `ConfigStore`
- `StructBinding<T>` is a typed struct projection over a `ConfigView`
- exported text is a serialized projection

Projections are useful, but the store remains the governed truth.

## C ABI

The **C ABI** is the C-callable interface exposed by `configlib`.

It is intended to be the long-term binding surface for other languages such as Python, Rust, Zig, Lua, or Flowcore tooling.

The C ABI should use opaque handles and explicit ownership rules. It must not expose C++ STL types or throw C++ exceptions across the boundary.

## Opaque handle

An **opaque handle** is a pointer-like C API object whose internals are hidden from the caller.

Example style:

```c
typedef struct configlib_ctx configlib_ctx;
```

The caller can pass the handle back to library functions but cannot inspect its internal layout.

Successful operations should return valid handles. Empty is allowed. Invalid is not.

## Stable API

A **stable API** is a public interface that downstream projects can rely on without constant rewrites.

Before v1.0, `configlib` may still make cleanup changes. v1.0 is intended to become the first stable dependency target.

## Tiny tool boundary

`configlib` is not meant to replace every simple argument parser.

For tiny tools, plain `argc`/`argv`, `strtol()`, or a small CLI parser is often the correct choice.

Use `configlib` when configuration becomes a runtime governance problem: multiple sources, precedence, validation, runtime mutation, scoped subsystem views, export/redaction, provenance, and diagnostics.

## Summary of major boundaries

```text
Facts      = input claims
Sources    = where facts came from
Policy     = behavior rules
Schema     = shape/validation rules
Resolver   = policy-applies-to-facts mechanism
Store      = governed runtime container
View       = scoped read-only keyhole
Binding    = typed projection into app structs
Diagnostics= structured complaints/notes/explanations
```

## Configuration swamp

The **configuration swamp** is the practical mess caused by many unrelated tools and systems each having their own configuration language, file locations, defaults, precedence rules, reload behavior, validators, version differences, and undocumented folklore.

`configlib` does not try to eliminate native configuration formats. The long-term idea is to make their rules discoverable, governed, explainable, validated, and reversible.

## Configuration law

**Configuration law** is the long-term idea that a project, distribution, vendor, site, or user can ship machine-readable rules for a configuration surface.

A law package may describe schemas, policies, file locations, source precedence, validators, probes, templates, migrations, risk metadata, rollback requirements, and version-specific behavior.

Configuration law describes the target. Tools such as `configlint`, `configforge`, or `configapply` would operate using that law.

See `MACHINE_CONFIGURATION_ROADMAP.md` for the long-term roadmap.


## Fire testing

Fire testing is the planned pre-v1 hardening phase where configlib is deliberately tested with edge cases, hostile input, malformed files, conflicting facts, C ABI misuse, redaction risks, transaction failures, stress loads, and sanitizer/fuzzing tools.

The goal is not to prove the happy path again. The goal is to prove that configlib fails safely, explains itself, and preserves state.

See [`FIRE_TESTING.md`](FIRE_TESTING.md).
