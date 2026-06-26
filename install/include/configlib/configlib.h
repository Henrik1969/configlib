// SPDX-License-Identifier: MIT

#pragma once

/**
 * @file configlib.h
 * Opaque C ABI for configlib.
 *
 * The C ABI is the intended long-term binding surface for non-C++ languages.
 * Handles returned from successful calls are valid until destroyed by their
 * matching destroy function. C functions return explicit status codes and do
 * not expose C++ exceptions or STL types.
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIGLIB_C_VERSION_MAJOR 0
#define CONFIGLIB_C_VERSION_MINOR 9
#define CONFIGLIB_C_VERSION_PATCH 0

/** Opaque mutable context holding facts and policy before resolution. */
typedef struct configlib_ctx configlib_ctx;
/** Opaque resolved result object produced by configlib_resolve(). */
typedef struct configlib_result configlib_result;
/** Opaque schema object for validating resolved/store/view configuration shape. */
typedef struct configlib_schema configlib_schema;
/** Opaque schema validation result. */
typedef struct configlib_schema_result configlib_schema_result;
/** Opaque runtime access/export policy object. */
typedef struct configlib_access_policy configlib_access_policy;
/** Opaque governed runtime store. */
typedef struct configlib_store configlib_store;
/** Opaque borrowed scoped read-only view into a store. */
typedef struct configlib_view configlib_view;
/** Opaque transaction for staged runtime store mutation. */
typedef struct configlib_transaction configlib_transaction;

/** Status code returned by C ABI functions. */
typedef enum configlib_status {
    CONFIGLIB_OK = 0,
    CONFIGLIB_ERROR = 1,
    CONFIGLIB_INVALID_ARGUMENT = 2,
    CONFIGLIB_NOT_FOUND = 3,
    CONFIGLIB_DENIED = 4,
    CONFIGLIB_VALIDATION_FAILED = 5
} configlib_status;

/** Scalar value type tag used by the C ABI. */
typedef enum configlib_value_type {
    CONFIGLIB_VALUE_NULL = 0,
    CONFIGLIB_VALUE_BOOL = 1,
    CONFIGLIB_VALUE_INT = 2,
    CONFIGLIB_VALUE_DOUBLE = 3,
    CONFIGLIB_VALUE_STRING = 4
} configlib_value_type;

/** Source kind used for fact provenance through the C ABI. */
typedef enum configlib_source_kind {
    CONFIGLIB_SOURCE_INTERNAL_DEFAULT = 0,
    CONFIGLIB_SOURCE_FILE = 1,
    CONFIGLIB_SOURCE_ENVIRONMENT = 2,
    CONFIGLIB_SOURCE_CLI = 3,
    CONFIGLIB_SOURCE_RUNTIME = 4,
    CONFIGLIB_SOURCE_UNKNOWN = 5
} configlib_source_kind;

/** Export mode for store/view serialization. */
typedef enum configlib_export_mode {
    CONFIGLIB_EXPORT_EFFECTIVE = 0,
    CONFIGLIB_EXPORT_EFFECTIVE_REDACTED = 1,
    CONFIGLIB_EXPORT_CHANGED_ONLY = 2,
    CONFIGLIB_EXPORT_CHANGED_ONLY_REDACTED = 3,
    CONFIGLIB_EXPORT_RUNTIME_CHANGES_ONLY = 4,
    CONFIGLIB_EXPORT_RUNTIME_CHANGES_ONLY_REDACTED = 5
} configlib_export_mode;

int configlib_version_major(void);
int configlib_version_minor(void);
int configlib_version_patch(void);
const char* configlib_status_name(configlib_status status);
const char* configlib_value_type_name(configlib_value_type type);
const char* configlib_source_kind_name(configlib_source_kind kind);
const char* configlib_export_mode_name(configlib_export_mode mode);

/** Creates a new empty configlib context. Destroy with configlib_destroy(). */
configlib_ctx* configlib_create(void);
/** Destroys a context created by configlib_create(). Accepts null. */
void configlib_destroy(configlib_ctx* ctx);
void configlib_clear(configlib_ctx* ctx);

configlib_status configlib_set_source_precedence(configlib_ctx* ctx, configlib_source_kind kind, int precedence);
configlib_status configlib_require(configlib_ctx* ctx, const char* key, configlib_value_type type);
configlib_status configlib_optional(configlib_ctx* ctx, const char* key, configlib_value_type type);
configlib_status configlib_allowed_string(configlib_ctx* ctx, const char* key, const char* value);
configlib_status configlib_int_range(configlib_ctx* ctx, const char* key, int64_t min_value, int64_t max_value);
configlib_status configlib_default_string(configlib_ctx* ctx, const char* key, const char* value);
configlib_status configlib_default_int(configlib_ctx* ctx, const char* key, int64_t value);
configlib_status configlib_default_bool(configlib_ctx* ctx, const char* key, int value);
configlib_status configlib_default_double(configlib_ctx* ctx, const char* key, double value);

configlib_status configlib_add_string(configlib_ctx* ctx, const char* key, const char* value, configlib_source_kind kind, const char* source_name);
configlib_status configlib_add_int(configlib_ctx* ctx, const char* key, int64_t value, configlib_source_kind kind, const char* source_name);
configlib_status configlib_add_bool(configlib_ctx* ctx, const char* key, int value, configlib_source_kind kind, const char* source_name);
configlib_status configlib_add_double(configlib_ctx* ctx, const char* key, double value, configlib_source_kind kind, const char* source_name);

/** Resolves facts in the context into a result. On success, out_result owns a valid handle. */
configlib_status configlib_resolve(configlib_ctx* ctx, configlib_result** out_result);
/** Destroys a result returned by configlib_resolve(). Accepts null. */
void configlib_result_destroy(configlib_result* result);
int configlib_result_ok(const configlib_result* result);
int configlib_result_contains(const configlib_result* result, const char* key);
configlib_status configlib_result_get_string(const configlib_result* result, const char* key, const char** out_value);
configlib_status configlib_result_get_int(const configlib_result* result, const char* key, int64_t* out_value);
configlib_status configlib_result_get_bool(const configlib_result* result, const char* key, int* out_value);
configlib_status configlib_result_get_double(const configlib_result* result, const char* key, double* out_value);
/** Returns a borrowed diagnostics string valid until the result is destroyed or queried again. */
const char* configlib_result_diagnostics(const configlib_result* result);
/** Returns a borrowed explanation string valid until the result is destroyed or queried again. */
const char* configlib_result_explain(const configlib_result* result, const char* key);

configlib_status configlib_internal_default_string(configlib_ctx* ctx, const char* key, const char* value);
configlib_status configlib_internal_default_int(configlib_ctx* ctx, const char* key, int64_t value);
configlib_status configlib_internal_default_bool(configlib_ctx* ctx, const char* key, int value);
configlib_status configlib_internal_default_double(configlib_ctx* ctx, const char* key, double value);
configlib_status configlib_load_env_mapping(configlib_ctx* ctx, const char* variable, const char* key, configlib_value_type type);
configlib_status configlib_load_cli_args(configlib_ctx* ctx, int argc, const char* const argv[], const char* option, const char* key, configlib_value_type type);
configlib_status configlib_load_file(configlib_ctx* ctx, const char* path);

configlib_schema* configlib_schema_create(void);
void configlib_schema_destroy(configlib_schema* schema);
configlib_status configlib_schema_require(configlib_schema* schema, const char* key, configlib_value_type type);
configlib_status configlib_schema_optional(configlib_schema* schema, const char* key, configlib_value_type type);
configlib_status configlib_schema_allowed_string(configlib_schema* schema, const char* key, const char* value);
configlib_status configlib_schema_int_range(configlib_schema* schema, const char* key, int64_t min_value, int64_t max_value);
configlib_status configlib_schema_double_range(configlib_schema* schema, const char* key, double min_value, double max_value);
configlib_status configlib_schema_documented_default_string(configlib_schema* schema, const char* key, const char* value);
configlib_status configlib_schema_documented_default_int(configlib_schema* schema, const char* key, int64_t value);
configlib_status configlib_schema_documented_default_bool(configlib_schema* schema, const char* key, int value);
configlib_status configlib_schema_documented_default_double(configlib_schema* schema, const char* key, double value);
configlib_status configlib_schema_describe(configlib_schema* schema, const char* key, const char* description);
configlib_status configlib_schema_validate_result(const configlib_schema* schema, const configlib_result* result, configlib_schema_result** out_result);
configlib_status configlib_schema_validate_store(const configlib_schema* schema, const configlib_store* store, configlib_schema_result** out_result);
configlib_status configlib_schema_validate_view(const configlib_schema* schema, const configlib_view* view, configlib_schema_result** out_result);
void configlib_schema_result_destroy(configlib_schema_result* result);
int configlib_schema_result_ok(const configlib_schema_result* result);
const char* configlib_schema_result_diagnostics(const configlib_schema_result* result);

configlib_access_policy* configlib_access_policy_create(void);
void configlib_access_policy_destroy(configlib_access_policy* access);
configlib_status configlib_access_runtime_mutable(configlib_access_policy* access, const char* key, int value);
configlib_status configlib_access_exportable(configlib_access_policy* access, const char* key, int value);
configlib_status configlib_access_secret(configlib_access_policy* access, const char* key, int value);
configlib_status configlib_access_resettable(configlib_access_policy* access, const char* key, int value);

configlib_status configlib_store_from_result(const configlib_ctx* ctx, const configlib_result* result, const configlib_access_policy* access, configlib_store** out_store);
void configlib_store_destroy(configlib_store* store);
int configlib_store_contains(const configlib_store* store, const char* key);
configlib_status configlib_store_get_string(const configlib_store* store, const char* key, const char** out_value);
configlib_status configlib_store_get_int(const configlib_store* store, const char* key, int64_t* out_value);
configlib_status configlib_store_get_bool(const configlib_store* store, const char* key, int* out_value);
configlib_status configlib_store_get_double(const configlib_store* store, const char* key, double* out_value);
int configlib_store_has_runtime_change(const configlib_store* store, const char* key);
const char* configlib_store_explain(const configlib_store* store, const char* key);
const char* configlib_store_export(const configlib_store* store, configlib_export_mode mode);
const char* configlib_store_diagnostics(const configlib_store* store);
configlib_status configlib_store_view(const configlib_store* store, const char* prefix, configlib_view** out_view);
configlib_status configlib_store_begin_transaction(configlib_store* store, configlib_transaction** out_tx);

void configlib_view_destroy(configlib_view* view);
int configlib_view_contains(const configlib_view* view, const char* local_key);
configlib_status configlib_view_get_string(const configlib_view* view, const char* local_key, const char** out_value);
configlib_status configlib_view_get_int(const configlib_view* view, const char* local_key, int64_t* out_value);
configlib_status configlib_view_get_bool(const configlib_view* view, const char* local_key, int* out_value);
configlib_status configlib_view_get_double(const configlib_view* view, const char* local_key, double* out_value);
const char* configlib_view_explain(const configlib_view* view, const char* local_key);
const char* configlib_view_export(const configlib_view* view, configlib_export_mode mode);
const char* configlib_view_export_local(const configlib_view* view, configlib_export_mode mode);

void configlib_transaction_destroy(configlib_transaction* tx);
configlib_status configlib_transaction_set_string(configlib_transaction* tx, const char* key, const char* value);
configlib_status configlib_transaction_set_int(configlib_transaction* tx, const char* key, int64_t value);
configlib_status configlib_transaction_set_bool(configlib_transaction* tx, const char* key, int value);
configlib_status configlib_transaction_set_double(configlib_transaction* tx, const char* key, double value);
configlib_status configlib_transaction_erase(configlib_transaction* tx, const char* key);
configlib_status configlib_transaction_reset_to_base(configlib_transaction* tx, const char* key);
configlib_status configlib_transaction_reset_to_default(configlib_transaction* tx, const char* key);
int configlib_transaction_validate(const configlib_transaction* tx);
const char* configlib_transaction_diagnostics(const configlib_transaction* tx);
int configlib_transaction_commit(configlib_transaction* tx);
void configlib_transaction_rollback(configlib_transaction* tx);

#ifdef __cplusplus
}
#endif
