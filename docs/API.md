# configlib API overview

## C++ API

Include:

```cpp
#include <configlib/configlib.hpp>
```

Minimal example:

```cpp
#include <configlib/configlib.hpp>
#include <iostream>

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("logging.level"), Value("info"));
    facts.add_file(KeyPath("logging.level"), Value("warn"), "./app.conf");
    facts.add_env(KeyPath("logging.level"), Value("debug"), "MYAPP_LOGGING_LEVEL");
    facts.add_cli(KeyPath("logging.level"), Value("trace"), "--log-level");

    PolicySet policy;
    policy.require(KeyPath("logging.level"), ValueType::String);
    policy.allowed_strings(KeyPath("logging.level"), {"trace", "debug", "info", "warn", "error"});

    auto result = resolve(facts, policy);
    if (!result.ok()) {
        std::cerr << result.diagnostics().format();
        return 1;
    }

    std::cout << result.config().get_string(KeyPath("logging.level")) << '\n';
    std::cout << result.config().format_explanation(KeyPath("logging.level"));
}
```

## Main C++ types

### `configlib::Value`

Scalar variant value:

```cpp
Value();
Value(bool);
Value(int);
Value(std::int64_t);
Value(double);
Value(const char*);
Value(std::string);
```

### `configlib::KeyPath`

Dotted config key:

```cpp
KeyPath("server.port")
```

### `configlib::FactSet`

Fact collection:

```cpp
facts.add_default(KeyPath("x"), Value(1));
facts.add_file(KeyPath("x"), Value(2), "./app.conf");
facts.add_env(KeyPath("x"), Value(3), "APP_X");
facts.add_cli(KeyPath("x"), Value(4), "--x");
facts.add_runtime(KeyPath("x"), Value(5));
```

### `configlib::PolicySet`

Policy collection:

```cpp
policy.set_precedence(SourceKind::Environment, 50);
policy.require(KeyPath("server.port"), ValueType::Int);
policy.defaulted(KeyPath("server.host"), Value("127.0.0.1"));
policy.allowed_strings(KeyPath("logging.level"), {"debug", "info", "warn"});
policy.int_range(KeyPath("server.port"), 1, 65535);
policy.conflict(KeyPath("feature.x"), ConflictPolicy::RejectConflict);
```

### `configlib::resolve`

```cpp
ResolveResult resolve(const FactSet& facts, const PolicySet& policies);
```

Returns:

- `ResolvedConfig`
- `DiagnosticLog`

## C ABI

Include:

```c
#include <configlib/configlib.h>
```

The C API uses opaque handles and does not expose C++ types over the ABI.

Minimal example:

```c
#include <configlib/configlib.h>
#include <stdio.h>

int main(void) {
    configlib_ctx* ctx = configlib_create();

    configlib_default_string(ctx, "logging.level", "info");
    configlib_add_string(ctx, "logging.level", "trace", CONFIGLIB_SOURCE_CLI, "--log-level");
    configlib_require(ctx, "logging.level", CONFIGLIB_VALUE_STRING);

    configlib_result* result = NULL;
    configlib_resolve(ctx, &result);

    const char* level = NULL;
    if (configlib_result_get_string(result, "logging.level", &level) == CONFIGLIB_OK) {
        printf("%s\n", level);
    }

    configlib_result_destroy(result);
    configlib_destroy(ctx);
}
```

## ABI principles

- no STL types across the C boundary
- no exceptions across the C boundary
- ownership is explicit
- all context/result objects are opaque
- returned strings are owned by the result object and remain valid until the result is destroyed or the next explain call mutates the explanation cache

## v0.2 loader API

Include remains:

```cpp
#include <configlib/configlib.hpp>
```

### Internal defaults provider

```cpp
InternalDefaultsProvider defaults;
defaults.set_string(KeyPath("logging.level"), "info")
        .set_int(KeyPath("server.port"), 8080)
        .set_bool(KeyPath("feature.enabled"), false);

auto defaults_report = load_internal_defaults(defaults, policies);
```

### Environment loader

```cpp
EnvironmentLoaderPolicy env;
env.enabled()
   .prefix("MYAPP_")
   .mapping_style(EnvMappingStyle::PrefixToDottedLowercase)
   .map("MYAPP_LOG_LEVEL", KeyPath("logging.level"), ValueType::String);

auto env_report = load_environment(env, policies);
```

There is also a testable overload that accepts an explicit environment map:

```cpp
std::map<std::string, std::string> fake_env{
    {"MYAPP_LOG_LEVEL", "debug"},
    {"MYAPP_SERVER_PORT", "9000"}
};

auto env_report = load_environment(env, policies, fake_env);
```

### CLI loader

```cpp
CliLoaderPolicy cli;
cli.enabled()
   .option("--log-level", KeyPath("logging.level"), ValueType::String)
   .option("--port", KeyPath("server.port"), ValueType::Int)
   .flag("--feature", KeyPath("feature.enabled"));

auto cli_report = load_cli(cli, policies, argc, argv);
```

Supported option forms:

```text
--option value
--option=value
--flag
```

### Combining loader output

Loaders intentionally return `FactSet` fragments instead of mutating resolver state directly:

```cpp
FactSet facts;
for (const auto& fact : defaults_report.facts.all()) facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
for (const auto& fact : env_report.facts.all()) facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
for (const auto& fact : cli_report.facts.all()) facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);

auto result = resolve(facts, policies);
```

This keeps intake separate from resolution.

### v0.2 C ABI additions

The C ABI remains opaque. v0.2 adds convenience helpers:

```c
configlib_internal_default_string(ctx, "logging.level", "info");
configlib_internal_default_int(ctx, "server.port", 8080);
configlib_internal_default_bool(ctx, "feature.enabled", 0);

configlib_load_env_mapping(ctx, "MYAPP_LOG_LEVEL", "logging.level", CONFIGLIB_VALUE_STRING);

configlib_load_cli_args(ctx, argc, argv, "--log-level", "logging.level", CONFIGLIB_VALUE_STRING);
```

The C ABI helpers are intentionally narrower than the C++ loader policies. They provide simple integration points without exposing C++ policy objects over the ABI.

## v0.3 file discovery API

Include:

```cpp
#include <configlib/configlib.hpp>
```

Minimal file discovery example:

```cpp
using namespace configlib;

PolicySet policies;
policies.require(KeyPath("logging.level"), ValueType::String);
policies.require(KeyPath("server.port"), ValueType::Int);

InternalDefaultsProvider defaults;
defaults.set_string(KeyPath("logging.level"), "info")
        .set_int(KeyPath("server.port"), 8080);

FileDiscoveryPolicy files;
files.enabled()
     .search_path("./app.conf")
     .search_path("/etc/myapp/app.conf")
     .when_none_found(AbsenceAction::UseInternalDefaults)
     .internal_defaults(defaults);

auto discovered = discover_config_files(files, policies);
auto result = resolve(discovered.load.facts, policies);
```

Main v0.3 types:

- `FileDiscoveryPolicy`
- `FileSearchRule`
- `DiscoveredConfigFile`
- `DiscoveryReport`
- `AbsenceAction`
- `ConfigFileFormat`

Functions:

```cpp
DiscoveryReport discover_config_files(const FileDiscoveryPolicy&, const PolicySet&);
LoadReport load_config_file(const std::string& path, const PolicySet&, ConfigFileFormat = ConfigFileFormat::KeyValue);
LoadReport load_config_files(const std::vector<DiscoveredConfigFile>&, const PolicySet&);
Value infer_value(std::string text);
```

## Runtime store API

Include:

```cpp
#include <configlib/configlib.hpp>
```

Create a store from a resolver result:

```cpp
auto result = resolve(facts, policies);
auto store = ConfigStore::from_result(std::move(result), policies);
```

Query values:

```cpp
auto level = store.get_string(KeyPath("logging.level"), "info");
auto port = store.get_int(KeyPath("server.port"), 8080);
```

Govern access:

```cpp
AccessPolicy access;
access.runtime_mutable(KeyPath("server.port"), false)
      .secret(KeyPath("auth.token"));

auto store = ConfigStore::from_result(std::move(result), policies, access);
```

Make staged runtime changes:

```cpp
auto tx = store.begin_transaction();
tx.set(KeyPath("logging.level"), Value("trace"));

if (!tx.commit()) {
    std::cerr << tx.diagnostics().format();
}
```

Reset or erase:

```cpp
auto tx = store.begin_transaction();
tx.reset_to_base(KeyPath("logging.level"));
tx.reset_to_default(KeyPath("logging.level"));
tx.erase(KeyPath("temporary.key"));
tx.commit();
```

Export:

```cpp
std::cout << store.export_config(ExportMode::Effective);
std::cout << store.export_config(ExportMode::ChangedOnly);
```

See `docs/STORE.md` for details.


## v0.5 scoped view API

`ConfigView` is a read-only scoped lens into a `ConfigStore`.

```cpp
auto store = ConfigStore::from_result(std::move(result), policies);
auto logging = store.view(KeyPath("logging"));

std::string level = logging.get_string_or(KeyPath("level"), "info");
bool color = logging.get_boolean_or(KeyPath("color"), true);
```

The local key `level` is qualified to `logging.level`.

### `ConfigStore::view`

```cpp
ConfigView view(KeyPath prefix) const;
```

### `ConfigView` accessors

```cpp
bool contains(const KeyPath& local_key) const;
std::optional<Value> get(const KeyPath& local_key) const;
std::string get_string(const KeyPath& local_key, std::string fallback = {}) const;
std::int64_t get_int(const KeyPath& local_key, std::int64_t fallback = 0) const;
bool get_bool(const KeyPath& local_key, bool fallback = false) const;
double get_double(const KeyPath& local_key, double fallback = 0.0) const;
```

### Fallback helpers

```cpp
std::string get_string_or(const KeyPath& local_key, std::string fallback) const;
std::int64_t get_integer_or(const KeyPath& local_key, std::int64_t fallback) const;
bool get_boolean_or(const KeyPath& local_key, bool fallback) const;
double get_floating_or(const KeyPath& local_key, double fallback) const;
```

Fallback helpers are convenience only. Authoritative defaults still belong in facts/policies.

### View inspection

```cpp
std::vector<std::string> keys() const;
std::string explain(const KeyPath& local_key) const;
```

`keys()` returns local keys under the view prefix. `explain()` reports full keys so provenance remains globally clear.

### View export

```cpp
std::string export_config(ExportMode mode = ExportMode::Effective) const;
std::string export_local_config(ExportMode mode = ExportMode::Effective) const;
```

`export_config()` preserves full paths:

```text
logging.level = debug
logging.color = true
```

`export_local_config()` strips the prefix:

```text
level = debug
color = true
```

Both honor access policy for secrets and exportability.


## Struct bindings

`StructBinding<T>` reads a scoped `ConfigView` into an ordinary C++ struct. It supports explicit field binders for strings, booleans, integers, and doubles, including required fields and fallback values. See `docs/BINDINGS.md`.


## Schema API

Header:

```cpp
#include <configlib/schema.hpp>
```

Main types:

```cpp
configlib::ConfigSchema
configlib::SchemaRule
configlib::SchemaPathBuilder
configlib::SchemaValidationResult
```

Common pattern:

```cpp
configlib::ConfigSchema schema;

schema.path(configlib::KeyPath("server.port"))
    .integer()
    .required()
    .range(static_cast<std::int64_t>(1), static_cast<std::int64_t>(65535));

auto checked = schema.validate(result.config());
```

See `docs/SCHEMA.md` for the full explanation.
