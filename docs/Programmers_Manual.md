# configlib Programmer's Manual

This manual is a practical guide for programmers who want to use `configlib` in real applications.
It complements the API reference by showing the intended workflow and by sketching how the same library can be reached from several modern languages.

`configlib` is not a command-line parser and it is not merely a config-file parser. It is a policy-driven configuration mechanism:

```text
facts + policies + provenance
        |
        v
resolver
        |
        v
ResolvedConfig
        |
        v
ConfigStore
        |
        v
ConfigView / transaction / export / explain
```

The short design law is:

```text
All inputs become facts.
All behavior is governed by policy.
The mechanism resolves, stores, exposes, explains, and exports.
Application meaning stays in the application.
```

## When to use it

Use `configlib` when your program needs several of these at the same time:

- internal defaults
- config files
- environment variables
- CLI options
- runtime overrides
- validation
- source precedence
- provenance/explanation
- redacted/exportable configuration reports
- subsystem-specific scoped views
- controlled runtime mutation

Do not use it just to add two numbers from `argv`. For tiny tools, ordinary argument parsing is still the right tool.

## Core concepts

### Fact

A fact is a claim about a key:

```text
logging.level = "debug"
source = env:MYAPP_LOG_LEVEL
precedence = 50
```

Facts may come from defaults, files, environment, CLI, runtime overrides, or later plugins.

### Policy

Policies govern facts:

```text
logging.level must be a string
logging.level must be one of trace/debug/info/warn/error
CLI wins over environment
server.port may not be changed at runtime
secrets may not be exported in plain text
```

### Resolver

The resolver applies policy to facts and produces a resolved configuration plus diagnostics.

### ConfigStore

A `ConfigStore` is the governed runtime access surface. It lets code query values, stage runtime changes, commit or rollback, explain keys, and export configuration.

### ConfigView

A `ConfigView` is a read-only scoped keyhole into part of a store:

```cpp
auto logging = store.view(configlib::KeyPath("logging"));
logging.get_string_or(configlib::KeyPath("level"), "info");
```

The view local key `level` maps to the full key `logging.level`.

## C++ quick start

Include the umbrella header:

```cpp
#include <configlib/configlib.hpp>
```

Minimal application setup:

```cpp
#include <configlib/configlib.hpp>
#include <iostream>

int main(int argc, char** argv) {
    using namespace configlib;

    PolicySet policies;
    policies.set_precedence(SourceKind::InternalDefault, 10);
    policies.set_precedence(SourceKind::File, 30);
    policies.set_precedence(SourceKind::Environment, 50);
    policies.set_precedence(SourceKind::CLI, 70);
    policies.set_precedence(SourceKind::Runtime, 90);

    policies.require(KeyPath("xyz.logging.level"), ValueType::String);
    policies.allowed_strings(KeyPath("xyz.logging.level"), {
        "trace", "debug", "info", "warn", "error"
    });
    policies.require(KeyPath("xyz.server.port"), ValueType::Int);
    policies.int_range(KeyPath("xyz.server.port"), 1, 65535);

    InternalDefaultsProvider defaults;
    defaults.set_string(KeyPath("xyz.logging.level"), "info")
            .set_bool(KeyPath("xyz.logging.color"), true)
            .set_int(KeyPath("xyz.server.port"), 8080);

    FactSet facts;
    auto default_report = load_internal_defaults(defaults, policies);
    for (const auto& fact : default_report.facts.all()) {
        facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
    }

    EnvironmentLoaderPolicy env;
    env.enabled()
       .map("XYZ_LOG_LEVEL", KeyPath("xyz.logging.level"), ValueType::String)
       .map("XYZ_PORT", KeyPath("xyz.server.port"), ValueType::Int);

    auto env_report = load_environment(env, policies);
    for (const auto& fact : env_report.facts.all()) {
        facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
    }

    CliLoaderPolicy cli;
    cli.enabled()
       .option("--log-level", KeyPath("xyz.logging.level"), ValueType::String)
       .option("--port", KeyPath("xyz.server.port"), ValueType::Int)
       .flag("--no-color", KeyPath("xyz.logging.color"));

    auto cli_report = load_cli(cli, policies, argc, argv);
    for (const auto& fact : cli_report.facts.all()) {
        facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
    }

    auto resolved = resolve(facts, policies);
    if (!resolved.ok()) {
        std::cerr << resolved.diagnostics().format();
        return 1;
    }

    AccessPolicy access;
    access.runtime_mutable(KeyPath("xyz.server.port"), false)
          .runtime_mutable(KeyPath("xyz.logging.level"), true)
          .exportable(KeyPath("xyz.logging.level"), true);

    auto store = ConfigStore::from_result(std::move(resolved), policies, access);
    auto logging = store.view(KeyPath("xyz.logging"));
    auto server = store.view(KeyPath("xyz.server"));

    std::cout << "log level = " << logging.get_string_or(KeyPath("level"), "info") << '\n';
    std::cout << "port      = " << server.get_integer_or(KeyPath("port"), 8080) << '\n';

    std::cout << logging.explain(KeyPath("level"));
}
```

### Runtime mutation in C++

```cpp
auto tx = store.begin_transaction();
tx.set(configlib::KeyPath("xyz.logging.level"), configlib::Value("trace"));

if (!tx.commit()) {
    std::cerr << tx.diagnostics().format();
}
```

If a key is not runtime mutable, the transaction is rejected with diagnostics.

### Export in C++

```cpp
std::cout << store.export_config(configlib::ExportMode::Effective);
std::cout << store.export_config(configlib::ExportMode::ChangedOnly);

const auto logging = store.view(configlib::KeyPath("xyz.logging"));
std::cout << logging.export_config();       // full paths
std::cout << logging.export_local_config(); // prefix stripped
```

## C ABI quick start

The C ABI uses opaque handles. It does not expose C++ STL types or exceptions across the boundary.

```c
#include <configlib/configlib.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, char** argv) {
    configlib_ctx* ctx = configlib_create();
    if (!ctx) return 1;

    configlib_default_string(ctx, "xyz.logging.level", "info");
    configlib_default_int(ctx, "xyz.server.port", 8080);

    configlib_set_source_precedence(ctx, CONFIGLIB_SOURCE_INTERNAL_DEFAULT, 10);
    configlib_set_source_precedence(ctx, CONFIGLIB_SOURCE_ENVIRONMENT, 50);
    configlib_set_source_precedence(ctx, CONFIGLIB_SOURCE_CLI, 70);

    configlib_require(ctx, "xyz.logging.level", CONFIGLIB_VALUE_STRING);
    configlib_require(ctx, "xyz.server.port", CONFIGLIB_VALUE_INT);

    configlib_load_env_mapping(ctx, "XYZ_LOG_LEVEL", "xyz.logging.level", CONFIGLIB_VALUE_STRING);
    configlib_load_env_mapping(ctx, "XYZ_PORT", "xyz.server.port", CONFIGLIB_VALUE_INT);

    configlib_load_cli_args(ctx, argc, (const char* const*)argv,
                            "--log-level", "xyz.logging.level", CONFIGLIB_VALUE_STRING);
    configlib_load_cli_args(ctx, argc, (const char* const*)argv,
                            "--port", "xyz.server.port", CONFIGLIB_VALUE_INT);

    configlib_result* result = NULL;
    if (configlib_resolve(ctx, &result) != CONFIGLIB_OK || !configlib_result_ok(result)) {
        fprintf(stderr, "%s\n", configlib_result_diagnostics(result));
        configlib_result_destroy(result);
        configlib_destroy(ctx);
        return 1;
    }

    const char* level = NULL;
    int64_t port = 0;

    configlib_result_get_string(result, "xyz.logging.level", &level);
    configlib_result_get_int(result, "xyz.server.port", &port);

    printf("level=%s\n", level);
    printf("port=%lld\n", (long long)port);
    printf("%s\n", configlib_result_explain(result, "xyz.logging.level"));

    configlib_result_destroy(result);
    configlib_destroy(ctx);
    return 0;
}
```

For plain C programs, keep the rule of scale in mind: use this when configuration is real application state, not when `argv` is the whole problem.

## Python example through `ctypes`

`configlib` does not yet ship an official Python package. A thin Python binding can be built on the C ABI. The following example shows the shape.

```python
from ctypes import (
    CDLL, POINTER, byref, c_char_p, c_int, c_longlong, c_void_p
)

CONFIGLIB_OK = 0
CONFIGLIB_VALUE_STRING = 4
CONFIGLIB_VALUE_INT = 2
CONFIGLIB_SOURCE_INTERNAL_DEFAULT = 0
CONFIGLIB_SOURCE_ENVIRONMENT = 2
CONFIGLIB_SOURCE_CLI = 3

lib = CDLL("./build/libconfiglib.so")

lib.configlib_create.restype = c_void_p
lib.configlib_destroy.argtypes = [c_void_p]

lib.configlib_default_string.argtypes = [c_void_p, c_char_p, c_char_p]
lib.configlib_default_int.argtypes = [c_void_p, c_char_p, c_longlong]
lib.configlib_require.argtypes = [c_void_p, c_char_p, c_int]
lib.configlib_set_source_precedence.argtypes = [c_void_p, c_int, c_int]
lib.configlib_add_string.argtypes = [c_void_p, c_char_p, c_char_p, c_int, c_char_p]
lib.configlib_resolve.argtypes = [c_void_p, POINTER(c_void_p)]
lib.configlib_result_ok.argtypes = [c_void_p]
lib.configlib_result_ok.restype = c_int
lib.configlib_result_get_string.argtypes = [c_void_p, c_char_p, POINTER(c_char_p)]
lib.configlib_result_diagnostics.argtypes = [c_void_p]
lib.configlib_result_diagnostics.restype = c_char_p
lib.configlib_result_explain.argtypes = [c_void_p, c_char_p]
lib.configlib_result_explain.restype = c_char_p
lib.configlib_result_destroy.argtypes = [c_void_p]

ctx = lib.configlib_create()
try:
    lib.configlib_default_string(ctx, b"xyz.logging.level", b"info")
    lib.configlib_default_int(ctx, b"xyz.server.port", 8080)

    lib.configlib_set_source_precedence(ctx, CONFIGLIB_SOURCE_INTERNAL_DEFAULT, 10)
    lib.configlib_set_source_precedence(ctx, CONFIGLIB_SOURCE_ENVIRONMENT, 50)
    lib.configlib_set_source_precedence(ctx, CONFIGLIB_SOURCE_CLI, 70)

    lib.configlib_require(ctx, b"xyz.logging.level", CONFIGLIB_VALUE_STRING)
    lib.configlib_require(ctx, b"xyz.server.port", CONFIGLIB_VALUE_INT)

    # A real wrapper would map os.environ and sys.argv through policy objects.
    lib.configlib_add_string(
        ctx,
        b"xyz.logging.level",
        b"debug",
        CONFIGLIB_SOURCE_CLI,
        b"--log-level",
    )

    result = c_void_p()
    status = lib.configlib_resolve(ctx, byref(result))
    if status != CONFIGLIB_OK or not lib.configlib_result_ok(result):
        print(lib.configlib_result_diagnostics(result).decode())
        raise SystemExit(1)

    out = c_char_p()
    if lib.configlib_result_get_string(result, b"xyz.logging.level", byref(out)) == CONFIGLIB_OK:
        print("level =", out.value.decode())

    print(lib.configlib_result_explain(result, b"xyz.logging.level").decode())
finally:
    try:
        lib.configlib_result_destroy(result)
    except NameError:
        pass
    lib.configlib_destroy(ctx)
```

A nicer future `pyconfiglib` layer should wrap handles in classes and expose `FactSet`, `PolicySet`, `ConfigStore`, and `ConfigView` as Pythonic objects. It should still preserve the configlib model rather than collapsing everything into a plain dict.

## Rust example through FFI

Rust can bind to the C ABI. A real crate should generate or maintain bindings and wrap raw handles in safe RAII structs.

```rust
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int};
use std::ptr;

type ConfiglibCtx = std::ffi::c_void;
type ConfiglibResult = std::ffi::c_void;

const CONFIGLIB_OK: c_int = 0;
const CONFIGLIB_VALUE_STRING: c_int = 4;
const CONFIGLIB_SOURCE_CLI: c_int = 3;

#[link(name = "configlib")]
extern "C" {
    fn configlib_create() -> *mut ConfiglibCtx;
    fn configlib_destroy(ctx: *mut ConfiglibCtx);
    fn configlib_default_string(ctx: *mut ConfiglibCtx, key: *const c_char, value: *const c_char) -> c_int;
    fn configlib_require(ctx: *mut ConfiglibCtx, key: *const c_char, ty: c_int) -> c_int;
    fn configlib_add_string(
        ctx: *mut ConfiglibCtx,
        key: *const c_char,
        value: *const c_char,
        kind: c_int,
        source_name: *const c_char,
    ) -> c_int;
    fn configlib_resolve(ctx: *mut ConfiglibCtx, out_result: *mut *mut ConfiglibResult) -> c_int;
    fn configlib_result_ok(result: *const ConfiglibResult) -> c_int;
    fn configlib_result_get_string(
        result: *const ConfiglibResult,
        key: *const c_char,
        out_value: *mut *const c_char,
    ) -> c_int;
    fn configlib_result_destroy(result: *mut ConfiglibResult);
}

fn main() {
    unsafe {
        let ctx = configlib_create();
        assert!(!ctx.is_null());

        let key = CString::new("xyz.logging.level").unwrap();
        let default_value = CString::new("info").unwrap();
        let cli_value = CString::new("debug").unwrap();
        let source = CString::new("--log-level").unwrap();

        configlib_default_string(ctx, key.as_ptr(), default_value.as_ptr());
        configlib_require(ctx, key.as_ptr(), CONFIGLIB_VALUE_STRING);
        configlib_add_string(ctx, key.as_ptr(), cli_value.as_ptr(), CONFIGLIB_SOURCE_CLI, source.as_ptr());

        let mut result: *mut ConfiglibResult = ptr::null_mut();
        if configlib_resolve(ctx, &mut result) == CONFIGLIB_OK && configlib_result_ok(result) != 0 {
            let mut out: *const c_char = ptr::null();
            if configlib_result_get_string(result, key.as_ptr(), &mut out) == CONFIGLIB_OK {
                println!("level = {}", CStr::from_ptr(out).to_string_lossy());
            }
        }

        configlib_result_destroy(result);
        configlib_destroy(ctx);
    }
}
```

The Rust wrapper should own `configlib_ctx*` and `configlib_result*` with `Drop`, convert errors into `Result<T, Error>`, and avoid exposing raw pointers to user code.

## Zig example through C import

Zig can import C headers directly. The exact build setup depends on how the library is linked, but the usage shape is simple.

```zig
const std = @import("std");
const c = @cImport({
    @cInclude("configlib/configlib.h");
});

pub fn main() !void {
    const ctx = c.configlib_create() orelse return error.CreateFailed;
    defer c.configlib_destroy(ctx);

    _ = c.configlib_default_string(ctx, "xyz.logging.level", "info");
    _ = c.configlib_require(ctx, "xyz.logging.level", c.CONFIGLIB_VALUE_STRING);
    _ = c.configlib_add_string(
        ctx,
        "xyz.logging.level",
        "debug",
        c.CONFIGLIB_SOURCE_CLI,
        "--log-level",
    );

    var result: ?*c.configlib_result = null;
    if (c.configlib_resolve(ctx, &result) != c.CONFIGLIB_OK) return error.ResolveFailed;
    defer c.configlib_result_destroy(result);

    if (c.configlib_result_ok(result) == 0) {
        std.debug.print("{s}\n", .{c.configlib_result_diagnostics(result)});
        return error.ConfigInvalid;
    }

    var value: [*c]const u8 = undefined;
    if (c.configlib_result_get_string(result, "xyz.logging.level", &value) == c.CONFIGLIB_OK) {
        std.debug.print("level = {s}\n", .{value});
    }
}
```

## LuaJIT FFI sketch

LuaJIT can call the C ABI through `ffi`. This is useful for tools and embedded scripting layers.

```lua
local ffi = require("ffi")

ffi.cdef[[
typedef struct configlib_ctx configlib_ctx;
typedef struct configlib_result configlib_result;

typedef enum configlib_status {
    CONFIGLIB_OK = 0,
    CONFIGLIB_ERROR = 1,
    CONFIGLIB_INVALID_ARGUMENT = 2,
    CONFIGLIB_NOT_FOUND = 3
} configlib_status;

configlib_ctx* configlib_create(void);
void configlib_destroy(configlib_ctx* ctx);
int configlib_default_string(configlib_ctx* ctx, const char* key, const char* value);
int configlib_require(configlib_ctx* ctx, const char* key, int type);
int configlib_add_string(configlib_ctx* ctx, const char* key, const char* value, int kind, const char* source_name);
int configlib_resolve(configlib_ctx* ctx, configlib_result** out_result);
int configlib_result_ok(const configlib_result* result);
int configlib_result_get_string(const configlib_result* result, const char* key, const char** out_value);
void configlib_result_destroy(configlib_result* result);
]]

local lib = ffi.load("configlib")
local ctx = lib.configlib_create()

lib.configlib_default_string(ctx, "xyz.logging.level", "info")
lib.configlib_require(ctx, "xyz.logging.level", 4) -- CONFIGLIB_VALUE_STRING
lib.configlib_add_string(ctx, "xyz.logging.level", "debug", 3, "--log-level")

local result = ffi.new("configlib_result*[1]")
lib.configlib_resolve(ctx, result)

if lib.configlib_result_ok(result[0]) ~= 0 then
    local out = ffi.new("const char*[1]")
    if lib.configlib_result_get_string(result[0], "xyz.logging.level", out) == 0 then
        print(ffi.string(out[0]))
    end
end

lib.configlib_result_destroy(result[0])
lib.configlib_destroy(ctx)
```

## Shell integration pattern

Shell scripts should usually not link to `configlib` directly. The better pattern is to create a small CLI helper later, for example `configlib-query`, that can print resolved keys from a chosen config policy.

Possible future usage:

```sh
level=$(configlib-query --profile xyz --get xyz.logging.level)
port=$(configlib-query --profile xyz --get xyz.server.port)
```

Until such a helper exists, shell scripts should use normal shell parsing for small jobs.

## Recommended project layout

For a real app using `configlib`:

```text
project/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── config.cpp        # builds FactSet/PolicySet/ConfigStore
│   ├── logging.cpp       # receives store.view("xyz.logging")
│   ├── server.cpp        # receives store.view("xyz.server")
│   └── ai.cpp            # receives store.view("xyz.ai")
└── config/
    ├── xyz.conf.example
    └── policy-notes.md
```

Keep configuration assembly in one place. Pass `ConfigView`s into subsystems.

Good:

```cpp
Logger logger(store.view(configlib::KeyPath("xyz.logging")));
Server server(store.view(configlib::KeyPath("xyz.server")));
```

Avoid:

```cpp
extern GlobalSettings settings;
```

The store is allowed to be central. It should not become a naked global variable.

## Common mistakes

### Treating fallback getters as real defaults

This is convenient:

```cpp
view.get_string_or(configlib::KeyPath("level"), "info");
```

But authoritative defaults should be facts/policies:

```cpp
defaults.set_string(configlib::KeyPath("xyz.logging.level"), "info");
```

Fallback getters are defensive code. They are not the configuration model.

### Hiding precedence in application code

Do not do this:

```cpp
if (cli_value) level = cli_value;
else if (env_value) level = env_value;
else level = default_value;
```

Put precedence in policy and let the resolver explain it.

### Passing the whole store everywhere

For large programs, prefer scoped views:

```cpp
auto parser_cfg = store.view(configlib::KeyPath("flowmini.parser"));
```

The parser should not need the database or UI configuration.

### Exporting secrets by accident

Mark secrets in access policy:

```cpp
access.secret(configlib::KeyPath("xyz.auth.token"));
```

Then redacted/export modes can protect them.

## Version note

This manual describes `configlib` v0.8.0. The C ABI is intentionally narrower than the C++ API at this stage. Some language examples are binding sketches that show intended usage over the C ABI; they are not official packages yet.


## Typed struct bindings

For C++ subsystem code, `StructBinding<T>` can read a `ConfigView` into a plain struct.

```cpp
struct LoggingConfig {
    std::string level;
    bool color;
};

configlib::StructBinding<LoggingConfig> binding("LoggingConfig");
binding
    .string(configlib::KeyPath("level"), &LoggingConfig::level, "info")
    .boolean(configlib::KeyPath("color"), &LoggingConfig::color, true);

auto cfg_result = binding.read(store.view(configlib::KeyPath("xyz.logging")));
```

Bindings are snapshots. Runtime mutation still belongs to `ConfigStore` transactions.


## Schema validation

`ConfigSchema` is the v0.8 contract layer. It validates a resolved config or a scoped view.

```cpp
configlib::ConfigSchema schema;

schema.path(configlib::KeyPath("xyz.logging.level"))
    .string()
    .required()
    .allowed({"trace", "debug", "info", "warn", "error"});

schema.path(configlib::KeyPath("xyz.server.port"))
    .integer()
    .required()
    .range(static_cast<std::int64_t>(1), static_cast<std::int64_t>(65535));

auto checked = schema.validate(result.config());
if (!checked.ok()) {
    std::cerr << checked.diagnostics().format();
}
```

Schema is not a replacement for defaults or policy. Defaults are still facts. Policy still governs precedence, mutation, and export. Schema describes what the resolved configuration is expected to look like.
