// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

static void append(configlib::FactSet& target, const configlib::FactSet& source) {
    for (const auto& fact : source.all()) target.add(fact.key, fact.value, fact.source, fact.precedence, fact.role);
}

int main() {
    using namespace configlib;

    const auto tmpdir = std::filesystem::temp_directory_path() / "configlib_v0_3_tests";
    std::filesystem::create_directories(tmpdir);
    const auto config_path = tmpdir / "app.conf";
    const auto missing_path = tmpdir / "missing.conf";

    {
        std::ofstream out(config_path);
        out << "# comment\n";
        out << "[logging]\n";
        out << "level = debug\n";
        out << "[server]\n";
        out << "port = 4444\n";
        out << "feature.enabled = true\n";
    }

    PolicySet policies;
    policies.require(KeyPath("logging.level"), ValueType::String);
    policies.allowed_strings(KeyPath("logging.level"), {"trace", "debug", "info", "warn", "error"});
    policies.require(KeyPath("server.port"), ValueType::Int);
    policies.int_range(KeyPath("server.port"), 1, 65535);
    policies.optional(KeyPath("feature.enabled"), ValueType::Bool);

    auto file_report = load_config_file(config_path.string(), policies);
    REQUIRE(file_report.ok());
    REQUIRE(file_report.facts.all().size() == 3);

    auto result = resolve(file_report.facts, policies);
    REQUIRE(result.ok());
    REQUIRE(result.config().get_string(KeyPath("logging.level")) == "debug");
    REQUIRE(result.config().get_int(KeyPath("server.port")) == 4444);
    REQUIRE(result.config().get_bool(KeyPath("feature.enabled")) == true);

    FileDiscoveryPolicy discovery;
    discovery.enabled()
             .search_path(missing_path.string())
             .search_path(config_path.string())
             .when_none_found(AbsenceAction::Error);
    auto discovered = discover_config_files(discovery, policies);
    REQUIRE(discovered.ok());
    REQUIRE(discovered.files.size() == 1);
    REQUIRE(discovered.load.facts.all().size() == 3);

    InternalDefaultsProvider defaults;
    defaults.set_string(KeyPath("logging.level"), "info")
            .set_int(KeyPath("server.port"), 8080);
    FileDiscoveryPolicy fallback;
    fallback.enabled()
            .search_path(missing_path.string())
            .when_none_found(AbsenceAction::UseInternalDefaults)
            .internal_defaults(defaults);
    auto fallback_report = discover_config_files(fallback, policies);
    REQUIRE(fallback_report.ok());
    REQUIRE(fallback_report.files.empty());
    REQUIRE(fallback_report.load.facts.all().size() == 2);
    auto fallback_result = resolve(fallback_report.load.facts, policies);
    REQUIRE(fallback_result.ok());
    REQUIRE(fallback_result.config().get_string(KeyPath("logging.level")) == "info");
    REQUIRE(fallback_result.config().get_int(KeyPath("server.port")) == 8080);

    auto bad_report = load_config_file((tmpdir / "does-not-exist.conf").string(), policies);
    REQUIRE(!bad_report.ok());

    std::filesystem::remove_all(tmpdir);
    return EXIT_SUCCESS;
}
