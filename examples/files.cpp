// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <fstream>
#include <iostream>

static void append(configlib::FactSet& target, const configlib::FactSet& source) {
    for (const auto& fact : source.all()) target.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
}

int main() {
    using namespace configlib;

    const char* path = "./examples/example.conf";
    {
        std::ofstream out(path);
        out << "# configlib v0.3 key=value demo\n";
        out << "[logging]\n";
        out << "level = warn\n";
        out << "[server]\n";
        out << "port = 9090\n";
    }

    PolicySet policies;
    policies.require(KeyPath("logging.level"), ValueType::String);
    policies.allowed_strings(KeyPath("logging.level"), {"trace", "debug", "info", "warn", "error"});
    policies.require(KeyPath("server.port"), ValueType::Int);
    policies.int_range(KeyPath("server.port"), 1, 65535);

    InternalDefaultsProvider fallback;
    fallback.set_string(KeyPath("logging.level"), "info")
            .set_int(KeyPath("server.port"), 8080);

    FileDiscoveryPolicy files;
    files.enabled()
         .search_path(path)
         .search_path("./missing.conf")
         .when_none_found(AbsenceAction::UseInternalDefaults)
         .internal_defaults(fallback);

    auto discovery = discover_config_files(files, policies);
    if (!discovery.ok()) {
        std::cerr << discovery.load.diagnostics.format();
        return 1;
    }

    FactSet facts;
    append(facts, discovery.load.facts);

    auto result = resolve(facts, policies);
    if (!result.ok()) {
        std::cerr << result.diagnostics().format();
        return 1;
    }

    std::cout << "logging.level=" << result.config().get_string_or(KeyPath("logging.level"), "") << '\n';
    std::cout << "server.port=" << result.config().get_integer_or(KeyPath("server.port"), 0) << '\n';
    std::cout << result.config().format_explanation(KeyPath("server.port"));
}
