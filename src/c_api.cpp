// SPDX-License-Identifier: MIT

#include <configlib/configlib.h>
#include <configlib/configlib.hpp>

#include <exception>
#include <string>
#include <utility>
#include <vector>
#include <variant>

struct configlib_ctx {
    configlib::FactSet facts;
    configlib::PolicySet policies;
};

struct configlib_result {
    configlib::ResolveResult result;
    mutable std::string diagnostics_cache;
    mutable std::string explain_cache;

    explicit configlib_result(configlib::ResolveResult r)
        : result(std::move(r)), diagnostics_cache(result.diagnostics().format()) {}
};

struct configlib_schema {
    configlib::ConfigSchema schema;
};

struct configlib_schema_result {
    configlib::SchemaValidationResult result;
    mutable std::string diagnostics_cache;

    explicit configlib_schema_result(configlib::SchemaValidationResult r)
        : result(std::move(r)), diagnostics_cache(result.diagnostics().format()) {}
};

struct configlib_access_policy {
    configlib::AccessPolicy access;
};

struct configlib_store {
    configlib::ConfigStore store;
    mutable std::string string_cache;
    mutable std::string explain_cache;
    mutable std::string export_cache;
    mutable std::string diagnostics_cache;

    explicit configlib_store(configlib::ConfigStore s)
        : store(std::move(s)), diagnostics_cache(store.diagnostics().format()) {}
};

struct configlib_view {
    configlib::ConfigView view;
    mutable std::string string_cache;
    mutable std::string explain_cache;
    mutable std::string export_cache;

    explicit configlib_view(configlib::ConfigView v) : view(std::move(v)) {}
};

struct configlib_transaction {
    configlib::ConfigTransaction tx;
    mutable std::string diagnostics_cache;

    explicit configlib_transaction(configlib::ConfigTransaction t) : tx(std::move(t)) {}
};

namespace {

configlib::ValueType to_cpp(configlib_value_type type) {
    switch (type) {
        case CONFIGLIB_VALUE_NULL: return configlib::ValueType::Null;
        case CONFIGLIB_VALUE_BOOL: return configlib::ValueType::Bool;
        case CONFIGLIB_VALUE_INT: return configlib::ValueType::Int;
        case CONFIGLIB_VALUE_DOUBLE: return configlib::ValueType::Double;
        case CONFIGLIB_VALUE_STRING: return configlib::ValueType::String;
    }
    return configlib::ValueType::Null;
}

configlib::SourceKind to_cpp(configlib_source_kind kind) {
    switch (kind) {
        case CONFIGLIB_SOURCE_INTERNAL_DEFAULT: return configlib::SourceKind::InternalDefault;
        case CONFIGLIB_SOURCE_FILE: return configlib::SourceKind::File;
        case CONFIGLIB_SOURCE_ENVIRONMENT: return configlib::SourceKind::Environment;
        case CONFIGLIB_SOURCE_CLI: return configlib::SourceKind::CLI;
        case CONFIGLIB_SOURCE_RUNTIME: return configlib::SourceKind::Runtime;
        case CONFIGLIB_SOURCE_UNKNOWN: return configlib::SourceKind::Unknown;
    }
    return configlib::SourceKind::Unknown;
}

configlib::ExportMode to_cpp(configlib_export_mode mode) {
    switch (mode) {
        case CONFIGLIB_EXPORT_EFFECTIVE: return configlib::ExportMode::Effective;
        case CONFIGLIB_EXPORT_EFFECTIVE_REDACTED: return configlib::ExportMode::EffectiveRedacted;
        case CONFIGLIB_EXPORT_CHANGED_ONLY: return configlib::ExportMode::ChangedOnly;
        case CONFIGLIB_EXPORT_CHANGED_ONLY_REDACTED: return configlib::ExportMode::ChangedOnlyRedacted;
        case CONFIGLIB_EXPORT_RUNTIME_CHANGES_ONLY: return configlib::ExportMode::RuntimeChangesOnly;
        case CONFIGLIB_EXPORT_RUNTIME_CHANGES_ONLY_REDACTED: return configlib::ExportMode::RuntimeChangesOnlyRedacted;
    }
    return configlib::ExportMode::Effective;
}

configlib::Source make_source(configlib_source_kind kind, const char* source_name) {
    return {to_cpp(kind), source_name ? source_name : ""};
}

bool bad_str(const char* text) { return text == nullptr; }
bool bad_key(const char* key) { return key == nullptr || !configlib::KeyPath::valid_dotted(key); }

template <typename Fn>
configlib_status guard(Fn&& fn) {
    try {
        fn();
        return CONFIGLIB_OK;
    } catch (...) {
        return CONFIGLIB_ERROR;
    }
}

void add_report_facts(configlib_ctx* ctx, const configlib::LoadReport& report) {
    for (const auto& fact : report.facts.all()) ctx->facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
}

std::vector<std::string> collect_allowed_strings(const configlib::PolicySet& policies, const char* key, const char* value) {
    std::vector<std::string> values;
    const auto* existing = policies.find_key_policy(configlib::KeyPath(key));
    if (existing) values.assign(existing->allowed_strings.begin(), existing->allowed_strings.end());
    values.emplace_back(value);
    return values;
}

configlib_status get_string_from_value(const std::optional<configlib::Value>& value, std::string& cache, const char** out_value) {
    if (!out_value) return CONFIGLIB_INVALID_ARGUMENT;
    *out_value = nullptr;
    if (!value || value->type() != configlib::ValueType::String) return CONFIGLIB_NOT_FOUND;
    cache = value->as_string().value_or(std::string{});
    *out_value = cache.c_str();
    return CONFIGLIB_OK;
}

configlib_status get_int_from_value(const std::optional<configlib::Value>& value, int64_t* out_value) {
    if (!out_value) return CONFIGLIB_INVALID_ARGUMENT;
    if (!value || value->type() != configlib::ValueType::Int) return CONFIGLIB_NOT_FOUND;
    *out_value = value->as_integer().value_or(0);
    return CONFIGLIB_OK;
}

configlib_status get_bool_from_value(const std::optional<configlib::Value>& value, int* out_value) {
    if (!out_value) return CONFIGLIB_INVALID_ARGUMENT;
    if (!value || value->type() != configlib::ValueType::Bool) return CONFIGLIB_NOT_FOUND;
    *out_value = value->as_boolean().value_or(false) ? 1 : 0;
    return CONFIGLIB_OK;
}

configlib_status get_double_from_value(const std::optional<configlib::Value>& value, double* out_value) {
    if (!out_value) return CONFIGLIB_INVALID_ARGUMENT;
    if (!value || (value->type() != configlib::ValueType::Double && value->type() != configlib::ValueType::Int)) return CONFIGLIB_NOT_FOUND;
    *out_value = value->as_floating().value_or(0.0);
    return CONFIGLIB_OK;
}

} // namespace

extern "C" {

int configlib_version_major(void) { return CONFIGLIB_VERSION_MAJOR; }
int configlib_version_minor(void) { return CONFIGLIB_VERSION_MINOR; }
int configlib_version_patch(void) { return CONFIGLIB_VERSION_PATCH; }

const char* configlib_status_name(configlib_status status) {
    switch (status) {
        case CONFIGLIB_OK: return "ok";
        case CONFIGLIB_ERROR: return "error";
        case CONFIGLIB_INVALID_ARGUMENT: return "invalid-argument";
        case CONFIGLIB_NOT_FOUND: return "not-found";
        case CONFIGLIB_DENIED: return "denied";
        case CONFIGLIB_VALIDATION_FAILED: return "validation-failed";
    }
    return "unknown";
}

const char* configlib_value_type_name(configlib_value_type type) { return configlib::value_type_name(to_cpp(type)); }
const char* configlib_source_kind_name(configlib_source_kind kind) { return configlib::source_kind_name(to_cpp(kind)); }
const char* configlib_export_mode_name(configlib_export_mode mode) { return configlib::export_mode_name(to_cpp(mode)); }
int configlib_key_is_valid(const char* key) { return key != nullptr && configlib::KeyPath::valid_dotted(key); }

configlib_ctx* configlib_create(void) {
    try { return new configlib_ctx{}; } catch (...) { return nullptr; }
}

void configlib_destroy(configlib_ctx* ctx) { delete ctx; }

void configlib_clear(configlib_ctx* ctx) {
    if (!ctx) return;
    ctx->facts.clear();
    ctx->policies = configlib::PolicySet{};
}

configlib_status configlib_set_source_precedence(configlib_ctx* ctx, configlib_source_kind kind, int precedence) {
    if (!ctx) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.set_precedence(to_cpp(kind), precedence); });
}

configlib_status configlib_require(configlib_ctx* ctx, const char* key, configlib_value_type type) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.require(configlib::KeyPath(key), to_cpp(type)); });
}

configlib_status configlib_optional(configlib_ctx* ctx, const char* key, configlib_value_type type) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.optional(configlib::KeyPath(key), to_cpp(type)); });
}

configlib_status configlib_allowed_string(configlib_ctx* ctx, const char* key, const char* value) {
    if (!ctx || bad_key(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.allowed_strings(configlib::KeyPath(key), collect_allowed_strings(ctx->policies, key, value)); });
}

configlib_status configlib_int_range(configlib_ctx* ctx, const char* key, int64_t min_value, int64_t max_value) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.int_range(configlib::KeyPath(key), min_value, max_value); });
}

configlib_status configlib_add_string(configlib_ctx* ctx, const char* key, const char* value, configlib_source_kind kind, const char* source_name) {
    if (!ctx || bad_key(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add(configlib::KeyPath(key), configlib::Value(value), make_source(kind, source_name), ctx->policies.precedence_for(to_cpp(kind))); });
}

configlib_status configlib_add_int(configlib_ctx* ctx, const char* key, int64_t value, configlib_source_kind kind, const char* source_name) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add(configlib::KeyPath(key), configlib::Value(value), make_source(kind, source_name), ctx->policies.precedence_for(to_cpp(kind))); });
}

configlib_status configlib_add_bool(configlib_ctx* ctx, const char* key, int value, configlib_source_kind kind, const char* source_name) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add(configlib::KeyPath(key), configlib::Value(value != 0), make_source(kind, source_name), ctx->policies.precedence_for(to_cpp(kind))); });
}

configlib_status configlib_add_double(configlib_ctx* ctx, const char* key, double value, configlib_source_kind kind, const char* source_name) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add(configlib::KeyPath(key), configlib::Value(value), make_source(kind, source_name), ctx->policies.precedence_for(to_cpp(kind))); });
}

configlib_status configlib_resolve(configlib_ctx* ctx, configlib_result** out_result) {
    if (!ctx || out_result == nullptr) return CONFIGLIB_INVALID_ARGUMENT;
    *out_result = nullptr;
    try {
        *out_result = new configlib_result(configlib::resolve(ctx->facts, ctx->policies));
        return CONFIGLIB_OK;
    } catch (...) {
        return CONFIGLIB_ERROR;
    }
}

void configlib_result_destroy(configlib_result* result) { delete result; }

int configlib_result_ok(const configlib_result* result) { return result && result->result.ok() ? 1 : 0; }

int configlib_result_contains(const configlib_result* result, const char* key) {
    if (!result || !key) return 0;
    return result->result.config().contains(configlib::KeyPath(key)) ? 1 : 0;
}

configlib_status configlib_result_get_string(const configlib_result* result, const char* key, const char** out_value) {
    if (!result || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_string_from_value(result->result.config().get_value(configlib::KeyPath(key)), result->explain_cache, out_value);
}

configlib_status configlib_result_get_int(const configlib_result* result, const char* key, int64_t* out_value) {
    if (!result || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_int_from_value(result->result.config().get_value(configlib::KeyPath(key)), out_value);
}

configlib_status configlib_result_get_bool(const configlib_result* result, const char* key, int* out_value) {
    if (!result || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_bool_from_value(result->result.config().get_value(configlib::KeyPath(key)), out_value);
}

configlib_status configlib_result_get_double(const configlib_result* result, const char* key, double* out_value) {
    if (!result || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_double_from_value(result->result.config().get_value(configlib::KeyPath(key)), out_value);
}

const char* configlib_result_diagnostics(const configlib_result* result) {
    if (!result) return "";
    result->diagnostics_cache = result->result.diagnostics().format();
    return result->diagnostics_cache.c_str();
}

const char* configlib_result_explain(const configlib_result* result, const char* key) {
    if (!result || !key) return "";
    result->explain_cache = result->result.config().format_explanation(configlib::KeyPath(key));
    return result->explain_cache.c_str();
}

configlib_status configlib_internal_default_string(configlib_ctx* ctx, const char* key, const char* value) {
    if (!ctx || bad_key(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add_default(configlib::KeyPath(key), configlib::Value(value)); });
}

configlib_status configlib_internal_default_int(configlib_ctx* ctx, const char* key, int64_t value) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add_default(configlib::KeyPath(key), configlib::Value(value)); });
}

configlib_status configlib_internal_default_bool(configlib_ctx* ctx, const char* key, int value) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add_default(configlib::KeyPath(key), configlib::Value(value != 0)); });
}

configlib_status configlib_internal_default_double(configlib_ctx* ctx, const char* key, double value) {
    if (!ctx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add_default(configlib::KeyPath(key), configlib::Value(value)); });
}

configlib_status configlib_load_env_mapping(configlib_ctx* ctx, const char* variable, const char* key, configlib_value_type type) {
    if (!ctx || bad_str(variable) || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] {
        configlib::EnvironmentLoaderPolicy env;
        env.enabled().map(variable, configlib::KeyPath(key), to_cpp(type));
        add_report_facts(ctx, configlib::load_environment(env, ctx->policies));
    });
}

configlib_status configlib_load_cli_args(configlib_ctx* ctx, int argc, const char* const argv[], const char* option, const char* key, configlib_value_type type) {
    if (!ctx || argv == nullptr || bad_str(option) || bad_key(key) || argc < 0) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] {
        configlib::CliLoaderPolicy cli;
        cli.enabled().option(option, configlib::KeyPath(key), to_cpp(type));
        add_report_facts(ctx, configlib::load_cli(cli, ctx->policies, argc, argv));
    });
}

configlib_status configlib_load_file(configlib_ctx* ctx, const char* path) {
    if (!ctx || bad_str(path)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { add_report_facts(ctx, configlib::load_config_file(path, ctx->policies)); });
}

configlib_schema* configlib_schema_create(void) {
    try { return new configlib_schema{}; } catch (...) { return nullptr; }
}

void configlib_schema_destroy(configlib_schema* schema) { delete schema; }

configlib_status configlib_schema_require(configlib_schema* schema, const char* key, configlib_value_type type) {
    if (!schema || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).type(to_cpp(type)).required(); });
}

configlib_status configlib_schema_optional(configlib_schema* schema, const char* key, configlib_value_type type) {
    if (!schema || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).type(to_cpp(type)).optional(); });
}

configlib_status configlib_schema_allowed_string(configlib_schema* schema, const char* key, const char* value) {
    if (!schema || bad_key(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] {
        std::vector<std::string> values;
        if (const auto* rule = schema->schema.find(configlib::KeyPath(key))) values.assign(rule->allowed_strings.begin(), rule->allowed_strings.end());
        values.emplace_back(value);
        schema->schema.path(configlib::KeyPath(key)).allowed(std::move(values));
    });
}

configlib_status configlib_schema_int_range(configlib_schema* schema, const char* key, int64_t min_value, int64_t max_value) {
    if (!schema || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).range(min_value, max_value); });
}

configlib_status configlib_schema_double_range(configlib_schema* schema, const char* key, double min_value, double max_value) {
    if (!schema || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).range(min_value, max_value); });
}

configlib_status configlib_schema_documented_default_string(configlib_schema* schema, const char* key, const char* value) {
    if (!schema || bad_key(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).documented_default(configlib::Value(value)); });
}

configlib_status configlib_schema_documented_default_int(configlib_schema* schema, const char* key, int64_t value) {
    if (!schema || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).documented_default(configlib::Value(value)); });
}

configlib_status configlib_schema_documented_default_bool(configlib_schema* schema, const char* key, int value) {
    if (!schema || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).documented_default(configlib::Value(value != 0)); });
}

configlib_status configlib_schema_documented_default_double(configlib_schema* schema, const char* key, double value) {
    if (!schema || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).documented_default(configlib::Value(value)); });
}

configlib_status configlib_schema_describe(configlib_schema* schema, const char* key, const char* description) {
    if (!schema || bad_key(key) || bad_str(description)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { schema->schema.path(configlib::KeyPath(key)).describe(description); });
}

configlib_status configlib_schema_validate_result(const configlib_schema* schema, const configlib_result* result, configlib_schema_result** out_result) {
    if (!schema || !result || !out_result) return CONFIGLIB_INVALID_ARGUMENT;
    *out_result = nullptr;
    try { *out_result = new configlib_schema_result(schema->schema.validate(result->result.config())); return CONFIGLIB_OK; } catch (...) { return CONFIGLIB_ERROR; }
}

configlib_status configlib_schema_validate_store(const configlib_schema* schema, const configlib_store* store, configlib_schema_result** out_result) {
    if (!schema || !store || !out_result) return CONFIGLIB_INVALID_ARGUMENT;
    *out_result = nullptr;
    try { *out_result = new configlib_schema_result(schema->schema.validate(store->store.base_config())); return CONFIGLIB_OK; } catch (...) { return CONFIGLIB_ERROR; }
}

configlib_status configlib_schema_validate_view(const configlib_schema* schema, const configlib_view* view, configlib_schema_result** out_result) {
    if (!schema || !view || !out_result) return CONFIGLIB_INVALID_ARGUMENT;
    *out_result = nullptr;
    try { *out_result = new configlib_schema_result(schema->schema.validate(view->view)); return CONFIGLIB_OK; } catch (...) { return CONFIGLIB_ERROR; }
}

void configlib_schema_result_destroy(configlib_schema_result* result) { delete result; }
int configlib_schema_result_ok(const configlib_schema_result* result) { return result && result->result.ok() ? 1 : 0; }
const char* configlib_schema_result_diagnostics(const configlib_schema_result* result) {
    if (!result) return "";
    result->diagnostics_cache = result->result.diagnostics().format();
    return result->diagnostics_cache.c_str();
}

configlib_access_policy* configlib_access_policy_create(void) {
    try { return new configlib_access_policy{}; } catch (...) { return nullptr; }
}
void configlib_access_policy_destroy(configlib_access_policy* access) { delete access; }

configlib_status configlib_access_runtime_mutable(configlib_access_policy* access, const char* key, int value) {
    if (!access || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { access->access.runtime_mutable(configlib::KeyPath(key), value != 0); });
}
configlib_status configlib_access_exportable(configlib_access_policy* access, const char* key, int value) {
    if (!access || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { access->access.exportable(configlib::KeyPath(key), value != 0); });
}
configlib_status configlib_access_secret(configlib_access_policy* access, const char* key, int value) {
    if (!access || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { access->access.secret(configlib::KeyPath(key), value != 0); });
}
configlib_status configlib_access_resettable(configlib_access_policy* access, const char* key, int value) {
    if (!access || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { access->access.resettable(configlib::KeyPath(key), value != 0); });
}

configlib_status configlib_store_from_result(const configlib_ctx* ctx, const configlib_result* result, const configlib_access_policy* access, configlib_store** out_store) {
    if (!ctx || !result || !out_store) return CONFIGLIB_INVALID_ARGUMENT;
    *out_store = nullptr;
    try {
        configlib::AccessPolicy access_copy = access ? access->access : configlib::AccessPolicy{};
        *out_store = new configlib_store(configlib::ConfigStore::from_result(result->result, ctx->policies, std::move(access_copy)));
        return CONFIGLIB_OK;
    } catch (...) { return CONFIGLIB_ERROR; }
}

void configlib_store_destroy(configlib_store* store) { delete store; }
int configlib_store_contains(const configlib_store* store, const char* key) {
    if (!store || !key) return 0;
    return store->store.contains(configlib::KeyPath(key)) ? 1 : 0;
}
configlib_status configlib_store_get_string(const configlib_store* store, const char* key, const char** out_value) {
    if (!store || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_string_from_value(store->store.get_value(configlib::KeyPath(key)), store->string_cache, out_value);
}
configlib_status configlib_store_get_int(const configlib_store* store, const char* key, int64_t* out_value) {
    if (!store || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_int_from_value(store->store.get_value(configlib::KeyPath(key)), out_value);
}
configlib_status configlib_store_get_bool(const configlib_store* store, const char* key, int* out_value) {
    if (!store || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_bool_from_value(store->store.get_value(configlib::KeyPath(key)), out_value);
}
configlib_status configlib_store_get_double(const configlib_store* store, const char* key, double* out_value) {
    if (!store || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_double_from_value(store->store.get_value(configlib::KeyPath(key)), out_value);
}
int configlib_store_has_runtime_change(const configlib_store* store, const char* key) {
    if (!store || !key) return 0;
    return store->store.has_runtime_change(configlib::KeyPath(key)) ? 1 : 0;
}
const char* configlib_store_explain(const configlib_store* store, const char* key) {
    if (!store || !key) return "";
    store->explain_cache = store->store.explain(configlib::KeyPath(key));
    return store->explain_cache.c_str();
}
const char* configlib_store_export(const configlib_store* store, configlib_export_mode mode) {
    if (!store) return "";
    store->export_cache = store->store.export_config(to_cpp(mode));
    return store->export_cache.c_str();
}
const char* configlib_store_diagnostics(const configlib_store* store) {
    if (!store) return "";
    store->diagnostics_cache = store->store.diagnostics().format();
    return store->diagnostics_cache.c_str();
}
configlib_status configlib_store_view(const configlib_store* store, const char* prefix, configlib_view** out_view) {
    if (!store || !prefix || !out_view) return CONFIGLIB_INVALID_ARGUMENT;
    *out_view = nullptr;
    try { *out_view = new configlib_view(store->store.view(configlib::KeyPath(prefix))); return CONFIGLIB_OK; } catch (...) { return CONFIGLIB_ERROR; }
}
configlib_status configlib_store_begin_transaction(configlib_store* store, configlib_transaction** out_tx) {
    if (!store || !out_tx) return CONFIGLIB_INVALID_ARGUMENT;
    *out_tx = nullptr;
    try { *out_tx = new configlib_transaction(store->store.begin_transaction()); return CONFIGLIB_OK; } catch (...) { return CONFIGLIB_ERROR; }
}

void configlib_view_destroy(configlib_view* view) { delete view; }
int configlib_view_contains(const configlib_view* view, const char* local_key) {
    if (!view || !local_key) return 0;
    return view->view.contains(configlib::KeyPath(local_key)) ? 1 : 0;
}
configlib_status configlib_view_get_string(const configlib_view* view, const char* local_key, const char** out_value) {
    if (!view || bad_key(local_key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_string_from_value(view->view.get_value(configlib::KeyPath(local_key)), view->string_cache, out_value);
}
configlib_status configlib_view_get_int(const configlib_view* view, const char* local_key, int64_t* out_value) {
    if (!view || bad_key(local_key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_int_from_value(view->view.get_value(configlib::KeyPath(local_key)), out_value);
}
configlib_status configlib_view_get_bool(const configlib_view* view, const char* local_key, int* out_value) {
    if (!view || bad_key(local_key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_bool_from_value(view->view.get_value(configlib::KeyPath(local_key)), out_value);
}
configlib_status configlib_view_get_double(const configlib_view* view, const char* local_key, double* out_value) {
    if (!view || bad_key(local_key)) return CONFIGLIB_INVALID_ARGUMENT;
    return get_double_from_value(view->view.get_value(configlib::KeyPath(local_key)), out_value);
}
const char* configlib_view_explain(const configlib_view* view, const char* local_key) {
    if (!view || !local_key) return "";
    view->explain_cache = view->view.explain(configlib::KeyPath(local_key));
    return view->explain_cache.c_str();
}
const char* configlib_view_export(const configlib_view* view, configlib_export_mode mode) {
    if (!view) return "";
    view->export_cache = view->view.export_config(to_cpp(mode));
    return view->export_cache.c_str();
}
const char* configlib_view_export_local(const configlib_view* view, configlib_export_mode mode) {
    if (!view) return "";
    view->export_cache = view->view.export_local_config(to_cpp(mode));
    return view->export_cache.c_str();
}

void configlib_transaction_destroy(configlib_transaction* tx) { delete tx; }
configlib_status configlib_transaction_set_string(configlib_transaction* tx, const char* key, const char* value) {
    if (!tx || bad_key(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { tx->tx.set_string(configlib::KeyPath(key), value); });
}
configlib_status configlib_transaction_set_int(configlib_transaction* tx, const char* key, int64_t value) {
    if (!tx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { tx->tx.set_integer(configlib::KeyPath(key), value); });
}
configlib_status configlib_transaction_set_bool(configlib_transaction* tx, const char* key, int value) {
    if (!tx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { tx->tx.set_boolean(configlib::KeyPath(key), value != 0); });
}
configlib_status configlib_transaction_set_double(configlib_transaction* tx, const char* key, double value) {
    if (!tx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { tx->tx.set_floating(configlib::KeyPath(key), value); });
}
configlib_status configlib_transaction_erase(configlib_transaction* tx, const char* key) {
    if (!tx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { tx->tx.erase(configlib::KeyPath(key)); });
}
configlib_status configlib_transaction_reset_to_base(configlib_transaction* tx, const char* key) {
    if (!tx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { tx->tx.reset_to_base(configlib::KeyPath(key)); });
}
configlib_status configlib_transaction_reset_to_default(configlib_transaction* tx, const char* key) {
    if (!tx || bad_key(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { tx->tx.reset_to_default(configlib::KeyPath(key)); });
}
int configlib_transaction_validate(const configlib_transaction* tx) {
    if (!tx) return 0;
    return tx->tx.validate() ? 1 : 0;
}
const char* configlib_transaction_diagnostics(const configlib_transaction* tx) {
    if (!tx) return "";
    tx->diagnostics_cache = tx->tx.diagnostics().format();
    return tx->diagnostics_cache.c_str();
}
int configlib_transaction_commit(configlib_transaction* tx) {
    if (!tx) return 0;
    return tx->tx.commit() ? 1 : 0;
}
void configlib_transaction_rollback(configlib_transaction* tx) {
    if (!tx) return;
    tx->tx.rollback();
}

} // extern C
