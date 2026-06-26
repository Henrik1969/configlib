# configlib explained simply

This document explains `configlib` without assuming that the reader already knows the project vocabulary.

For exact definitions, see [`TERMINOLOGY.md`](TERMINOLOGY.md). This document is intentionally more conversational.

## One paragraph

`configlib` helps an application decide what its configuration really is when values may come from several places: internal defaults, config files, environment variables, command-line arguments, and runtime changes. It keeps track of where values came from, which rules made one value win over another, whether the final values are valid, which parts may be changed or exported, and how to explain the final result.

In short:

> `configlib` is not only a config parser. It is a governed configuration resolver and runtime access layer.

## The courtroom metaphor

A useful way to understand `configlib` is to imagine a small court case.

Several sources make claims about configuration values. These claims are **facts**.

The **policy** is the rulebook. It says which sources are allowed, which sources outrank other sources, how conflicts are handled, and how the configuration may behave at runtime.

The **schema** is the validity or admissibility test. It says which keys are required, which types are valid, and which values are acceptable.

The **resolver** is the judge. It hears the competing facts, applies the policy, and produces a final result.

The **resolved configuration** is the ruling.

The **diagnostics** are the court record. They explain what happened, what failed, and why.

The **ConfigStore** is the runtime vault that holds the ruling safely while the program runs.

The **ConfigView** is a scoped keyhole into the vault. It lets one subsystem see only its own part of the configuration.

The **StructBinding** is a typed projection. It copies a governed configuration view into a normal application struct when that is convenient.

## Metaphor map

| Technical term | Plain-language metaphor | What it means in configlib |
|---|---|---|
| Fact | Claim/witness statement | A raw key/value claim from a source |
| FactSet | Witness list | The collection of raw configuration claims |
| Source | Witness identity | Where a fact came from: default, file, env, CLI, runtime |
| Provenance | Origin/history | The record of where a final value came from |
| Policy | Rulebook | The rules for resolution and runtime behavior |
| Schema | Validity/admissibility test | The rules for valid keys, types, ranges, and allowed values |
| Resolver | Judge/arbiter | The mechanism that applies policy to facts |
| ResolvedConfig | Ruling | The final selected configuration values |
| Diagnostics | Court record | Explanation, warnings, and errors |
| ConfigStore | Runtime vault | Governed runtime owner of resolved configuration |
| ConfigTransaction | Staged change request | A temporary set of runtime changes before commit |
| ConfigView | Scoped keyhole | A read-only subtree view into the store |
| StructBinding | Typed projection | A convenient struct snapshot from a view |

## Concrete example

This example is deliberately small. It shows several sources claiming different values for the same key.

```cpp
#include <configlib/configlib.hpp>

#include <iostream>

int main() {
    using namespace configlib;

    // The FactSet is where the witnesses make their claims.
    // Each source may claim a value for the same key.
    FactSet facts;

    // Witness 1: internal defaults.
    // "If nobody says otherwise, logging.level is info."
    facts.add_default(KeyPath("logging.level"), Value("info"));

    // Witness 2: config file.
    // "The config file says logging.level is warn."
    facts.add_file(KeyPath("logging.level"), Value("warn"), "examples/example.conf");

    // Witness 3: environment.
    // "The environment says logging.level is debug."
    facts.add_env(KeyPath("logging.level"), Value("debug"), "APP_LOGGING_LEVEL");

    // Witness 4: command line.
    // "The command line says logging.level is trace."
    facts.add_cli(KeyPath("logging.level"), Value("trace"), "--log-level");

    // The PolicySet is the rulebook.
    // It says this key is required and must be a string.
    // Source precedence is already part of the built-in source defaults:
    // CLI beats env, env beats files, files beat internal defaults.
    PolicySet policy;
    policy.require(KeyPath("logging.level"), ValueType::String);

    // The ConfigSchema is the validity test.
    // It says which string values are legal for this key.
    ConfigSchema schema;
    schema.path(KeyPath("logging.level"))
        .string()
        .required()
        .allowed({"trace", "debug", "info", "warn", "error"})
        .documented_default(Value("info"));

    // The resolver is the judge.
    // It hears all claims, applies the rulebook, and produces a ruling.
    auto result = resolve(facts, policy);

    // The schema checks that the ruling is legally valid.
    auto checked = schema.validate(result.config());

    if (!result.ok() || !checked.ok()) {
        // Diagnostics are the court record.
        // They explain what went wrong and where.
        std::cerr << result.diagnostics().format();
        std::cerr << checked.diagnostics().format();
        return 1;
    }

    // The ResolvedConfig is the ruling.
    // In this case, CLI wins, so logging.level is "trace".
    const auto& config = result.config();
    std::cout << "logging.level = "
              << config.get_string_or(KeyPath("logging.level"), "info")
              << '\n';

    // The explanation shows the court record for one key:
    // which source won and which other claims were overruled.
    std::cout << "\nExplanation:\n"
              << config.format_explanation(KeyPath("logging.level"));

    // The ConfigStore is the governed runtime vault.
    // It lets the program use the ruling safely while it runs.
    auto store = ConfigStore::from_result(std::move(result), policy);

    // The ConfigView is a scoped keyhole into the vault.
    // This subsystem only sees logging.* as local keys.
    auto logging = store.view(KeyPath("logging"));

    std::cout << "\nlogging.level through view = "
              << logging.get_string_or(KeyPath("level"), "info")
              << '\n';

    return 0;
}
```

## The same flow without the metaphor

In technical terms, the example does this:

1. Create a `FactSet` containing competing facts from defaults, file, environment, and CLI.
2. Create a `PolicySet` describing resolution requirements.
3. Create a `ConfigSchema` describing valid values.
4. Call `resolve(facts, policy)` to produce a `ResolveResult`.
5. Validate the resolved config with `schema.validate(...)`.
6. Read values from `ResolvedConfig` or put the result into a `ConfigStore`.
7. Give subsystems scoped access through `ConfigView`.

## Why not just parse a config file?

A parser answers:

```text
What values are written in this file?
```

`configlib` answers a larger question:

```text
Given defaults, files, environment variables, command-line arguments,
runtime changes, schema, policy, access rules, and diagnostics:
what is the final governed configuration, and why?
```

For tiny tools, that is too much machinery. A plain `argv` loop, `strtol()`, or a normal CLI parser is better.

For larger applications, `configlib` prevents configuration from becoming a pile of global variables, mystery overrides, and hand-written special cases.

## Where the metaphor stops

The courtroom metaphor is only a teaching tool.

The resolver is not intelligent, subjective, or magical. It does not guess what the user meant. It mechanically applies explicit policy to explicit facts and reports diagnostics when something is missing, invalid, or conflicting.

That mechanical behavior is the point: if the rules are clear, the result can be explained.
