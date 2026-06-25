# Struct bindings

`configlib` v0.6.0 adds explicit typed struct bindings.

Bindings are convenience projections from a `ConfigView` into an ordinary C++ struct. They do not replace the governed store, resolver, provenance model, diagnostics, or runtime policy. The store remains the source of governed truth.

```text
ConfigStore
    -> ConfigView("xyz.logging")
    -> StructBinding<LoggingConfig>
    -> LoggingConfig snapshot
```

## Why bindings exist

Many subsystems want a normal typed configuration object:

```cpp
struct LoggingConfig {
    std::string level;
    bool color;
    std::string file;
};
```

That is convenient and readable. But without care, typed config structs become stale global blobs. `StructBinding<T>` gives the convenience while preserving the configlib model:

- facts and policies still resolve the values
- provenance and explanations still live in the store
- runtime mutation still goes through transactions
- subsystems can bind from scoped views only
- binding diagnostics report missing or wrongly typed fields

## Basic example

```cpp
#include <configlib/configlib.hpp>

struct LoggingConfig {
    std::string level;
    bool color{true};
    std::string file;
};

auto logging_view = store.view(configlib::KeyPath("xyz.logging"));

configlib::StructBinding<LoggingConfig> binding("LoggingConfig");
binding
    .string(configlib::KeyPath("level"), &LoggingConfig::level, "info")
    .boolean(configlib::KeyPath("color"), &LoggingConfig::color, true)
    .string(configlib::KeyPath("file"), &LoggingConfig::file, "stderr");

auto result = binding.read(logging_view);

if (!result.ok()) {
    std::cerr << result.diagnostics().format();
    return 1;
}

LoggingConfig cfg = result.value();
```

The keys are local to the view. With a view prefix of `xyz.logging`, the field key `level` reads the full key `xyz.logging.level`.

## Supported field types

v0.6.0 supports deliberately boring field binders:

```cpp
.string(KeyPath("name"), &T::name, "fallback")
.required_string(KeyPath("name"), &T::name)

.boolean(KeyPath("enabled"), &T::enabled, false)
.required_boolean(KeyPath("enabled"), &T::enabled)

.integer(KeyPath("port"), &T::port, 8080)              // int
.integer(KeyPath("limit"), &T::limit, std::int64_t{0}) // int64_t
.required_integer(...)

.floating(KeyPath("timeout"), &T::timeout, 1.0)
.required_floating(KeyPath("timeout"), &T::timeout)
```

The supported member types are:

```text
std::string
bool
int
std::int64_t
double
```

`double` bindings accept both `Double` and `Int` values. Other bindings require exact matching `ValueType` values.

## Required vs optional fields

Optional fields use the supplied fallback when the key is absent:

```cpp
binding.string(KeyPath("file"), &LoggingConfig::file, "stderr");
```

Required fields produce an error diagnostic if absent:

```cpp
binding.required_string(KeyPath("level"), &LoggingConfig::level);
```

A missing required field produces:

```text
BINDING_REQUIRED_MISSING
```

A type mismatch produces:

```text
BINDING_TYPE_MISMATCH
```

## Design rule

Bindings are snapshots.

They are useful for constructing a subsystem object:

```cpp
Logger logger(binding.read(store.view(KeyPath("xyz.logging"))).value());
```

But they are not the canonical configuration store. If a runtime config value changes, bind again from the store/view or use the store directly.

```text
Bindings are convenience projections.
ConfigStore remains the source of governed truth.
```

## Non-goals for v0.6.0

v0.6.0 intentionally does not add:

- C++ reflection
- macro-based binding DSL
- generated code
- schema-language imports
- mutable scoped views
- automatic live rebinding

Those can come later if justified. The first binding layer is explicit, boring, and easy to inspect.
