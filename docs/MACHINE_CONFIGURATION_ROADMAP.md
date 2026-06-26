# Machine configuration roadmap

This document records a long-term direction for work that may grow around `configlib` after the first stable library release.

It is not a v1 requirement. It is a railroad marker: a visible track showing why the small core library is being designed with facts, policy, schema, provenance, diagnostics, stable C ABI, and explicit runtime access.

## The problem: the configuration swamp

A real machine contains many unrelated configuration systems:

- bootloaders
- kernel command lines
- initramfs generators
- init systems
- services
- network managers
- display systems
- audio systems
- shells
- desktop environments
- development tools
- databases
- daemons
- applications

Each system has its own files, syntax, defaults, include rules, precedence rules, reload rules, validation tools, version differences, distro conventions, and folklore.

The goal is not to force all of them into one universal configuration format. That would merely create another swamp creature.

The goal is:

```text
Many native configuration systems.
One governed way to discover, explain, validate, change, and roll them back.
```

## Native formats stay native

`configlib` and future tools should not require projects to abandon their own formats.

X11 can remain X11.

systemd can remain systemd.

OpenSSH can remain OpenSSH.

PipeWire can remain PipeWire.

GRUB, systemd-boot, kernel command lines, service drop-ins, environment files, and application configs can remain what they are.

The shared layer is not a new universal file syntax. The shared layer is the machine-readable knowledge around those native configuration surfaces.

## Configuration law packages

A later ecosystem may allow projects, distributions, vendors, sites, or users to ship machine-readable configuration law packages.

Example locations:

```text
/usr/share/configlaw/
/usr/local/share/configlaw/
/opt/*/share/configlaw/
~/.local/share/configlaw/
./configlaw/
```

Example packages:

```text
configlaw-grub
configlaw-systemd-boot
configlaw-kernel
configlaw-initramfs
configlaw-systemd
configlaw-networkmanager
configlaw-pipewire
configlaw-x11
configlaw-wayland
configlaw-openssh
configlaw-nginx
configlaw-postgresql
configlaw-bash
configlaw-user-session
```

A law package describes the target configuration surface. It should not secretly operate the target.

Possible contents:

```text
target.toml
schemas/
policies/
locations/
templates/
probes/
validators/
migrations/
risks/
rollback/
docs/
examples/
```

## Kinds of law

Configuration law can be layered:

```text
Upstream law
    What the original project says is valid.

Distro law
    Where this distribution places files and how services are managed.

Site law
    Local organization or machine policy.

User law
    Personal preferences and local overrides.

Scenario law
    Temporary test, deployment, or recovery rules.
```

The same governance model applies:

```text
upstream < distro < site < user < explicit session intent
```

## What law packages should describe

A useful law package may describe:

- valid keys, directives, sections, values, and types
- default facts
- source precedence
- file locations and include order
- environment variables
- CLI mappings
- version-specific rules
- deprecated or renamed settings
- distro-specific differences
- dangerous settings
- secret/redaction behavior
- reload/restart requirements
- runtime mutability
- validation commands
- dry-run support
- probes for installed version and active state
- templates for common setups
- migration rules
- backup requirements
- rollback procedures
- recovery entry points

This turns configuration from folklore into governed knowledge.

## Lifecycle scope

The long-term direction can cover the whole local machine lifecycle:

```text
firmware / bootloader
    -> kernel command line
    -> initramfs
    -> init / systemd / other init
    -> system services
    -> user session
    -> desktop, audio, network, dev tools, apps
    -> shutdown / reboot / recovery
```

This matters because some settings only take effect at certain stages.

Examples:

- a kernel command-line option may require a reboot
- an initramfs change may require regeneration before reboot
- a display-manager change may break graphical login
- an SSH change may lock out remote administration
- a service drop-in may require daemon reload plus service restart
- an audio change may be testable in the current session
- an application runtime setting may be changed transactionally

Future tools should model not only what setting exists, but when it exists and how it becomes active.

## Tool ecosystem

Possible post-v1 tools:

```text
configlint
    Validate, mock, inspect, and explain configuration setups.

configforge
    Generate candidate configuration from intent, machine facts, law packages, and documentation.

configdoctor
    Diagnose broken or suspicious configuration and propose repairs.

configdiff
    Compare two resolved configurations or two machine states.

configapply
    Dry-run, back up, apply, verify, and roll back configuration changes.

configctl
    Unified operator-facing command frontend.

configd
    Optional local daemon for inventory, policy, audit, state, snapshots, and safe operations.
```

The first practical tool after v1 should probably be `configlint`, because it directly proves the value of the stable library core without requiring a whole machine-management stack.

## Safe operation phases

Tools that change machine configuration should be phase-based:

```text
inspect
understand
plan
generate
explain
lint
diff
dry-run
backup
apply
verify
rollback
```

A tool should not blindly edit real system files. Risky changes should have a rollback story before they are applied.

## Rollback and recovery

For application configuration, rollback may only mean restoring a previous file or transaction state.

For machine configuration, rollback may require several layers:

- file backup
- generated manifest
- service state
- package/version information
- bootloader fallback entry
- initramfs fallback
- filesystem snapshot
- last-known-good configuration
- recovery-shell instructions

The law package should describe what level of rollback is required for a target and risk class.

## Relationship to AI and agents

This roadmap is also a trust boundary for AI-assisted administration.

An AI agent should not guess which file to edit, which command to run, or whether a setting is still valid.

It should ask installed law packages and governed tools:

```text
What target owns this setting?
What version is installed?
What files are valid?
What validator exists?
What is the risk?
What rollback is required?
What command applies safely?
```

The agent becomes an operator inside a governed system, not a wizard randomly editing files with elevated privileges.

## Relationship to a future distro or operating environment

This direction gives a future distro or derived operating environment its first real teeth.

The distro does not need to replace Linux, shells, compilers, init systems, service managers, desktop components, or application formats.

It can add a governed configuration layer around them.

That layer can make the machine self-describing:

```text
What configurable targets exist here?
What laws are installed for them?
What files do they own?
What values are active?
What differs from defaults?
What is dangerous?
What can be changed safely?
What needs root?
What needs reboot?
What has rollback support?
```

This is the path out of the configuration swamp.

## Boundary with configlib v1

This roadmap does not belong inside the v1 core feature set.

`configlib` v1 should remain a small, stable, reusable library for governed application configuration.

The post-v1 tooling ecosystem can grow around that stable island:

```text
configlib v1
    stable core facts / policy / schema / resolver / store / view / C ABI

post-v1 tools
    configlint, configforge, configdoctor, configapply, configctl

longer-term system layer
    machine configuration law packages and lifecycle governance
```

The core must stay clean enough that these larger tools can trust it.
