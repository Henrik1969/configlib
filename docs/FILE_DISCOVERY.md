# File discovery and file loading

v0.3 adds the first file-intake layer.

The design remains the same:

```text
files -> file facts -> policies -> resolver -> resolved config
```

The file loader does not decide application semantics. It only discovers files, parses simple facts, records provenance, and reports diagnostics.

## FileDiscoveryPolicy

```cpp
InternalDefaultsProvider fallback;
fallback.set_string(KeyPath("logging.level"), "info")
        .set_int(KeyPath("server.port"), 8080);

FileDiscoveryPolicy files;
files.enabled()
     .search_path("./app.conf")
     .search_path("$HOME/.config/myapp/app.conf")
     .require_path("/etc/myapp/app.conf")
     .when_none_found(AbsenceAction::UseInternalDefaults)
     .internal_defaults(fallback);
```

Search rules are checked in order. Existing regular files are returned as discovered config files. Missing required paths produce diagnostics.

## AbsenceAction

When no files are discovered, policy decides what happens:

```text
Ignore              produce no facts and no complaint
Warn                produce warning diagnostic
Error               produce error diagnostic
UseInternalDefaults produce facts from the attached InternalDefaultsProvider
```

This lets an application express whether configuration files are optional, mandatory, or replaceable by built-in minimal defaults.

## Key=value format

v0.3 intentionally ships only a small key=value loader. It is enough to prove the mechanism without dragging in TOML/YAML/JSON dependencies too early.

Supported syntax:

```ini
# comments start with # or ;
logging.level = debug
server.port = 8080
feature.enabled = true

[logging]
level = warn

[server]
port = 9090
```

A section prefixes keys that do not already contain a dot:

```ini
[server]
port = 9090
```

becomes:

```text
server.port = 9090
```

A key already containing dots is preserved:

```ini
[server]
feature.enabled = true
```

becomes:

```text
server.feature.enabled = true
```

## Type handling

If a key has a policy with an expected type, the file loader parses using that type.

If no policy exists for a key, v0.3 infers a scalar value:

```text
true/false/yes/no/on/off -> bool
integers                 -> int64
decimal-looking values   -> double
anything else            -> string
```

This is deliberately conservative and should not be mistaken for schema validation. The resolver still validates final values against policies.

## Provenance

File facts use source names with line information:

```text
file:/path/to/app.conf:12
```

The resolved config explanation can therefore show which file line supplied the winning value and which lower-precedence values were overridden.

## Future file formats

TOML, JSON, YAML, and XDG-style discovery are intentionally not part of v0.3. They should be adapters that produce facts, not new semantic centers.
