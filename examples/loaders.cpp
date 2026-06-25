// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <iostream>
#include <vector>

static void append(configlib::FactSet& target, const configlib::FactSet& source) {
    for (const auto& fact : source.all()) target.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
}

int main(int argc, const char* const argv[]) {
    using namespace configlib;

    PolicySet policies;
    policies.require(KeyPath("logging.level"), ValueType::String);
    policies.allowed_strings(KeyPath("logging.level"), {"trace", "debug", "info", "warn", "error"});
    policies.require(KeyPath("server.port"), ValueType::Int);
    policies.int_range(KeyPath("server.port"), 1, 65535);
    policies.optional(KeyPath("feature.enabled"), ValueType::Bool);

    InternalDefaultsProvider defaults;
    defaults.set_string(KeyPath("logging.level"), "info")
            .set_int(KeyPath("server.port"), 8080)
            .set_bool(KeyPath("feature.enabled"), false);

    EnvironmentLoaderPolicy env;
    env.enabled()
       .prefix("MYAPP_")
       .mapping_style(EnvMappingStyle::PrefixToDottedLowercase)
       .map("MYAPP_LOG_LEVEL", KeyPath("logging.level"), ValueType::String);

    CliLoaderPolicy cli;
    cli.enabled()
       .option("--log-level", KeyPath("logging.level"), ValueType::String)
       .option("--port", KeyPath("server.port"), ValueType::Int)
       .flag("--feature", KeyPath("feature.enabled"));

    FactSet facts;
    auto defaults_report = load_internal_defaults(defaults, policies);
    auto env_report = load_environment(env, policies);
    auto cli_report = load_cli(cli, policies, argc, argv);

    append(facts, defaults_report.facts);
    append(facts, env_report.facts);
    append(facts, cli_report.facts);

    auto result = resolve(facts, policies);
    if (!defaults_report.ok()) std::cerr << defaults_report.diagnostics.format();
    if (!env_report.ok()) std::cerr << env_report.diagnostics.format();
    if (!cli_report.ok()) std::cerr << cli_report.diagnostics.format();
    if (!result.ok()) {
        std::cerr << result.diagnostics().format();
        return 1;
    }

    std::cout << "logging.level = " << result.config().get_string(KeyPath("logging.level")) << '\n';
    std::cout << "server.port = " << result.config().get_int(KeyPath("server.port")) << '\n';
    std::cout << "feature.enabled = " << (result.config().get_bool(KeyPath("feature.enabled")) ? "true" : "false") << '\n';
    std::cout << "\n" << result.config().format_explanation(KeyPath("logging.level"));
}
