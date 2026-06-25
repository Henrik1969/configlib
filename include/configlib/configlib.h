// SPDX-License-Identifier: MIT

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIGLIB_C_VERSION_MAJOR 0
#define CONFIGLIB_C_VERSION_MINOR 2
#define CONFIGLIB_C_VERSION_PATCH 0

typedef struct configlib_ctx configlib_ctx;
typedef struct configlib_result configlib_result;

typedef enum configlib_status {
    CONFIGLIB_OK = 0,
    CONFIGLIB_ERROR = 1,
    CONFIGLIB_INVALID_ARGUMENT = 2,
    CONFIGLIB_NOT_FOUND = 3
} configlib_status;

typedef enum configlib_value_type {
    CONFIGLIB_VALUE_NULL = 0,
    CONFIGLIB_VALUE_BOOL = 1,
    CONFIGLIB_VALUE_INT = 2,
    CONFIGLIB_VALUE_DOUBLE = 3,
    CONFIGLIB_VALUE_STRING = 4
} configlib_value_type;

typedef enum configlib_source_kind {
    CONFIGLIB_SOURCE_INTERNAL_DEFAULT = 0,
    CONFIGLIB_SOURCE_FILE = 1,
    CONFIGLIB_SOURCE_ENVIRONMENT = 2,
    CONFIGLIB_SOURCE_CLI = 3,
    CONFIGLIB_SOURCE_RUNTIME = 4,
    CONFIGLIB_SOURCE_UNKNOWN = 5
} configlib_source_kind;

configlib_ctx* configlib_create(void);
void configlib_destroy(configlib_ctx* ctx);
void configlib_clear(configlib_ctx* ctx);

configlib_status configlib_set_source_precedence(configlib_ctx* ctx, configlib_source_kind kind, int precedence);
configlib_status configlib_require(configlib_ctx* ctx, const char* key, configlib_value_type type);
configlib_status configlib_optional(configlib_ctx* ctx, const char* key, configlib_value_type type);
configlib_status configlib_default_string(configlib_ctx* ctx, const char* key, const char* value);
configlib_status configlib_default_int(configlib_ctx* ctx, const char* key, int64_t value);
configlib_status configlib_default_bool(configlib_ctx* ctx, const char* key, int value);

configlib_status configlib_add_string(configlib_ctx* ctx, const char* key, const char* value, configlib_source_kind kind, const char* source_name);
configlib_status configlib_add_int(configlib_ctx* ctx, const char* key, int64_t value, configlib_source_kind kind, const char* source_name);
configlib_status configlib_add_bool(configlib_ctx* ctx, const char* key, int value, configlib_source_kind kind, const char* source_name);

configlib_status configlib_resolve(configlib_ctx* ctx, configlib_result** out_result);
void configlib_result_destroy(configlib_result* result);
int configlib_result_ok(const configlib_result* result);

configlib_status configlib_result_get_string(const configlib_result* result, const char* key, const char** out_value);
configlib_status configlib_result_get_int(const configlib_result* result, const char* key, int64_t* out_value);
configlib_status configlib_result_get_bool(const configlib_result* result, const char* key, int* out_value);
const char* configlib_result_diagnostics(const configlib_result* result);
const char* configlib_result_explain(const configlib_result* result, const char* key);

configlib_status configlib_internal_default_string(configlib_ctx* ctx, const char* key, const char* value);
configlib_status configlib_internal_default_int(configlib_ctx* ctx, const char* key, int64_t value);
configlib_status configlib_internal_default_bool(configlib_ctx* ctx, const char* key, int value);
configlib_status configlib_load_env_mapping(configlib_ctx* ctx, const char* variable, const char* key, configlib_value_type type);
configlib_status configlib_load_cli_args(configlib_ctx* ctx, int argc, const char* const argv[], const char* option, const char* key, configlib_value_type type);

#ifdef __cplusplus
}
#endif
