# configlib design notes

## Mechanism versus governing data

The central idea is that the application should not bury configuration behavior in scattered `if` statements.

Bad shape:

```cpp
if (has_cli_flag("--log-level")) {
    config.logging.level = cli_value;
} else if (has_env("APP_LOG_LEVEL")) {
    config.logging.level = env_value;
} else if (file_has("logging.level")) {
    config.logging.level = file_value;
} else {
    config.logging.level = "info";
}
```

Better shape:

```cpp
FactSet facts = collect_facts(...);
PolicySet policy = make_policy(...);
ResolveResult result = resolve(facts, policy);
```

The mechanism is reusable. The governing data changes.

## Application config versus meta-config

There are two levels of configuration:

```text
application config:
    server.port = 8080
    logging.level = debug

meta-config:
    search these paths
    env overrides files
    CLI overrides env
    missing config file is warning, not fatal
```

v0.1 only implements application facts and policies directly.

The `FactRole` enum already has room for:

- application facts
- meta-config facts
- policy facts

This is a hook for the later self-configuring resolver.

## Precedence

Default source precedence in v0.1:

```text
internal default = 10
file             = 30
environment      = 50
CLI              = 60
runtime          = 70
unknown          = 0
```

The values are intentionally configurable.

Typical resolution:

```text
internal defaults < file < environment < CLI < runtime
```

## Why provenance matters

Without provenance, configuration bugs become guesswork.

A serious config library must be able to say:

```text
logging.level = trace
chosen from cli:--log-level
overrode env:MYAPP_LOGGING_LEVEL
overrode file:./app.conf
overrode internal default
```

This version keeps all winning candidates in the resolved entry so explanation is available after resolution.

## Why file parsing is not v0.1

Starting with JSON/TOML/YAML would make the parser look like the project.

It is not.

The project is the policy/fact/resolution model. File parsers are just fact producers.

## Current limitations

- only scalar values
- no file loaders
- no env scanning
- no CLI parser
- no object/array merge
- no redaction/secrets policy
- no policy serialization
- minimal validation model

These are intentional omissions, not conceptual limits.

## v0.2 loader design

v0.2 introduces intake adapters, but deliberately keeps them subordinate to the fact/policy model.

```text
defaults/env/CLI -> facts -> resolver -> resolved config
```

The loader layer exists because real programs need outside-world intake, but this must not become scattered application logic. For example, the environment loader is configured with mappings and a prefix policy; it does not decide whether env should override CLI. That is source precedence in `PolicySet`.

## Internal defaults versus policy defaults

There are now two useful forms of defaults:

```text
FactSet::add_default(...)
InternalDefaultsProvider
```

`FactSet::add_default(...)` is a validation/resolution policy: if the key is absent, synthesize a default during resolution.

`InternalDefaultsProvider` is an intake provider: it emits ordinary internal-default facts before resolution.

Defaults are facts. This keeps provenance explicit and avoids a second source of truth inside policy.

## Environment variables are not magic globals

Environment variables are process-global side channels. `configlib` therefore treats them as disabled unless an `EnvironmentLoaderPolicy` explicitly enables them.

A program should decide:

- whether env intake is enabled
- which prefix is acceptable
- which variables are explicitly mapped
- whether unknown matching variables are ignored, warnings, or errors
- how values are typed

## CLI flags are explicit mappings

The CLI loader does not try to be a full argument parser. It maps known options to facts:

```text
--log-level trace -> logging.level = trace source=cli:--log-level
```

This keeps the loader as a fact producer. More advanced CLI parsing can later be implemented as another replaceable mechanism that emits the same fact model.
