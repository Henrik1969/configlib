# When to Use configlib

`configlib` is a governed runtime configuration library. It is not meant to replace every small argument parser, every `argv` loop, or every tiny settings struct.

The project rule is simple:

```text
Use the right tool for the right task.
```

## Use ordinary argument parsing for tiny tools

For a small command-line program where the entire configuration surface is just `argc` and `argv`, plain C/C++ argument handling is usually the correct choice.

Example:

```c
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int result = 0;

    for (int i = 1; i < argc; ++i) {
        result += atoi(argv[i]);
    }

    return result;
}
```

For a program like that, using `configlib` would be needless machinery. A simple `argv` loop, `strtol()`, or a small command-line parser is enough.

## Use configlib when configuration becomes governance

`configlib` becomes useful when configuration is no longer only command invocation, but a governed runtime concern.

Typical signs that `configlib` is appropriate:

```text
internal defaults
configuration files
environment variables
CLI flags
runtime overrides
validation
source precedence
provenance/explainability
scoped subsystem views
runtime mutation policy
export/redaction policy
diagnostics
```

In other words:

```text
argv parsing is for command invocation.
configlib is for governed application configuration.
```

## Good fit

`configlib` is a good fit for:

```text
large CLI tools
daemons/services
GUI/TUI applications
compiler/toolchain frontends
applications with subsystems
applications needing runtime configuration changes
applications needing explainable precedence
applications needing safe export/redaction
cross-language projects using a C/C++ core with bindings
```

## Bad fit

`configlib` is probably the wrong tool for:

```text
tiny one-shot command-line programs
programs with only one or two simple arguments
throwaway scripts
cases where ordinary argv parsing is clearer
cases where no provenance, policy, runtime mutation, or scoped access is needed
```

## Design boundary

`configlib` does not aim to abolish old-school argument parsing. Old-school parsing is still the right tool for small programs.

The library exists for the point where hand-rolled configuration logic starts turning into accidental infrastructure:

```text
--config
--profile
--log-level
XYZ_LOG_LEVEL
user config
system config
runtime admin changes
why did this value win?
which subsystem may see this?
may this key change at runtime?
may this value be exported?
```

At that point, `configlib` provides the deliberate mechanism:

```text
facts + policies + provenance
        |
        v
resolver
        |
        v
ConfigStore
        |
        v
scoped views / transactions / export / diagnostics
```

## Short version

```text
Use `strtol()` for a number.
Use an argv loop for tiny tools.
Use a normal CLI parser for normal command invocation.
Use configlib when configuration becomes a runtime governance problem.
```
