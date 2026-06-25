# Future plugin model note

This is a future-facing design note, not an implemented v0.5 feature.

`configlib` should eventually support pluggable providers and adapters without making the core depend on any single file format, language catalog, secret store, remote source, or presentation system.

The core rule is:

```text
Plugins may produce facts, policies, diagnostics, or rendered presentation data.
Plugins do not take over resolver/store semantics.
```

Possible future plugin classes:

- JSON/TOML/YAML/INI loaders
- custom config format loaders such as `foobar_weirdformat.foobar`
- emitters/exporters
- secret providers
- remote config providers
- schema providers
- validation providers
- i18n/catalog providers using stable message monikers, gettext `*.po`, or other catalog systems

This is a direct continuation of the core mechanism/data/policy separation.

Examples:

```text
custom file format
    -> plugin loader
    -> facts + provenance
    -> policy resolver
```

```text
message key / moniker
    -> locale/catalog provider
    -> resolved presentation string
    -> diagnostic/UI/help output
```

Immediate development remains focused on the core runtime access model, especially `ConfigStore` and `ConfigView`.
