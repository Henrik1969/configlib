// SPDX-License-Identifier: MIT

#include <configlib/configlib.hpp>

#include <cstdlib>
#include <iostream>

#define REQUIRE(expr) do { if (!(expr)) { std::cerr << "FAILED: " #expr " at " << __FILE__ << ':' << __LINE__ << '\n'; return EXIT_FAILURE; } } while (0)

int main() {
    using namespace configlib;

    {
        FactSet facts;
        facts.add_default(KeyPath("logging.level"), Value("info"));
        facts.add_file(KeyPath("logging.level"), Value("warn"), "./app.conf");
        facts.add_env(KeyPath("logging.level"), Value("debug"), "MYAPP_LOGGING_LEVEL");
        facts.add_cli(KeyPath("logging.level"), Value("trace"), "--log-level");

        PolicySet policy;
        policy.require(KeyPath("logging.level"), ValueType::String);
        auto result = resolve(facts, policy);
        REQUIRE(result.ok());
        REQUIRE(result.config().get_string(KeyPath("logging.level")) == "trace");
    }

    {
        FactSet facts;
        PolicySet policy;
        policy.defaulted(KeyPath("server.host"), Value("127.0.0.1"));
        auto result = resolve(facts, policy);
        REQUIRE(result.ok());
        REQUIRE(result.config().get_string(KeyPath("server.host")) == "127.0.0.1");
    }

    {
        FactSet facts;
        PolicySet policy;
        policy.require(KeyPath("server.port"), ValueType::Int);
        auto result = resolve(facts, policy);
        REQUIRE(!result.ok());
    }

    {
        FactSet facts;
        facts.add_file(KeyPath("server.port"), Value("not-an-int"), "./app.conf");
        PolicySet policy;
        policy.require(KeyPath("server.port"), ValueType::Int);
        auto result = resolve(facts, policy);
        REQUIRE(!result.ok());
    }

    {
        FactSet facts;
        facts.add_file(KeyPath("server.port"), Value(70000), "./app.conf");
        PolicySet policy;
        policy.require(KeyPath("server.port"), ValueType::Int);
        policy.int_range(KeyPath("server.port"), 1, 65535);
        auto result = resolve(facts, policy);
        REQUIRE(!result.ok());
    }

    return EXIT_SUCCESS;
}
