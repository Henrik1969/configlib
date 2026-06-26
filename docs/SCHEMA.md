# Schema

`ConfigSchema` describes what configuration is allowed to be.

It validates already-resolved configuration. It does not discover files, choose precedence, inject defaults, decide runtime mutability, or decide export/redaction behavior.

## Boundary

```text
Schema:
    presence
    type
    allowed string values
    numeric bounds
    descriptions
    documented defaults

Policy / AccessPolicy:
    source precedence
    conflict handling
    file/env/CLI behavior
    runtime mutability
    exportability
    secret/redaction rules
```

## Example

```cpp
configlib::ConfigSchema schema;

schema.path(configlib::KeyPath("xyz.logging.level"))
    .string()
    .required()
    .documented_default(configlib::Value("info"))
    .allowed({"trace", "debug", "info", "warn", "error"})
    .describe("Logging verbosity.");

schema.path(configlib::KeyPath("xyz.server.port"))
    .integer()
    .required()
    .range(static_cast<std::int64_t>(1), static_cast<std::int64_t>(65535))
    .documented_default(configlib::Value(8080));

schema.path(configlib::KeyPath("xyz.ai.remote_allowed"))
    .boolean()
    .optional()
    .documented_default(configlib::Value(false));
```

Then validate a resolved config:

```cpp
auto resolved = configlib::resolve(facts, policies);
auto checked = schema.validate(resolved.config());

if (!checked.ok()) {
    std::cerr << checked.diagnostics().format();
}
```

Or validate a scoped view:

```cpp
auto store = configlib::ConfigStore::from_result(resolved, policies);
auto logging = store.view(configlib::KeyPath("xyz.logging"));

auto checked = logging_schema.validate(logging);
```

## Defaults

Authoritative defaults are facts:

```cpp
facts.add_default(configlib::KeyPath("xyz.logging.level"), configlib::Value("info"));
```

Schema defaults are documented defaults only:

```cpp
schema.path(configlib::KeyPath("xyz.logging.level"))
    .string()
    .documented_default(configlib::Value("info"));
```

A documented default helps documentation, tooling, and future generated UIs, but it does not inject a value into the resolver.

## Access and secrets

Access rules are not schema rules.

Use `AccessPolicy` for runtime mutation, exportability, secret handling, and reset behavior:

```cpp
configlib::AccessPolicy access;
access.runtime_mutable(configlib::KeyPath("xyz.server.port"), false)
      .secret(configlib::KeyPath("xyz.auth.token"))
      .exportable(configlib::KeyPath("xyz.debug.internal"), false);
```

## Design rule

```text
Schema validates shape.
Policy governs behavior.
AccessPolicy governs runtime and export access.
Defaults are facts.
```
