# C ABI

`configlib.h` is the stable binding surface for C and for non-C++ language bindings.

The C ABI deliberately uses opaque handles, scalar values, explicit status codes, and borrowed strings. It does not expose C++ exceptions, STL containers, templates, references, or ownership rules that are hard for other languages to model.

## Design rules

- Every public C handle is opaque.
- Every handle returned by a successful create/open function has a matching destroy function.
- Destroy functions accept `NULL`.
- Functions return `configlib_status` where failure must be visible.
- String results are borrowed and remain valid until the owning handle is destroyed or the same handle is queried again.
- No C++ exception crosses the C ABI.
- C++ templates such as `StructBinding<T>` are not represented directly in C.

## Main handles

```text
configlib_ctx
    Facts and resolution policy before resolve.

configlib_result
    ResolvedConfig plus diagnostics from resolve.

configlib_schema
    Shape validation rules.

configlib_schema_result
    Result of schema validation.

configlib_access_policy
    Runtime mutation/export/secret/reset behavior.

configlib_store
    Governed runtime configuration store.

configlib_view
    Borrowed scoped read-only view into a store.

configlib_transaction
    Staged runtime mutation against a store.
```

## Normal C workflow

```c
configlib_ctx* ctx = configlib_create();

configlib_internal_default_string(ctx, "logging.level", "info");
configlib_add_string(ctx, "logging.level", "debug", CONFIGLIB_SOURCE_CLI, "--log-level");
configlib_require(ctx, "logging.level", CONFIGLIB_VALUE_STRING);

configlib_result* result = NULL;
configlib_resolve(ctx, &result);

if (!configlib_result_ok(result)) {
    puts(configlib_result_diagnostics(result));
}
```

## Schema validation

```c
configlib_schema* schema = configlib_schema_create();
configlib_schema_require(schema, "logging.level", CONFIGLIB_VALUE_STRING);
configlib_schema_allowed_string(schema, "logging.level", "trace");
configlib_schema_allowed_string(schema, "logging.level", "debug");
configlib_schema_allowed_string(schema, "logging.level", "info");
configlib_schema_allowed_string(schema, "logging.level", "warn");
configlib_schema_allowed_string(schema, "logging.level", "error");

configlib_schema_result* checked = NULL;
configlib_schema_validate_result(schema, result, &checked);
```

## Store, transaction, and view

```c
configlib_access_policy* access = configlib_access_policy_create();
configlib_access_secret(access, "secret.token", 1);

configlib_store* store = NULL;
configlib_store_from_result(ctx, result, access, &store);

configlib_transaction* tx = NULL;
configlib_store_begin_transaction(store, &tx);
configlib_transaction_set_string(tx, "logging.level", "warn");
configlib_transaction_commit(tx);

configlib_view* logging = NULL;
configlib_store_view(store, "logging", &logging);

const char* level = NULL;
configlib_view_get_string(logging, "level", &level);
```

## Binding-language notes

For Python, Rust, Zig, LuaJIT, and similar languages, bind to `configlib.h` and treat all handles as opaque pointers.

Important binding rules:

- Never free borrowed strings.
- Copy borrowed strings into the host language if they need to live beyond the next query on the same handle.
- Always call the matching destroy function for owned handles.
- Do not allow a `configlib_view` or `configlib_transaction` to outlive its `configlib_store`.
- Prefer wrapper objects with destructors/finalizers around C handles.

## v0.9 scope

v0.9.0 brings the C ABI much closer to the non-template C++ feature set:

- version/status helpers
- facts and source precedence
- string/int/bool/double facts
- internal defaults and simple file/env/CLI loading
- resolver result getters and explanations
- schema construction and validation
- access policy construction
- store creation and access
- runtime transactions
- scoped views
- export and diagnostics

C++ template projections such as `StructBinding<T>` remain C++-only by design.

## Key validation

The C ABI exposes `configlib_key_is_valid(const char* key)` so binding layers and tools can reject malformed dotted keys before attempting mutation or resolution. Empty keys are valid for root-style contexts; dotted keys with empty path segments such as `.a`, `a.`, or `a..b` are invalid.
