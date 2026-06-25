// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

struct LoggingConfig {
    std::string level;
    bool color{false};
    std::string file;
};

struct ServerConfig {
    int port{0};
    std::int64_t max_connections{0};
    double timeout_seconds{0.0};
};

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("xyz.logging.level"), Value("info"));
    facts.add_default(KeyPath("xyz.logging.color"), Value(true));
    facts.add_default(KeyPath("xyz.server.port"), Value(8080));
    facts.add_default(KeyPath("xyz.server.max_connections"), Value(1000));
    facts.add_default(KeyPath("xyz.server.timeout_seconds"), Value(3.5));
    facts.add_cli(KeyPath("xyz.logging.level"), Value("debug"), "--log-level");

    PolicySet policies;
    policies.optional(KeyPath("xyz.logging.level"), ValueType::String);
    policies.optional(KeyPath("xyz.logging.color"), ValueType::Bool);
    policies.optional(KeyPath("xyz.server.port"), ValueType::Int);
    policies.optional(KeyPath("xyz.server.max_connections"), ValueType::Int);
    policies.optional(KeyPath("xyz.server.timeout_seconds"), ValueType::Double);

    auto result = resolve(facts, policies);
    REQUIRE(result.ok());
    auto store = ConfigStore::from_result(std::move(result), policies);

    StructBinding<LoggingConfig> logging_binding("LoggingConfig");
    logging_binding
        .required_string(KeyPath("level"), &LoggingConfig::level)
        .boolean(KeyPath("color"), &LoggingConfig::color, false)
        .string(KeyPath("file"), &LoggingConfig::file, "stderr");

    auto logging_result = logging_binding.read(store.view(KeyPath("xyz.logging")));
    REQUIRE(logging_result.ok());
    REQUIRE(logging_result.value().level == "debug");
    REQUIRE(logging_result.value().color == true);
    REQUIRE(logging_result.value().file == "stderr");

    StructBinding<ServerConfig> server_binding("ServerConfig");
    server_binding
        .integer(KeyPath("port"), &ServerConfig::port, 1)
        .integer(KeyPath("max_connections"), &ServerConfig::max_connections, 10)
        .floating(KeyPath("timeout_seconds"), &ServerConfig::timeout_seconds, 1.0);

    auto server_result = server_binding.read(store.view(KeyPath("xyz.server")));
    REQUIRE(server_result.ok());
    REQUIRE(server_result.value().port == 8080);
    REQUIRE(server_result.value().max_connections == 1000);
    REQUIRE(server_result.value().timeout_seconds == 3.5);

    StructBinding<LoggingConfig> missing_required("MissingRequired");
    missing_required.required_string(KeyPath("missing"), &LoggingConfig::level);
    auto missing_result = missing_required.read(store.view(KeyPath("xyz.logging")));
    REQUIRE(!missing_result.ok());
    REQUIRE(missing_result.diagnostics().format().find("BINDING_REQUIRED_MISSING") != std::string::npos);

    StructBinding<LoggingConfig> mismatch("Mismatch");
    mismatch.required_string(KeyPath("color"), &LoggingConfig::level);
    auto mismatch_result = mismatch.read(store.view(KeyPath("xyz.logging")));
    REQUIRE(!mismatch_result.ok());
    REQUIRE(mismatch_result.diagnostics().format().find("BINDING_TYPE_MISMATCH") != std::string::npos);

    return EXIT_SUCCESS;
}
