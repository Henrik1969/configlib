# Code documentation policy

Public headers should be documented for generated code documentation tooling before `1.0.0`.

Preferred style is Doxygen-compatible `///` comments on public classes, functions, enums, and ownership-sensitive methods.

Documentation should cover:

- Purpose
- Ownership/lifetime
- Return semantics
- Diagnostics behavior
- Whether fallback is explicit
- Whether a type is a view, owner, result, or report

The detailed public-header documentation pass is planned for `v0.8.1`.
