// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

int main() {
    using namespace configlib;

    FactSet facts;
    facts.add_default(KeyPath("logging.level"), Value("info"));
    facts.add_default(KeyPath("logging.color"), Value(true));
    facts.add_default(KeyPath("logging.file"), Value("/tmp/app.log"));
    facts.add_default(KeyPath("server.port"), Value(8080));
    facts.add_default(KeyPath("auth.token"), Value("secret-token"));
    facts.add_cli(KeyPath("logging.level"), Value("debug"), "--log-level");

    PolicySet policies;
    facts.add_default(KeyPath("logging.level"), Value("info"));
    policies.optional(KeyPath("logging.color"), ValueType::Bool);
    policies.optional(KeyPath("logging.file"), ValueType::String);
    policies.optional(KeyPath("server.port"), ValueType::Int);
    policies.optional(KeyPath("auth.token"), ValueType::String);

    AccessPolicy access;
    access.secret(KeyPath("auth.token"));

    auto result = resolve(facts, policies);
    REQUIRE(result.ok());
    auto store = ConfigStore::from_result(std::move(result), policies, access);

    auto logging = store.view(KeyPath("logging"));
    REQUIRE(logging.prefix_string() == "logging");
    REQUIRE(logging.contains(KeyPath("level")));
    REQUIRE(!logging.contains(KeyPath("server.port")));
    REQUIRE(logging.get_string(KeyPath("level")) == "debug");
    REQUIRE(logging.get_boolean_or(KeyPath("color"), false) == true);
    REQUIRE(logging.get_integer_or(KeyPath("missing"), 42) == 42);
    REQUIRE(logging.get_string_or(KeyPath("missing"), "fallback") == "fallback");

    auto keys = logging.keys();
    REQUIRE(keys.size() == 3);
    REQUIRE(keys[0] == "color");
    REQUIRE(keys[1] == "file");
    REQUIRE(keys[2] == "level");

    const auto explanation = logging.explain(KeyPath("level"));
    REQUIRE(explanation.find("logging.level") != std::string::npos);
    REQUIRE(explanation.find("chosen:") != std::string::npos);

    const auto full_export = logging.export_config();
    REQUIRE(full_export.find("logging.level = debug") != std::string::npos);
    REQUIRE(full_export.find("logging.color = true") != std::string::npos);
    REQUIRE(full_export.find("server.port") == std::string::npos);
    REQUIRE(full_export.find("auth.token") == std::string::npos);

    const auto local_export = logging.export_local_config();
    REQUIRE(local_export.find("level = debug") != std::string::npos);
    REQUIRE(local_export.find("color = true") != std::string::npos);
    REQUIRE(local_export.find("logging.level") == std::string::npos);

    auto tx = store.begin_transaction();
    tx.set(KeyPath("logging.level"), Value("trace"));
    tx.erase(KeyPath("logging.file"));
    REQUIRE(tx.commit());

    REQUIRE(logging.get_string(KeyPath("level")) == "trace");
    REQUIRE(!logging.contains(KeyPath("file")));
    const auto changed = logging.export_config(ExportMode::ChangedOnly);
    REQUIRE(changed.find("logging.level = trace") != std::string::npos);
    REQUIRE(changed.find("logging.file = <erased>") != std::string::npos);
    REQUIRE(changed.find("server.port") == std::string::npos);

    auto root = store.view(KeyPath(""));
    REQUIRE(root.contains(KeyPath("server.port")));
    REQUIRE(root.get_integer_or(KeyPath("server.port"), 0) == 8080);

    return EXIT_SUCCESS;
}
