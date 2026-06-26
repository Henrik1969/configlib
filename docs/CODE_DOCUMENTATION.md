# Code documentation policy

Public headers are documented for generated code documentation tooling as of `v0.8.1`.

The preferred style is Doxygen-compatible `///` comments on public C++ declarations and `/** ... */` comments in the C ABI header. Header comments should be short, semantic, and useful to a programmer reading the API. Longer explanations belong in `docs/`.

Documentation should cover:

- purpose
- ownership/lifetime
- return semantics
- diagnostics behavior
- fallback behavior
- whether a type owns state, borrows state, reports, or returns a result

The most important repeated rules are:

```text
No silent fallback.
Only *_or functions use explicit fallback values.
Defaults are facts.
Schema validates shape.
Policy governs resolution behavior.
AccessPolicy governs runtime/export behavior.
ConfigView and ConfigTransaction borrow ConfigStore state.
Bindings project governed configuration into application structs.
```

Generated documentation can be built with:

```sh
doxygen Doxyfile
```

or through CMake when Doxygen is installed:

```sh
cmake -S . -B build -DCONFIGLIB_BUILD_DOCS=ON
cmake --build build --target configlib_docs
```

HTML and LaTeX output are both enabled. The LaTeX output is meant as groundwork for later PDF manuals.
