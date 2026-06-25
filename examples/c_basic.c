#include <configlib/configlib.h>

#include <stdint.h>
#include <stdio.h>

int main(void) {
    configlib_ctx* ctx = configlib_create();
    if (!ctx) return 1;

    configlib_default_string(ctx, "logging.level", "info");
    configlib_add_string(ctx, "logging.level", "warn", CONFIGLIB_SOURCE_FILE, "./app.conf");
    configlib_add_string(ctx, "logging.level", "debug", CONFIGLIB_SOURCE_ENVIRONMENT, "MYAPP_LOGGING_LEVEL");
    configlib_add_string(ctx, "logging.level", "trace", CONFIGLIB_SOURCE_CLI, "--log-level");
    configlib_require(ctx, "logging.level", CONFIGLIB_VALUE_STRING);

    configlib_result* result = NULL;
    if (configlib_resolve(ctx, &result) != CONFIGLIB_OK || !result) {
        configlib_destroy(ctx);
        return 1;
    }

    if (!configlib_result_ok(result)) {
        fprintf(stderr, "%s", configlib_result_diagnostics(result));
        configlib_result_destroy(result);
        configlib_destroy(ctx);
        return 1;
    }

    const char* level = NULL;
    if (configlib_result_get_string(result, "logging.level", &level) == CONFIGLIB_OK) {
        printf("logging.level = %s\n", level);
    }
    printf("\nExplanation:\n%s", configlib_result_explain(result, "logging.level"));

    configlib_result_destroy(result);
    configlib_destroy(ctx);
    return 0;
}
