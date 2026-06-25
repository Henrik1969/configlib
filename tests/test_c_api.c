// SPDX-License-Identifier: MIT

#include <configlib/configlib.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(expr) do { if (!(expr)) { fprintf(stderr, "FAILED: %s at %s:%d\n", #expr, __FILE__, __LINE__); return EXIT_FAILURE; } } while (0)

int main(void) {
    configlib_ctx* ctx = configlib_create();
    REQUIRE(ctx != NULL);

    REQUIRE(configlib_default_string(ctx, "logging.level", "info") == CONFIGLIB_OK);
    REQUIRE(configlib_add_string(ctx, "logging.level", "debug", CONFIGLIB_SOURCE_ENVIRONMENT, "MYAPP_LOGGING_LEVEL") == CONFIGLIB_OK);
    REQUIRE(configlib_add_string(ctx, "logging.level", "trace", CONFIGLIB_SOURCE_CLI, "--log-level") == CONFIGLIB_OK);
    REQUIRE(configlib_require(ctx, "logging.level", CONFIGLIB_VALUE_STRING) == CONFIGLIB_OK);

    configlib_result* result = NULL;
    REQUIRE(configlib_resolve(ctx, &result) == CONFIGLIB_OK);
    REQUIRE(result != NULL);
    REQUIRE(configlib_result_ok(result) == 1);

    const char* level = NULL;
    REQUIRE(configlib_result_get_string(result, "logging.level", &level) == CONFIGLIB_OK);
    REQUIRE(level != NULL);
    REQUIRE(strcmp(level, "trace") == 0);

    configlib_result_destroy(result);
    configlib_destroy(ctx);
    return EXIT_SUCCESS;
}
