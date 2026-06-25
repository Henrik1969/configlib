# ConfigStore and runtime mutation

v0.4 adds the controlled runtime configuration store layer.

The store is deliberately not a global variable map. It is a runtime access surface over resolved configuration, governed by policy.

```text
resolved config + defaults + policies + access rules
        |
        v
ConfigStore
        |
        v
queries / transactions / reset / export / diagnostics
```

## Design law

```text
Runtime changes are still facts in spirit.
Access is governed by policy.
Mutation is staged before commit.
Export is policy-controlled.
The store is a controlled view, not application-owned global soup.
```

## Main types

### `ConfigStore`

`ConfigStore` owns:

- a base resolved configuration
- known defaults
- runtime overrides
- runtime erasures
- the policy set used for validation
- access/export rules
- diagnostics copied from the initial resolution

Create one from a resolver result:

```cpp
FactSet facts;
PolicySet policies;

// populate facts and policies...

auto result = resolve(facts, policies);
auto store = ConfigStore::from_result(std::move(result), policies);
```

Query values:

```cpp
auto level = store.get_string(KeyPath("logging.level"), "info");
auto port = store.get_int(KeyPath("server.port"), 8080);
```

### `AccessPolicy`

`AccessPolicy` controls what may happen to individual keys at runtime.

```cpp
AccessPolicy access;
access.runtime_mutable(KeyPath("server.port"), false)
      .secret(KeyPath("auth.token"))
      .exportable(KeyPath("debug.internal"), false);
```

Current access flags:

- `runtime_mutable`
- `exportable`
- `secret`
- `resettable`

A secret key is automatically treated as not exportable in normal exports.

### `ConfigTransaction`

Runtime mutation is staged in a transaction.

```cpp
auto tx = store.begin_transaction();
tx.set(KeyPath("logging.level"), Value("trace"));

if (!tx.commit()) {
    std::cerr << tx.diagnostics().format();
}
```

Supported transaction operations:

```cpp
tx.set(KeyPath("x"), Value(1));
tx.erase(KeyPath("x"));
tx.reset_to_base(KeyPath("x"));
tx.reset_to_default(KeyPath("x"));
tx.rollback();
```

`commit()` validates before applying changes. Failed commits leave the store unchanged.

## Validation

Runtime changes are checked against the key policies already used by the resolver:

- expected type
- allowed string set
- integer min/max range
- runtime mutability
- reset permission
- known default for `reset_to_default()`

Example denial:

```text
error CONFIG_STORE_MUTATION_DENIED: key may not be changed at runtime
```

## Reset behavior

`reset_to_base(key)` drops any runtime override or runtime erase for the key, returning it to the resolved startup value.

`reset_to_default(key)` changes the effective runtime value to the known default. Defaults may come from policy defaults or resolved internal-default facts.

## Export

The store can export a simple line-based representation:

```cpp
auto full = store.export_config(ExportMode::Effective);
auto changed = store.export_config(ExportMode::ChangedOnly);
```

Modes:

- `Effective`: export the current effective view
- `ChangedOnly`: export runtime changes only
- `RuntimeChangesOnly`: synonym-style mode for runtime overrides/erasures
- `Redacted`: export effective view while redacting secret values when exported

Current export is intentionally simple `key = value` text. JSON/TOML/YAML emitters should remain separate adapters later.

## Example

```cpp
PolicySet policies;
policies.defaulted(KeyPath("logging.level"), Value("info"));
policies.allowed_strings(KeyPath("logging.level"), {"trace", "debug", "info", "warn", "error"});

AccessPolicy access;
access.runtime_mutable(KeyPath("server.port"), false);

auto result = resolve(facts, policies);
auto store = ConfigStore::from_result(std::move(result), policies, access);

auto tx = store.begin_transaction();
tx.set(KeyPath("logging.level"), Value("trace"));
tx.commit();

std::cout << store.export_config(ExportMode::ChangedOnly);
```

## Current limitation

v0.5 does not yet implement callbacks, live reload, watcher hooks, mutable subtree views, or typed binding adapters. Those belong in later versions.


## Scoped views

v0.5 adds `ConfigView` as a read-only scoped lens into a store.

```cpp
auto logging = store.view(KeyPath("logging"));
auto level = logging.get_string_or(KeyPath("level"), "info");
```

The store remains the mutation and policy enforcement surface. Views provide scoped read/export/explain behavior for subsystems.

See `VIEWS.md`.
