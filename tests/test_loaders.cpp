// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

static void append(configlib::FactSet& target, const configlib::FactSet& source) {
    for (const auto& fact : source.all()) target.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
}

int main() {
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

    auto defaults_report = load_internal_defaults(defaults, policies);
    REQUIRE(defaults_report.ok());
    REQUIRE(defaults_report.facts.all().size() == 3);

    EnvironmentLoaderPolicy env_policy;
    env_policy.enabled()
              .prefix("MYAPP_")
              .mapping_style(EnvMappingStyle::PrefixToDottedLowercase)
              .map("MYAPP_LOG", KeyPath("logging.level"), ValueType::String);

    std::map<std::string, std::string> env{
        {"MYAPP_LOG", "debug"},
        {"MYAPP_SERVER_PORT", "9000"}
    };
    auto env_report = load_environment(env_policy, policies, env);
    REQUIRE(env_report.ok());
    REQUIRE(env_report.facts.all().size() == 2);

    CliLoaderPolicy cli_policy;
    cli_policy.enabled()
              .option("--log-level", KeyPath("logging.level"), ValueType::String)
              .option("--port", KeyPath("server.port"), ValueType::Int)
              .flag("--feature", KeyPath("feature.enabled"));

    std::vector<std::string> args{"prog", "--log-level=trace", "--port", "10001", "--feature"};
    auto cli_report = load_cli(cli_policy, policies, args);
    REQUIRE(cli_report.ok());
    REQUIRE(cli_report.facts.all().size() == 3);

    FactSet facts;
    append(facts, defaults_report.facts);
    append(facts, env_report.facts);
    append(facts, cli_report.facts);

    auto result = resolve(facts, policies);
    REQUIRE(result.ok());
    REQUIRE(result.config().get_string(KeyPath("logging.level")) == "trace");
    REQUIRE(result.config().get_int(KeyPath("server.port")) == 10001);
    REQUIRE(result.config().get_bool(KeyPath("feature.enabled")) == true);

    auto bad_env = load_environment(env_policy, policies, {{"MYAPP_SERVER_PORT", "not-int"}});
    REQUIRE(!bad_env.ok());

    return EXIT_SUCCESS;
}
