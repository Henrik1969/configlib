// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <iostream>
#include <string>

struct LoggingConfig {
    std::string level;
    bool color{true};
    std::string file;
};

struct ServerConfig {
    int port{0};
    double timeout_seconds{0.0};
};

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("xyz.logging.level"), Value("info"));
    facts.add_default(KeyPath("xyz.logging.color"), Value(true));
    facts.add_default(KeyPath("xyz.logging.file"), Value("/tmp/xyz.log"));
    facts.add_default(KeyPath("xyz.server.port"), Value(8080));
    facts.add_default(KeyPath("xyz.server.timeout_seconds"), Value(2.5));
    facts.add_cli(KeyPath("xyz.logging.level"), Value("debug"), "--log-level");

    PolicySet policies;
    policies.optional(KeyPath("xyz.logging.level"), ValueType::String);
    policies.optional(KeyPath("xyz.logging.color"), ValueType::Bool);
    policies.optional(KeyPath("xyz.logging.file"), ValueType::String);
    policies.optional(KeyPath("xyz.server.port"), ValueType::Int);
    policies.optional(KeyPath("xyz.server.timeout_seconds"), ValueType::Double);

    auto resolved = resolve(facts, policies);
    if (!resolved.ok()) {
        std::cerr << resolved.diagnostics().format();
        return 1;
    }

    auto store = ConfigStore::from_result(std::move(resolved), policies);

    StructBinding<LoggingConfig> logging_binding("LoggingConfig");
    logging_binding
        .string(KeyPath("level"), &LoggingConfig::level, "info")
        .boolean(KeyPath("color"), &LoggingConfig::color, true)
        .string(KeyPath("file"), &LoggingConfig::file, "stderr");

    StructBinding<ServerConfig> server_binding("ServerConfig");
    server_binding
        .integer(KeyPath("port"), &ServerConfig::port, 8080)
        .floating(KeyPath("timeout_seconds"), &ServerConfig::timeout_seconds, 1.0);

    auto logging_result = logging_binding.read(store.view(KeyPath("xyz.logging")));
    auto server_result = server_binding.read(store.view(KeyPath("xyz.server")));

    if (!logging_result.ok() || !server_result.ok()) {
        std::cerr << logging_result.diagnostics().format();
        std::cerr << server_result.diagnostics().format();
        return 1;
    }

    const auto& logging = logging_result.value();
    const auto& server = server_result.value();

    std::cout << "logging.level = " << logging.level << '\n';
    std::cout << "logging.color = " << (logging.color ? "true" : "false") << '\n';
    std::cout << "logging.file = " << logging.file << '\n';
    std::cout << "server.port = " << server.port << '\n';
    std::cout << "server.timeout_seconds = " << server.timeout_seconds << '\n';

    return 0;
}
