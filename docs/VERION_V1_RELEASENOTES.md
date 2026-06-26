configlib v1.0.0

First stable release of configlib: a governed configuration core for C++ and C ABI consumers.

Highlights:
- Governed facts, policies, schema validation, resolving, diagnostics, runtime store, views, and struct bindings.
- Stable C++ API surface for core configuration workflows.
- Stable C ABI surface for external language bindings and FFI consumers.
- Loaders for internal defaults, environment, CLI mapping, and simple config files.
- Access policy, redaction/export behavior, transactions, scoped views, and typed projection.
- CMake package and pkg-config install support.
- Documentation covering architecture, terminology, C ABI, packaging, fire testing, and how-to usage.

Validated with:
- normal build/test
- ASAN/UBSAN
- Valgrind
- install tree test
- external CMake C++ consumer
- pkg-config C consumer
- Python ctypes smoke
- LuaJIT FFI smoke
- Rust FFI smoke
- ABI dump / abidw
- coverage generation

Known non-blocking issue:
- Doxygen HTML currently writes to source-relative build/docs/html due to Doxyfi
