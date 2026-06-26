// SPDX-License-Identifier: MIT

#include <configlib/configlib.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(expr) do { if (!(expr)) { fprintf(stderr, "FAILED: %s at %s:%d\n", #expr, __FILE__, __LINE__); return EXIT_FAILURE; } } while (0)
#define REQUIRE_OK(expr) do { configlib_status st__ = (expr); if (st__ != CONFIGLIB_OK) { fprintf(stderr, "FAILED: %s returned %s at %s:%d\n", #expr, configlib_status_name(st__), __FILE__, __LINE__); return EXIT_FAILURE; } } while (0)

int main(void) {
    REQUIRE(configlib_version_major() == 0);
    REQUIRE(configlib_version_minor() == 9);
    REQUIRE(configlib_version_patch() == 1);

    configlib_ctx* ctx = configlib_create();
    REQUIRE(ctx != NULL);

    REQUIRE_OK(configlib_internal_default_string(ctx, "logging.level", "info"));
    REQUIRE_OK(configlib_add_string(ctx, "logging.level", "debug", CONFIGLIB_SOURCE_ENVIRONMENT, "MYAPP_LOGGING_LEVEL"));
    REQUIRE_OK(configlib_add_string(ctx, "logging.level", "trace", CONFIGLIB_SOURCE_CLI, "--log-level"));
    REQUIRE_OK(configlib_require(ctx, "logging.level", CONFIGLIB_VALUE_STRING));
    REQUIRE_OK(configlib_allowed_string(ctx, "logging.level", "trace"));
    REQUIRE_OK(configlib_allowed_string(ctx, "logging.level", "debug"));
    REQUIRE_OK(configlib_allowed_string(ctx, "logging.level", "info"));
    REQUIRE_OK(configlib_add_int(ctx, "server.port", 8080, CONFIGLIB_SOURCE_FILE, "app.conf"));
    REQUIRE_OK(configlib_require(ctx, "server.port", CONFIGLIB_VALUE_INT));
    REQUIRE_OK(configlib_int_range(ctx, "server.port", 1, 65535));
    REQUIRE_OK(configlib_add_bool(ctx, "feature.enabled", 1, CONFIGLIB_SOURCE_FILE, "app.conf"));
    REQUIRE_OK(configlib_add_double(ctx, "limits.ratio", 0.75, CONFIGLIB_SOURCE_FILE, "app.conf"));
    REQUIRE_OK(configlib_require(ctx, "limits.ratio", CONFIGLIB_VALUE_DOUBLE));
    REQUIRE_OK(configlib_add_string(ctx, "secret.token", "swordfish", CONFIGLIB_SOURCE_FILE, "secret.conf"));

    configlib_result* result = NULL;
    REQUIRE_OK(configlib_resolve(ctx, &result));
    REQUIRE(result != NULL);
    REQUIRE(configlib_result_ok(result) == 1);
    REQUIRE(configlib_result_contains(result, "logging.level") == 1);

    const char* level = NULL;
    REQUIRE_OK(configlib_result_get_string(result, "logging.level", &level));
    REQUIRE(level != NULL);
    REQUIRE(strcmp(level, "trace") == 0);

    int64_t port = 0;
    REQUIRE_OK(configlib_result_get_int(result, "server.port", &port));
    REQUIRE(port == 8080);

    double ratio = 0.0;
    REQUIRE_OK(configlib_result_get_double(result, "limits.ratio", &ratio));
    REQUIRE(ratio > 0.74 && ratio < 0.76);

    configlib_schema* schema = configlib_schema_create();
    REQUIRE(schema != NULL);
    REQUIRE_OK(configlib_schema_require(schema, "logging.level", CONFIGLIB_VALUE_STRING));
    REQUIRE_OK(configlib_schema_allowed_string(schema, "logging.level", "trace"));
    REQUIRE_OK(configlib_schema_allowed_string(schema, "logging.level", "debug"));
    REQUIRE_OK(configlib_schema_allowed_string(schema, "logging.level", "info"));
    REQUIRE_OK(configlib_schema_require(schema, "server.port", CONFIGLIB_VALUE_INT));
    REQUIRE_OK(configlib_schema_int_range(schema, "server.port", 1, 65535));
    REQUIRE_OK(configlib_schema_require(schema, "limits.ratio", CONFIGLIB_VALUE_DOUBLE));
    REQUIRE_OK(configlib_schema_double_range(schema, "limits.ratio", 0.0, 1.0));

    configlib_schema_result* checked = NULL;
    REQUIRE_OK(configlib_schema_validate_result(schema, result, &checked));
    REQUIRE(checked != NULL);
    REQUIRE(configlib_schema_result_ok(checked) == 1);

    configlib_access_policy* access = configlib_access_policy_create();
    REQUIRE(access != NULL);
    REQUIRE_OK(configlib_access_runtime_mutable(access, "server.port", 0));
    REQUIRE_OK(configlib_access_secret(access, "secret.token", 1));

    configlib_store* store = NULL;
    REQUIRE_OK(configlib_store_from_result(ctx, result, access, &store));
    REQUIRE(store != NULL);
    REQUIRE(configlib_store_contains(store, "logging.level") == 1);
    REQUIRE_OK(configlib_store_get_string(store, "logging.level", &level));
    REQUIRE(strcmp(level, "trace") == 0);

    const char* exported = configlib_store_export(store, CONFIGLIB_EXPORT_EFFECTIVE_REDACTED);
    REQUIRE(exported != NULL);
    REQUIRE(strstr(exported, "logging.level = trace") != NULL);
    REQUIRE(strstr(exported, "secret.token = <redacted>") != NULL);

    configlib_transaction* tx = NULL;
    REQUIRE_OK(configlib_store_begin_transaction(store, &tx));
    REQUIRE(tx != NULL);
    REQUIRE_OK(configlib_transaction_set_string(tx, "logging.level", "debug"));
    REQUIRE(configlib_transaction_commit(tx) == 1);
    configlib_transaction_destroy(tx);
    tx = NULL;

    REQUIRE(configlib_store_has_runtime_change(store, "logging.level") == 1);
    REQUIRE_OK(configlib_store_get_string(store, "logging.level", &level));
    REQUIRE(strcmp(level, "debug") == 0);

    REQUIRE_OK(configlib_store_begin_transaction(store, &tx));
    REQUIRE_OK(configlib_transaction_set_int(tx, "server.port", 9090));
    REQUIRE(configlib_transaction_validate(tx) == 0);
    REQUIRE(strstr(configlib_transaction_diagnostics(tx), "CONFIG_STORE_MUTATION_DENIED") != NULL);
    REQUIRE(configlib_transaction_commit(tx) == 0);
    configlib_transaction_destroy(tx);

    configlib_view* view = NULL;
    REQUIRE_OK(configlib_store_view(store, "logging", &view));
    REQUIRE(view != NULL);
    REQUIRE(configlib_view_contains(view, "level") == 1);
    REQUIRE_OK(configlib_view_get_string(view, "level", &level));
    REQUIRE(strcmp(level, "debug") == 0);
    REQUIRE(strstr(configlib_view_export_local(view, CONFIGLIB_EXPORT_EFFECTIVE), "level = debug") != NULL);

    configlib_schema_result_destroy(checked);
    configlib_view_destroy(view);
    configlib_store_destroy(store);
    configlib_access_policy_destroy(access);
    configlib_schema_destroy(schema);
    configlib_result_destroy(result);
    configlib_destroy(ctx);
    return EXIT_SUCCESS;
}
