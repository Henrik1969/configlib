// SPDX-License-Identifier: MIT

#include <configlib/configlib.h>
#include <configlib/configlib.hpp>

#include <exception>
#include <memory>
#include <variant>
#include <string>

struct configlib_ctx {
    configlib::FactSet facts;
    configlib::PolicySet policies;
};

struct configlib_result {
    configlib::ResolveResult result;
    std::string diagnostics_cache;
    std::string explain_cache;

    explicit configlib_result(configlib::ResolveResult r)
        : result(std::move(r)), diagnostics_cache(result.diagnostics().format()) {}
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

configlib::Source make_source(configlib_source_kind kind, const char* source_name) {
    return {to_cpp(kind), source_name ? source_name : ""};
}

bool bad_ctx(configlib_ctx* ctx) { return ctx == nullptr; }
bool bad_result(const configlib_result* result) { return result == nullptr; }
bool bad_str(const char* text) { return text == nullptr; }

template <typename Fn>
configlib_status guard(Fn&& fn) {
    try {
        fn();
        return CONFIGLIB_OK;
    } catch (...) {
        return CONFIGLIB_ERROR;
    }
}

} // namespace

extern "C" {

configlib_ctx* configlib_create(void) {
    try {
        return new configlib_ctx{};
    } catch (...) {
        return nullptr;
    }
}

void configlib_destroy(configlib_ctx* ctx) { delete ctx; }

void configlib_clear(configlib_ctx* ctx) {
    if (!ctx) return;
    ctx->facts.clear();
    ctx->policies = configlib::PolicySet{};
}

configlib_status configlib_set_source_precedence(configlib_ctx* ctx, configlib_source_kind kind, int precedence) {
    if (bad_ctx(ctx)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.set_precedence(to_cpp(kind), precedence); });
}

configlib_status configlib_require(configlib_ctx* ctx, const char* key, configlib_value_type type) {
    if (bad_ctx(ctx) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.require(configlib::KeyPath(key), to_cpp(type)); });
}

configlib_status configlib_optional(configlib_ctx* ctx, const char* key, configlib_value_type type) {
    if (bad_ctx(ctx) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.optional(configlib::KeyPath(key), to_cpp(type)); });
}

configlib_status configlib_default_string(configlib_ctx* ctx, const char* key, const char* value) {
    if (bad_ctx(ctx) || bad_str(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.defaulted(configlib::KeyPath(key), configlib::Value(value)); });
}

configlib_status configlib_default_int(configlib_ctx* ctx, const char* key, int64_t value) {
    if (bad_ctx(ctx) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.defaulted(configlib::KeyPath(key), configlib::Value(value)); });
}

configlib_status configlib_default_bool(configlib_ctx* ctx, const char* key, int value) {
    if (bad_ctx(ctx) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->policies.defaulted(configlib::KeyPath(key), configlib::Value(value != 0)); });
}

configlib_status configlib_add_string(configlib_ctx* ctx, const char* key, const char* value, configlib_source_kind kind, const char* source_name) {
    if (bad_ctx(ctx) || bad_str(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add(configlib::KeyPath(key), configlib::Value(value), make_source(kind, source_name), ctx->policies.precedence_for(to_cpp(kind))); });
}

configlib_status configlib_add_int(configlib_ctx* ctx, const char* key, int64_t value, configlib_source_kind kind, const char* source_name) {
    if (bad_ctx(ctx) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add(configlib::KeyPath(key), configlib::Value(value), make_source(kind, source_name), ctx->policies.precedence_for(to_cpp(kind))); });
}

configlib_status configlib_add_bool(configlib_ctx* ctx, const char* key, int value, configlib_source_kind kind, const char* source_name) {
    if (bad_ctx(ctx) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add(configlib::KeyPath(key), configlib::Value(value != 0), make_source(kind, source_name), ctx->policies.precedence_for(to_cpp(kind))); });
}

configlib_status configlib_resolve(configlib_ctx* ctx, configlib_result** out_result) {
    if (bad_ctx(ctx) || out_result == nullptr) return CONFIGLIB_INVALID_ARGUMENT;
    *out_result = nullptr;
    try {
        *out_result = new configlib_result(configlib::resolve(ctx->facts, ctx->policies));
        return CONFIGLIB_OK;
    } catch (...) {
        return CONFIGLIB_ERROR;
    }
}

void configlib_result_destroy(configlib_result* result) { delete result; }

int configlib_result_ok(const configlib_result* result) {
    if (!result) return 0;
    return result->result.ok() ? 1 : 0;
}

configlib_status configlib_result_get_string(const configlib_result* result, const char* key, const char** out_value) {
    if (bad_result(result) || bad_str(key) || out_value == nullptr) return CONFIGLIB_INVALID_ARGUMENT;
    const auto* entry = result->result.config().explain(configlib::KeyPath(key));
    if (!entry || entry->value.type() != configlib::ValueType::String) return CONFIGLIB_NOT_FOUND;
    *out_value = std::get<std::string>(entry->value.storage()).c_str();
    return CONFIGLIB_OK;
}

configlib_status configlib_result_get_int(const configlib_result* result, const char* key, int64_t* out_value) {
    if (bad_result(result) || bad_str(key) || out_value == nullptr) return CONFIGLIB_INVALID_ARGUMENT;
    const auto value = result->result.config().get(configlib::KeyPath(key));
    if (!value || value->type() != configlib::ValueType::Int) return CONFIGLIB_NOT_FOUND;
    *out_value = value->as_int();
    return CONFIGLIB_OK;
}

configlib_status configlib_result_get_bool(const configlib_result* result, const char* key, int* out_value) {
    if (bad_result(result) || bad_str(key) || out_value == nullptr) return CONFIGLIB_INVALID_ARGUMENT;
    const auto value = result->result.config().get(configlib::KeyPath(key));
    if (!value || value->type() != configlib::ValueType::Bool) return CONFIGLIB_NOT_FOUND;
    *out_value = value->as_bool() ? 1 : 0;
    return CONFIGLIB_OK;
}


configlib_status configlib_internal_default_string(configlib_ctx* ctx, const char* key, const char* value) {
    if (bad_ctx(ctx) || bad_str(key) || bad_str(value)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add_default(configlib::KeyPath(key), configlib::Value(value)); });
}

configlib_status configlib_internal_default_int(configlib_ctx* ctx, const char* key, int64_t value) {
    if (bad_ctx(ctx) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add_default(configlib::KeyPath(key), configlib::Value(value)); });
}

configlib_status configlib_internal_default_bool(configlib_ctx* ctx, const char* key, int value) {
    if (bad_ctx(ctx) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] { ctx->facts.add_default(configlib::KeyPath(key), configlib::Value(value != 0)); });
}

configlib_status configlib_load_env_mapping(configlib_ctx* ctx, const char* variable, const char* key, configlib_value_type type) {
    if (bad_ctx(ctx) || bad_str(variable) || bad_str(key)) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] {
        configlib::EnvironmentLoaderPolicy env;
        env.enabled().map(variable, configlib::KeyPath(key), to_cpp(type));
        auto report = configlib::load_environment(env, ctx->policies);
        for (const auto& fact : report.facts.all()) ctx->facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
    });
}

configlib_status configlib_load_cli_args(configlib_ctx* ctx, int argc, const char* const argv[], const char* option, const char* key, configlib_value_type type) {
    if (bad_ctx(ctx) || argv == nullptr || bad_str(option) || bad_str(key) || argc < 0) return CONFIGLIB_INVALID_ARGUMENT;
    return guard([&] {
        configlib::CliLoaderPolicy cli;
        cli.enabled().option(option, configlib::KeyPath(key), to_cpp(type));
        auto report = configlib::load_cli(cli, ctx->policies, argc, argv);
        for (const auto& fact : report.facts.all()) ctx->facts.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
    });
}

const char* configlib_result_diagnostics(const configlib_result* result) {
    if (!result) return "";
    return result->diagnostics_cache.c_str();
}

const char* configlib_result_explain(const configlib_result* result, const char* key) {
    if (!result || !key) return "";
    auto* mutable_result = const_cast<configlib_result*>(result);
    mutable_result->explain_cache = result->result.config().format_explanation(configlib::KeyPath(key));
    return mutable_result->explain_cache.c_str();
}

} // extern C
