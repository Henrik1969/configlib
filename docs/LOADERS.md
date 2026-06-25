# Intake loaders

v0.2 adds the first intake adapters.

The design rule is:

```text
Loaders produce facts.
Policies govern facts.
The resolver decides the final view.
```

A loader is not allowed to own application meaning. It may only map an external input into a `Fact` with a `Source`, `Value`, key path, and precedence.

## LoadReport

Each loader returns:

```cpp
struct LoadReport {
    FactSet facts;
    DiagnosticLog diagnostics;

    bool ok() const;
};
```

This keeps intake diagnostics separate from resolution diagnostics. Typical usage is:

```cpp
FactSet facts;

auto defaults_report = load_internal_defaults(defaults, policies);
auto env_report = load_environment(env_policy, policies);
auto cli_report = load_cli(cli_policy, policies, argc, argv);

for (const auto& fact : defaults_report.facts.all()) facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
for (const auto& fact : env_report.facts.all()) facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
for (const auto& fact : cli_report.facts.all()) facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);

auto result = resolve(facts, policies);
```

## InternalDefaultsProvider

`InternalDefaultsProvider` produces internal-default facts. This is useful when an application has a minimal usable built-in configuration.

```cpp
InternalDefaultsProvider defaults;
defaults.set_string(KeyPath("logging.level"), "info")
        .set_int(KeyPath("server.port"), 8080)
        .set_bool(KeyPath("feature.enabled"), false);

auto report = load_internal_defaults(defaults, policies);
```

These defaults become normal facts with `SourceKind::InternalDefault`, so they participate in precedence and explanation.

## Environment loader

Environment intake is policy-controlled.

```cpp
EnvironmentLoaderPolicy env;
env.enabled()
   .prefix("MYAPP_")
   .mapping_style(EnvMappingStyle::PrefixToDottedLowercase)
   .map("MYAPP_LOG_LEVEL", KeyPath("logging.level"), ValueType::String);

auto report = load_environment(env, policies);
```

### Explicit mapping

```cpp
env.map("MYAPP_LOG_LEVEL", KeyPath("logging.level"), ValueType::String);
```

This maps exactly one environment variable to exactly one key.

### Prefix-to-dotted lowercase mapping

With:

```cpp
env.prefix("MYAPP_")
   .mapping_style(EnvMappingStyle::PrefixToDottedLowercase);
```

this input:

```text
MYAPP_SERVER_PORT=9000
```

becomes:

```text
server.port = 9000
source = env:MYAPP_SERVER_PORT
```

The loader asks `PolicySet` for the expected type of `server.port`. If the policy says `ValueType::Int`, the string is parsed as an integer. Without an expected type, it is loaded as a string.

## CLI loader

The CLI loader supports explicit option mappings.

```cpp
CliLoaderPolicy cli;
cli.enabled()
   .option("--log-level", KeyPath("logging.level"), ValueType::String)
   .option("--port", KeyPath("server.port"), ValueType::Int)
   .flag("--feature", KeyPath("feature.enabled"));

auto report = load_cli(cli, policies, argc, argv);
```

Supported forms:

```text
--log-level trace
--log-level=trace
--port 9000
--port=9000
--feature
```

Boolean flags currently mean `true` when present. Negative flag forms such as `--no-feature` are not implemented yet.

## Unknown input policy

Both env and CLI policies support:

```cpp
unknown_inputs(UnknownInputPolicy::Ignore);
unknown_inputs(UnknownInputPolicy::Warn);
unknown_inputs(UnknownInputPolicy::Error);
```

This controls what the loader does with configured-prefix env variables or CLI arguments that are not consumed by a rule.

## Type parsing

Loaders receive text from the outside world. The `parse_value()` helper currently supports:

- string
- bool: `true/false`, `yes/no`, `on/off`, `1/0`
- int64, using base autodetection through `std::stoll(..., base=0)`
- double
- null

Invalid parse produces a loader diagnostic instead of a fact.

## File loader family, added in v0.3

v0.3 adds config-file discovery and a small key=value parser.

```cpp
FileDiscoveryPolicy files;
files.enabled()
     .search_path("./app.conf")
     .require_path("/etc/myapp/app.conf")
     .when_none_found(AbsenceAction::Warn);

auto discovery = discover_config_files(files, policies);
```

The resulting `DiscoveryReport` contains:

- discovered files
- loaded file facts
- file-loader diagnostics

Missing file behavior is governed by `AbsenceAction`, not hardcoded by the loader.

See `docs/FILE_DISCOVERY.md` for details.
