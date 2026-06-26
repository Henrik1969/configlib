# Documentation generation

`configlib` prepares its public headers for Doxygen-compatible generated API documentation as of `v0.8.1`.

The project ships a root `Doxyfile` with both HTML and LaTeX generation enabled:

```sh
doxygen Doxyfile
```

Generated output is written below:

```text
build/docs/html
build/docs/latex
```

The LaTeX output is intentionally enabled now. Later releases can use it as the foundation for polished PDF manuals.

## CMake helper target

If Doxygen is installed, a documentation target can be added by configuring with:

```sh
cmake -S . -B build -G Ninja -DCONFIGLIB_BUILD_DOCS=ON
cmake --build build --target configlib_docs
```

If Doxygen is not installed, normal library builds are unaffected.

## Documentation style

Public headers use Doxygen-compatible `///` comments. The comments should explain:

- purpose
- ownership and lifetime
- fallback behavior
- diagnostics behavior
- whether a type owns state, borrows state, reports, or returns a result

The documentation should preserve the core v0.8 rules:

```text
No silent fallback.
No invalid handles.
Defaults are facts.
Schema validates shape.
Policy governs resolution behavior.
AccessPolicy governs runtime/export behavior.
Bindings project into structs.
```
