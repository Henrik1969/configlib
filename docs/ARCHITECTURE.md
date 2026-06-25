# configlib architecture

## Purpose

`configlib` is not merely a config parser.

It is intended to become a configuration discovery, intake, precedence, policy, and resolution engine that can be linked into C/C++ programs and exposed through stable ABI bindings to other languages.

The core philosophy is separation of mechanism from governing data.

```text
mechanism:
    discover / load / normalize / resolve / validate / emit

governing data:
    facts / policies / defaults / source rules / precedence / constraints
```

The mechanism should be replaceable by another implementation without changing the facts and policies supplied to it.

## Design law

```text
All inputs become facts.
All behavior is governed by policy.
The mechanism only discovers, normalizes, resolves, validates, and emits.
No application meaning is hardcoded into the mechanism.
```

## Current pipeline

v0.5 pipeline:

```text
InternalDefaultsProvider   EnvironmentLoaderPolicy   CliLoaderPolicy   FileDiscoveryPolicy
          |                         |                       |                  |
          v                         v                       v                  v
   internal-default facts       env facts                cli facts          file facts
          \________________________|_______________________|__________________/
                                   |
                                   v
                          FactSet + PolicySet
                                   |
                                   v
                                resolver
                                   |
                                   v
                    ResolvedConfig + DiagnosticLog
```

## Core objects

### Value

A scalar configuration value.

Current values:

- null
- bool
- int64
- double
- string

Arrays and objects are intentionally left for a later version because merge semantics must be designed deliberately.

### KeyPath

A dotted key path such as:

```text
logging.level
server.port
window.width
```

Internally it stores both the dotted form and split parts.

### Source

A provenance record describing where a fact came from.

Current source kinds:

- internal default
- file
- environment
- CLI
- runtime
- unknown

### Fact

A fact is a raw claim about a configuration key.

```text
key = value
source = where it came from
precedence = how strongly it wins
role = application / meta-config / policy
```

Examples:

```text
logging.level = info   source=internal-default
logging.level = warn   source=file:./app.conf
logging.level = debug  source=env:MYAPP_LOGGING_LEVEL
logging.level = trace  source=cli:--log-level
```

### PolicySet

The governing data that tells the mechanism how to interpret facts.

Current policy areas:

- source precedence
- required keys
- optional keys
- defaulted keys
- expected scalar type
- string allow-list
- integer min/max range
- conflict strategy

### Loaders

v0.2 adds the first intake loaders:

- internal defaults provider
- environment loader
- CLI loader

These loaders do not resolve configuration. They only produce facts and diagnostics.

### Resolver

The resolver groups facts by key, validates them, applies conflict/precedence policy, and emits a resolved configuration view.

### ResolvedConfig

A queryable result containing chosen values and provenance.

It can explain why a value won:

```text
logging.level = trace
chosen: cli:--log-level
candidates:
  - precedence=60 source=cli:--log-level value=trace
  - precedence=50 source=env:MYAPP_LOGGING_LEVEL value=debug
  - precedence=10 source=internal-default:internal-defaults-provider value=info
```

### DiagnosticLog

Diagnostics are first-class output, not a side channel.

Examples:

- missing required key
- type mismatch
- integer out of allowed range
- string not in allow-list
- conflicting values
- env parse failure
- CLI missing value
- CLI parse failure

## Future full pipeline

```text
meta-policy
    |
    v
discover config sources
    |
    v
load files / env / CLI / defaults
    |
    v
normalize into facts
    |
    v
resolve according to policy
    |
    v
validate
    |
    v
emit resolved configuration + diagnostics + provenance
```

## v0.4 runtime store layer

v0.4 adds the runtime store after resolution.

```text
Internal defaults / env / CLI / files
        |
        v
FactSet + PolicySet
        |
        v
resolve()
        |
        v
ResolvedConfig
        |
        v
ConfigStore
        |
        v
governed query / transaction / reset / export
```

The store exists because many programs want a giant configuration struct in practice, but a giant public struct becomes noisy and hard to govern. `ConfigStore` gives the same practical benefit while preserving policy, validation, provenance, and controlled mutation.

Runtime mutation is staged through `ConfigTransaction`. A transaction must validate before commit. Rejected commits leave the store unchanged.

Access and export are governed by `AccessPolicy`, not hidden application convention.

```text
key may be runtime mutable
key may be startup-only
key may be secret
key may be exportable
key may be resettable
```

This keeps the mechanism replaceable and prevents the store from becoming an unstructured global variable bucket.


## v0.5 scoped view layer

v0.5 adds a read-only scoped view over `ConfigStore`.

```text
ResolvedConfig
    |
    v
ConfigStore
    |
    +--> ConfigView("logging")
    +--> ConfigView("database")
    +--> ConfigView("flowmini.parser")
```

The store remains the governed runtime vault. A view is a scoped keyhole into one subtree.

This lets larger applications hand a subsystem only the configuration surface it owns:

```cpp
Logger logger(store.view(KeyPath("logging")));
Parser parser(store.view(KeyPath("flowmini.parser")));
```

Views are read-only in v0.5. Mutation remains transaction-based through `ConfigStore` so write policy does not leak into convenience views too early.

## Future plugin direction

The core must not assume one blessed source format, localization system, secret provider, or presentation layer. Future plugins may provide loaders, emitters, i18n catalog resolution, schema providers, secret providers, and remote config providers. See `PLUGIN_MODEL.md`.

## Tool choice boundary

`configlib` is a runtime configuration governance mechanism, not a universal replacement for simple argument parsing. Tiny programs should keep using simple `argv` handling, `strtol()`, or ordinary command-line parsers when that is the clearest solution.

The architecture becomes useful when configuration has several fact sources, explicit precedence, validation, provenance, runtime mutation, scoped access, and export/redaction policy.

```text
argv parsing is for command invocation.
configlib is for governed application configuration.
```
