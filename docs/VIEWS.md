# Config views

`ConfigView` is the v0.5 scoped-access layer over `ConfigStore`.

The design sentence is:

```text
ConfigStore is the governed runtime vault.
ConfigView is a scoped keyhole into one subtree of that vault.
```

A view is not a copy of configuration. It is a read-only lens into a store. It maps local keys to full keys by prefixing them with the view prefix.

Example:

```cpp
auto logging = store.view(KeyPath("logging"));

logging.get_string(KeyPath("level"));
```

Internally this reads:

```text
logging.level
```

## Why views exist

Large applications should not hand every subsystem the whole configuration universe.

Instead of this:

```cpp
Database db(store);
Logger logger(store);
Parser parser(store);
```

prefer this:

```cpp
Database db(store.view(KeyPath("database")));
Logger logger(store.view(KeyPath("logging")));
Parser parser(store.view(KeyPath("flowmini.parser")));
```

The subsystem receives only the configuration surface it owns.

## API

```cpp
ConfigView view = store.view(KeyPath("logging"));
```

Read helpers:

```cpp
view.contains(KeyPath("level"));
view.get(KeyPath("level"));
view.get_string(KeyPath("level"));
view.get_int(KeyPath("port"));
view.get_bool(KeyPath("color"));
view.get_double(KeyPath("scale"));
```

Fallback helpers:

```cpp
view.get_string_or(KeyPath("level"), "info");
view.get_int_or(KeyPath("max_files"), 32);
view.get_bool_or(KeyPath("color"), true);
view.get_double_or(KeyPath("scale"), 1.0);
```

Fallback getters are convenience helpers only. Authoritative defaults should still be expressed as facts and policies.

## Key listing

```cpp
for (const auto& key : view.keys()) {
    std::cout << key << '\n';
}
```

For a `logging` view, keys are local:

```text
color
file
level
```

not:

```text
logging.color
logging.file
logging.level
```

## Explain

```cpp
std::cout << logging.explain(KeyPath("level"));
```

The explanation still uses full paths so diagnostics and provenance remain globally unambiguous:

```text
logging.level = debug
chosen: cli:--log-level
```

## Export

`export_config()` preserves full paths:

```cpp
std::cout << logging.export_config();
```

Example output:

```text
logging.color = true
logging.level = debug
```

`export_local_config()` strips the view prefix:

```cpp
std::cout << logging.export_local_config();
```

Example output:

```text
color = true
level = debug
```

Both export functions honor the store access policy for secret and non-exportable keys.

## Export modes

Views support the same `ExportMode` values as `ConfigStore`:

- `ExportMode::Effective`
- `ExportMode::ChangedOnly`
- `ExportMode::RuntimeChangesOnly`
- `ExportMode::Redacted`

Changed-only export is scoped to the view subtree.

## Mutation

v0.5 views are read-only.

Runtime mutation still happens through `ConfigStore::begin_transaction()`.

This is deliberate. View mutation is useful, but it raises additional policy questions around scoped write permission, subtree ownership, and capability boundaries. Those belong in a later version after read-only views are stable.
